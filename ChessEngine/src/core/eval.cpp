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

inline Bitboard Attacked(const EvalInfo& ei, int c, int type) { return ei.attacked[c][1 + type]; }
inline Bitboard PiecesOf(const EvalInfo& ei, int c, int type) { return ei.board->pieceBitboards[c][type]; }
inline int TypeOn(const EvalInfo& ei, int sq) { return (int)ei.mailbox[sq].GetPiece().GetType() - 1; }


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

	// Small bonus for STM
	info.score[IsWhite(GameState::currentPlayer) ? WHITE : BLACK] += Tempo;

	S total = info.score[WHITE] - info.score[BLACK];

	int scale = 64;
	// Blend middle and end game by phase
	int finalEval = (total.mg * phase + total.eg * (24 - phase) * scale / 64) / 24;
	return player == Color::WHITE ? finalEval : -finalEval;
}
