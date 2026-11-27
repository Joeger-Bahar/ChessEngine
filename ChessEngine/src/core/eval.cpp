#include "eval.hpp"
#include "movegen.hpp"

#include <algorithm>
#include <bit>
#include <cstdlib>

// S with a middlegame and endgame part. The eval blends them
struct S
{
	int mg = 0;
	int eg = 0;
	constexpr S() = default;
	constexpr S(int m, int e) : mg(m), eg(e) {}
};

constexpr S operator+(S a, S b) { return S(a.mg + b.mg, a.eg + b.eg); }
constexpr S operator-(S a, S b) { return S(a.mg - b.mg, a.eg - b.eg); }
constexpr S operator*(S a, int k) { return S(a.mg * k, a.eg * k); }
constexpr S operator+=(S& a, S b) { return S(a.mg += b.mg, a.eg += b.eg); }
constexpr S operator-=(S& a, S b) { return S(a.mg -= b.mg, a.eg -= b.eg); }

// Header with compilation of all the eval terms
#include "evalParams.hpp"

// Easy to use structs bc it doesn't start with NONE
enum { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum { WHITE, BLACK };

constexpr Bitboard FileABB = 0x0101010101010101ULL;
constexpr Bitboard FileHBB = FileABB << 7;
constexpr Bitboard Center = (1ULL << 27) | (1ULL << 28) | (1ULL << 35) | (1ULL << 36); // d5 e5 d4 e4
constexpr Bitboard LightSquares = 0x55AA55AA55AA55AAULL;

constexpr Bitboard FileBB(int f) { return FileABB << f; }
constexpr Bitboard RowBB(int r) { return 0xFFULL << (8 * r); }
constexpr int Row(int sq) { return sq >> 3; }
constexpr int Col(int sq) { return sq & 7; }

// Rank as seen by color c. 0 = own back rank, 7 = pawns promote
constexpr int RelRank(int c, int sq) { return c == WHITE ? 7 - Row(sq) : Row(sq); }
constexpr Bitboard RelRankBB(int c, int r) { return RowBB(c == WHITE ? 7 - r : r); }

// One square toward the enemy (Up) or toward us
constexpr Bitboard Up(int c, Bitboard b) { return c == WHITE ? b >> 8 : b << 8; }
constexpr Bitboard Down(int c, Bitboard b) { return c == WHITE ? b << 8 : b >> 8; }

constexpr Bitboard PawnAttakcksBB(int c, Bitboard p)
{
	return c == WHITE ? ((p & ~FileABB) >> 9) | ((p & ~FileHBB) >> 7)
		: ((p & ~FileABB) << 7) | ((p & ~FileHBB) << 9);
}
constexpr Bitboard PawnDoubleAttacksBB(int c, Bitboard p)
{
	return c == WHITE ? ((p & ~FileABB) >> 9) & ((p & ~FileHBB) >> 7)
		: ((p & ~FileABB) << 7) & ((p & ~FileHBB) << 9);
}
inline int LSB(Bitboard b) { return std::countr_zero(b); }
inline int MSB(Bitboard b) { return 63 - std::countl_zero(b); }
inline int PopLSB(Bitboard& b) { int s = std::countr_zero(b); b &= b - 1; return s; }
inline int Count(Bitboard b) { return std::popcount(b); }

constexpr Bitboard QueenSide = FileBB(0) | FileBB(1) | FileBB(2) | FileBB(3);
constexpr Bitboard CenterFiles = FileBB(2) | FileBB(3) | FileBB(4) | FileBB(5);
constexpr Bitboard KingSide = FileBB(4) | FileBB(5) | FileBB(6) | FileBB(7);
constexpr Bitboard KingFlank[8] = {
	QueenSide ^ FileBB(3), QueenSide, QueenSide, CenterFiles,
	CenterFiles, KingSide, KingSide, KingSide ^ FileBB(4)
};

// Precomputed square relationships
struct Tables
{
	Bitboard adjacentFiles[8];
	Bitboard forwardRanks[2][64];   // Every row in front of the square
	Bitboard forwardFile[2][64];    // The squares in front of it on its file
	Bitboard pawnAttackSpan[2][64]; // The squares in front of it on the neighbouring files
	Bitboard passedMask[2][64];     // Both: where an enemy pawn would stop a passed pawn
	int distance[64][64];
	Bitboard rookRays[64];          // Rook moves on an empty board
	Bitboard bishopRays[64];
	Bitboard between[64][64];       // Squares strictly between two squares on a line, else 0
	Bitboard line[64][64];          // The whole line through two squares, else 0

	Tables()
	{
		for (int f = 0; f < 8; ++f)
			adjacentFiles[f] = (f > 0 ? FileBB(f - 1) : 0) | (f < 7 ? FileBB(f + 1) : 0);

		for (int sq = 0; sq < 64; ++sq)
		{
			int r = Row(sq);
			forwardRanks[WHITE][sq] = r > 0 ? (1ULL << (8 * r)) - 1 : 0;
			forwardRanks[BLACK][sq] = r < 7 ? ~((1ULL << (8 * (r + 1))) - 1) : 0;
			for (int c = 0; c < 2; ++c)
			{
				forwardFile[c][sq] = forwardRanks[c][sq] & FileBB(Col(sq));
				pawnAttackSpan[c][sq] = forwardRanks[c][sq] & adjacentFiles[Col(sq)];
				passedMask[c][sq] = forwardFile[c][sq] | pawnAttackSpan[c][sq];
			}
			for (int s2 = 0; s2 < 64; ++s2)
			{
				distance[sq][s2] = std::max(std::abs(Row(sq) - Row(s2)), std::abs(Col(sq) - Col(s2)));
				between[sq][s2] = 0;
				line[sq][s2] = 0;
			}
		}

		// Walk the 8 directions from every square
		const int dirs[8][2] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {1,-1}, {-1,1}, {-1,-1} };
		for (int sq = 0; sq < 64; ++sq)
		{
			rookRays[sq] = bishopRays[sq] = 0;
			for (int d = 0; d < 8; ++d)
			{
				int dr = dirs[d][0], dc = dirs[d][1];
				// The full line through sq in this direction (both ways)
				Bitboard full = 1ULL << sq;
				for (int k = 1; k < 8; ++k)
				{
					int r = Row(sq) + dr * k, c = Col(sq) + dc * k;
					if (r < 0 || r > 7 || c < 0 || c > 7) break;
					full |= 1ULL << (r * 8 + c);
				}
				for (int k = 1; k < 8; ++k)
				{
					int r = Row(sq) - dr * k, c = Col(sq) - dc * k;
					if (r < 0 || r > 7 || c < 0 || c > 7) break;
					full |= 1ULL << (r * 8 + c);
				}
				Bitboard gap = 0;
				for (int k = 1; k < 8; ++k)
				{
					int r = Row(sq) + dr * k, c = Col(sq) + dc * k;
					if (r < 0 || r > 7 || c < 0 || c > 7) break;
					int to = r * 8 + c;
					if (d < 4) rookRays[sq] |= 1ULL << to;
					else       bishopRays[sq] |= 1ULL << to;
					between[sq][to] = gap;
					line[sq][to] = full;
					gap |= 1ULL << to;
				}
			}
		}
	}
};

const Tables T;

// Everything the terms share
struct EvalInfo
{
	const BitboardBoard* board;
	const Square* mailbox;
	Bitboard pawns[2];
	Bitboard pieces[2]; // All pieces of a color
	int kingSq[2];
	Bitboard attacked[2][7]; // [color][0] = by anything, [1 + type] = that piece type
	Bitboard mobilityArea[2];
	Bitboard kingRing[2];
	Bitboard pawnAttackSpan[2]; // Every square the color's pawns could ever attack
	Bitboard passed[2];
	Bitboard pinned[2]; // Our pieces pinned to the king

	int attackersCount[2]; // Pieces of this color attacking the enemy king
	int attackerTypes[2][4]; // by type: knight, bishop, rook, queen
	int kingAttacksCount[2]; // Attacks on squares next to the enemy king
	S score[2];
};

// Cache for the pawn structure. One per thread
struct PawnEntry
{
	Bitboard pawns[2];
	S score[2];
	Bitboard passed[2];
	Bitboard span[2];
};

constexpr int PAWN_TABLE_BITS = 13;
thread_local PawnEntry pawnTable[1 << PAWN_TABLE_BITS];

inline Bitboard Attacked(const EvalInfo& info, int c, int type) { return info.attacked[c][1 + type]; }
inline Bitboard PiecesOf(const EvalInfo& info, int c, int type) { return info.board->pieceBitboards[c][type]; }
inline int TypeOn(const EvalInfo& info, int sq) { return (int)info.mailbox[sq].GetPiece().GetType() - 1; }

S EvalPawns(EvalInfo& info, int us)
{
	const int them = us ^ 1;
	S score;
	info.passed[us] = 0;
	info.pawnAttackSpan[us] = 0;

	Bitboard ours = info.pawns[us];
	Bitboard theirs = info.pawns[them];
	Bitboard b = ours;
	while (b)
	{
		int sq = PopLSB(b);
		int r = RelRank(us, sq);
		int f = Col(sq);
		Bitboard stopSq = Up(us, 1ULL << sq);
		info.pawnAttackSpan[us] |= T.pawnAttackSpan[us][sq];
		bool opposed = (theirs & T.forwardFile[us][sq]) != 0;

		Bitboard neighbours = ours & T.adjacentFiles[f];
		Bitboard phalanx = neighbours & RowBB(Row(sq));
		Bitboard support = ours & Movegen::GetPawnAttacks()[them][sq]; // Our defending pawns
		bool doubled = (ours & T.forwardFile[us][sq]) != 0;
		bool passed = !(theirs & T.passedMask[us][sq]) && !doubled;

		// Backward pawn. No pawn of ours beside or behind and its stop square
		// is controlled or blocked by an enemy pawn
		bool backward = !(neighbours & ~T.forwardRanks[us][sq]) && ((PawnAttakcksBB(them, theirs) | theirs) & stopSq);

		if (phalanx) score += Phalanx[r];
		if (support) score += Supported[r];

		if (!neighbours) score += Isolated;
		else if (backward) score += Backward;

		// Isolated or backward on a half open file is easy target for rooks
		if (!opposed && (!neighbours || backward))
			score += WeakUnopposed;

		// Blocked by enemy pawn
		if (theirs & stopSq) score += BlockedPawn[r];

		if (doubled) score += Doubled;

		if (passed)
		{
			info.passed[us] |= 1ULL << sq;
			score += PassedRank[r];
		}
	}
	return score;
}

// Endgame scale factor (64 is normal). Drawish material gets its endgame score scaled down
int ScaleFactor(const EvalInfo& info, int egScore)
{
	int strong = egScore >= 0 ? WHITE : BLACK;
	int weak = strong ^ 1;
	auto npm = [&](int c)
	{
		return 320 * Count(PiecesOf(info, c, KNIGHT)) + 330 * Count(PiecesOf(info, c, BISHOP))
			+ 500 * Count(PiecesOf(info, c, ROOK)) + 900 * Count(PiecesOf(info, c, QUEEN));
	};
	int npmStrong = npm(strong);
	int npmWeak = npm(weak);
	int strongPawns = Count(info.pawns[strong]);

	// No pawns and at most a minor piece ahead is hard or impossible to win
	if (!strongPawns && npmStrong - npmWeak <= 330)
		return npmStrong < 500 ? 0 : (npmWeak <= 330 ? 4 : 14);
	// Can't get mate with 2 knights
	if (!strongPawns && npmStrong == 640 && Count(PiecesOf(info, strong, KNIGHT)) == 2)
		return 0;
	// One pawn and a minor piece ahead
	if (strongPawns == 1 && npmStrong - npmWeak <= 330)
		return 48;

	// Opposite colored bishops
	Bitboard wb = PiecesOf(info, WHITE, BISHOP);
	Bitboard bb = PiecesOf(info, BLACK, BISHOP);
	if (Count(wb) == 1 && Count(bb) == 1 && (bool(wb & LightSquares) != bool(bb & LightSquares)))
	{
		if (npm(WHITE) == 330 && npm(BLACK) == 330)
			return 18 + 4 * Count(info.passed[strong]);
		return 22 + 4 * Count(info.pieces[strong]);
	}

	// Rook endgame where the strong side's extra pawn is on a flank
	// and the weak king guards its pawns
	if (npm(WHITE) == 500 && npm(BLACK) == 500
		&& strongPawns - Count(info.pawns[weak]) <= 1
		&& bool(KingSide & info.pawns[strong]) != bool(QueenSide & info.pawns[strong])
		&& (Movegen::GetKingAttacks()[info.kingSq[weak]] & info.pawns[weak]))
		return 36;

	// Only one queen on the board so the side without it can hold with enough minor pieces
	Bitboard queens = PiecesOf(info, WHITE, QUEEN) | PiecesOf(info, BLACK, QUEEN);
	if (Count(queens) == 1)
	{
		int other = (PiecesOf(info, WHITE, QUEEN) ? BLACK : WHITE);
		return 37 + 3 * (Count(PiecesOf(info, other, BISHOP)) + Count(PiecesOf(info, other, KNIGHT)));
	}

	// Fewer pawns = more drawish
	return std::min(64, 36 + 7 * strongPawns);
}


int Eval(Color player, const Engine* engine)
{
	const BitboardBoard& board = engine->GetBitboardBoard();
	EvalInfo info;
	info.board = &board;
	info.mailbox = engine->GetBoard();
	for (int c = 0; c < 2; ++c)
	{
		info.pawns[c] = board.pieceBitboards[c][PAWN];
		info.pieces[c] = board.allPieces[c];
		info.kingSq[c] = LSB(board.pieceBitboards[c][KING]);
		info.passed[c] = 0;
		info.score[c] = S();
	}

	// Material and PST. Phase counts the pieces left from 24-0
	int phase = 0;
	for (int us = 0; us < 2; ++us)
	{
		for (int type = PAWN; type <= KING; ++type)
		{
			Bitboard b = board.pieceBitboards[us][type];
			int n = Count(b);
			if (type != KING)
			{
				info.score[us] += PieceValue[type] * n;
			}
			phase += n * (type == KNIGHT || type == BISHOP ? 1 : type == ROOK ? 2 : type == QUEEN ? 4 : 0);

			while (b)
			{
				int sq = PopLSB(b);
				int idx = us == WHITE ? sq : sq ^ 56;
				info.score[us] += PST[type][idx];
			}
		}
	}
	phase = std::min(phase, 24);

	// Pawn structure depends on the pawns so get it from cache if possible
	uint64_t hash = (info.pawns[WHITE] ^ (info.pawns[BLACK] * 0x9E3779B97F4A7C15ULL)) * 0xD6E8FEB86659FD93ULL;
	PawnEntry& entry = pawnTable[hash >> (64 - PAWN_TABLE_BITS)];
	// Update if outdated
	if (entry.pawns[WHITE] != info.pawns[WHITE] || entry.pawns[BLACK] != info.pawns[BLACK])
	{
		entry.pawns[WHITE] = info.pawns[WHITE];
		entry.pawns[BLACK] = info.pawns[BLACK];
		for (int us = 0; us < 2; ++us)
		{
			entry.score[us] = EvalPawns(info, us);
			entry.passed[us] = info.passed[us];
			entry.span[us] = info.pawnAttackSpan[us];
		}
	}
	// Get actual cached values
	for (int us = 0; us < 2; ++us)
	{
		info.score[us] += entry.score[us];
		info.passed[us] = entry.passed[us];
		info.pawnAttackSpan[us] = entry.span[us];
	}

	// Material combinations
	for (int us = 0; us < 2; ++us)
	{
		if (Count(board.pieceBitboards[us][BISHOP]) >= 2) info.score[us] += BishopPair;
		int pawnCount = Count(info.pawns[us]);
		int n = Count(board.pieceBitboards[us][KNIGHT]) * pawnCount;
		info.score[us] += KnightPawns * n;

		n = Count(board.pieceBitboards[us][ROOK]) * pawnCount;
		info.score[us] += RookPawns * n;
	}

	// Small bonus for STM
	info.score[IsWhite(GameState::currentPlayer) ? WHITE : BLACK] += Tempo;

	S total = info.score[WHITE] - info.score[BLACK];

	int scale = ScaleFactor(info, total.eg);
	// Blend middle and end game by phase
	int finalEval = (total.mg * phase + total.eg * (24 - phase) * scale / 64) / 24;
	return player == Color::WHITE ? finalEval : -finalEval;
}
