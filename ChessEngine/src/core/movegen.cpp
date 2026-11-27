#include "movegen.hpp"

#include "boardCalculator.hpp"
#include "engine.hpp"

#include <vector>
#include <iostream>
#include <immintrin.h>

#include "constants.hpp"

inline int PopLSB(Bitboard& b)
{
	// assumes b != 0
#if defined(_MSC_VER)
	unsigned long idx32 = 0;
	_BitScanForward64(&idx32, b);    // idx32 will hold 0..63
	b &= (b - 1);                    // clear LSB
	return static_cast<int>(idx32);
#else
	int idx = __builtin_ctzll(b);    // count trailing zeros -> index of LSB
	b &= (b - 1);
	return idx;
#endif
}

inline int PopCount64(uint64_t x)
{
#if defined(_MSC_VER)
	return (int)__popcnt64(x);
#else
	return __builtin_popcountll(x);
#endif
}

inline int FirstLSBIndex(Bitboard b)
{
	if (!b) return -1;
#if defined(_MSC_VER)
	unsigned long idx32 = 0;
	_BitScanForward64(&idx32, b);
	return static_cast<int>(idx32);
#else
	return __builtin_ctzll(b);
#endif
}

// Define offsets and directions
const int Movegen::bishopDirs[4] = {
	-9,   -7,

	7,	   9
};
const int Movegen::rookDirs[4] = {
		-8,
	 -1,   1,
		 8
};
const int Movegen::queenDirs[8] = {
	-9, -8, -7,
	-1,      1,
	 7,  8,  9
};

Bitboard pawnAttacks[2][64];
Bitboard knightAttacks[64];
Bitboard kingAttacks[64];

Bitboard rookAttacks[64][4096];   // 4096 = 2^12, max rook relevant occupancy bits
Bitboard bishopAttacks[64][512];  // 512 = 2^9, max bishop relevant occupancy bits
Bitboard rookMasks[64];
Bitboard bishopMasks[64];

// PEXT (BMI2) attack tables. _pext_u64 packs the relevant occupancy bits straight into an
// index, so there's no magic multiply, and the tables are dense: ~840 KB instead of ~2.3 MB,
// which fits far better in cache. Needs a CPU with BMI2 (Intel since 2013, AMD Zen 3+).
static Bitboard rookPextAttacks[102400];
static Bitboard bishopPextAttacks[5248];
static uint32_t rookPextOffset[64];
static uint32_t bishopPextOffset[64];

bool Movegen::IsSquareAttacked(int sq, Color byColor, const BitboardBoard& board)
{
	int c = IsWhite(byColor) ? 0 : 1;
	int friendly = 1 - c;

	Bitboard sqMask = 1ULL << sq;

	// Check for friendly pieces on the square
	if (board.allPieces[c] & sqMask)
		return false;

	// Friendly because I need opposite attacks (pawn moves aren't omni-directional)
	if (pawnAttacks[friendly][sq] & board.pieceBitboards[c][(int)Pieces::PAWN - 1])
		return true;

	if (knightAttacks[sq] & board.pieceBitboards[c][(int)Pieces::KNIGHT - 1])
		return true;

	// Do king first because fast lookup
	if (kingAttacks[sq] & board.pieceBitboards[c][static_cast<int>(Pieces::KING) - 1])
		return true;

	// Bishops / Queens (diagonals)
	Bitboard bishopMoves = BishopAttacks(sq, board.occupied);
	if (bishopMoves & (board.pieceBitboards[c][static_cast<int>(Pieces::BISHOP) - 1] |
		board.pieceBitboards[c][static_cast<int>(Pieces::QUEEN) - 1]))
		return true;

	// Rooks / Queens (orthogonal)
	Bitboard rookMoves = RookAttacks(sq, board.occupied);
	if (rookMoves & (board.pieceBitboards[c][static_cast<int>(Pieces::ROOK) - 1] |
		board.pieceBitboards[c][static_cast<int>(Pieces::QUEEN) - 1]))
		return true;

	return false;
}

// Valid moves for a single piece
std::vector<uint8_t> Movegen::GetValidMoves(int sq, const BitboardBoard& board)
{
	Piece piece;
	if (!BoardCalculator::GetPieceAt(sq, board, piece)) return {};

	Color color = piece.GetColor();
	int c = IsWhite(color) ? 0 : 1;
	Bitboard movesMask = EMPTY_BITBOARD;

	switch (piece.GetType())
	{
	case Pieces::PAWN:   movesMask = PawnMoves(sq, color, board); break;
	case Pieces::KNIGHT: movesMask = KnightMoves(sq, color, board); break;
	case Pieces::BISHOP: movesMask = SlidingMoves(sq, color, Pieces::BISHOP, board); break;
	case Pieces::ROOK:   movesMask = SlidingMoves(sq, color, Pieces::ROOK, board); break;
	case Pieces::QUEEN:  movesMask = SlidingMoves(sq, color, Pieces::QUEEN, board); break;
	case Pieces::KING:   movesMask = KingMoves(sq, color, board); break;
	default: return {};
	}

	// Now filter out moves that would put or leave the king in check
	// Find king position
	const int kingSq = FirstLSBIndex(board.pieceBitboards[c][static_cast<int>(Pieces::KING) - 1]);
	if (kingSq == -1)
	{
		std::cout << "Color: " << c << '\n';
	}

	std::vector<uint8_t> validMoves;
	Bitboard bb = movesMask;

	while (bb)
	{
		int endSq = PopLSB(bb);

		BitboardBoard temp = board;

		// Make move
		Piece captured;
		BoardCalculator::GetPieceAt(endSq, temp, captured);
		temp.Remove(piece, sq);
		if (captured.GetType() != Pieces::NONE)
			temp.Remove(captured, endSq);
		// En passant: the captured pawn is behind the target square (it may be the piece giving check)
		else if (piece.GetType() == Pieces::PAWN && ToCol(endSq) != ToCol(sq))
			temp.Remove(Piece(Pieces::PAWN, Opponent(color)), endSq + (IsWhite(color) ? 8 : -8));
		temp.Add(piece, endSq);

		// Update king square if moved
		int kingSqNew = (piece.GetType() == Pieces::KING ? endSq : kingSq);

		// This fails because if a white piece is getting moves, the white move can capture the black king

		// Legality check
		if (!IsSquareAttacked(kingSqNew, Opponent(color), temp))
			validMoves.push_back(endSq);
	}

	return validMoves;
}

std::vector<Move> Movegen::GetAllLegalMoves(Color color, const BitboardBoard& board, Engine* engine)
{
	if (engine->IsOver()) return std::vector<Move>();
	std::vector<Move> moves;

	GetAllMoves(moves, color, board, engine);
	moves.erase(
		std::remove_if(moves.begin(), moves.end(),
			[&](Move move)
			{
				// Remove moves that cause checks
				engine->MakeMove(move);
				bool inCheck = engine->InCheck(Opponent(GameState::currentPlayer));
				engine->UndoMove();
				return inCheck;
			}),
		moves.end()
	);

	return moves;
}

void Movegen::GetAllMoves(std::vector<Move>& moves, Color color, const BitboardBoard& board, Engine* engine,
	bool onlyNoisy, bool onlyCaptures)
{
	moves.clear();
	int c = IsWhite(color) ? 0 : 1;

	// Squares a piece of type t would give check from
	Bitboard checkSquares[6] = {};
	// Our pieces standing between a slider and enemy king
	Bitboard discoverers = 0;
	bool useCheckSquares = false;
	// For qsearch work out which squares give check instead of asking GivesCheck
	if (onlyNoisy && !onlyCaptures)
	{
		int them = 1 - c;
		int kingSq = FirstLSBIndex(board.pieceBitboards[them][(int)Pieces::KING - 1]);
		if (kingSq >= 0)
		{
			useCheckSquares = true;
			Bitboard occ = board.occupied;
			Bitboard diagonalFromKing = BishopAttacks(kingSq, occ);
			Bitboard straightFromKing = RookAttacks(kingSq, occ);

			checkSquares[(int)Pieces::PAWN - 1] = pawnAttacks[them][kingSq];
			checkSquares[(int)Pieces::KNIGHT - 1] = knightAttacks[kingSq];
			checkSquares[(int)Pieces::BISHOP - 1] = diagonalFromKing;
			checkSquares[(int)Pieces::ROOK - 1] = straightFromKing;
			checkSquares[(int)Pieces::QUEEN - 1] = diagonalFromKing | straightFromKing;
			// King never gives direct check

			Bitboard ourDiagonal = board.pieceBitboards[c][(int)Pieces::BISHOP - 1] |
				board.pieceBitboards[c][(int)Pieces::QUEEN - 1];
			Bitboard ourStraight = board.pieceBitboards[c][(int)Pieces::ROOK - 1] |
				board.pieceBitboards[c][(int)Pieces::QUEEN - 1];

			// Take away a piece inbetween our piece and the king. If LOS, its a discoverer
			Bitboard firstInLine = diagonalFromKing & board.allPieces[c];
			while (firstInLine)
			{
				int b = PopLSB(firstInLine);
				Bitboard bit = 1ULL << b;
				if (BishopAttacks(kingSq, occ ^ bit) & ourDiagonal & ~bit)
					discoverers |= bit;
			}
			firstInLine = straightFromKing & board.allPieces[c];
			while (firstInLine)
			{
				int b = PopLSB(firstInLine);
				Bitboard bit = 1ULL << b;
				if (RookAttacks(kingSq, occ ^ bit) & ourStraight & ~bit)
					discoverers |= bit;
			}
		}
	}

	// Iterate pieces by bitboards
	for (int t = 0; t < 6; ++t)
	{
		Bitboard bb = board.pieceBitboards[c][t];
		while (bb) // For each piece
		{
			int sq = PopLSB(bb);

			Pieces type = (Pieces)(t + 1); // Pawn is 1
			Bitboard pieceMoves = EMPTY_BITBOARD;

			switch (type)
			{
			case Pieces::PAWN:   pieceMoves = PawnMoves(sq, color, board); break;
			case Pieces::KNIGHT: pieceMoves = KnightMoves(sq, color, board); break;
			case Pieces::BISHOP: pieceMoves = SlidingMoves(sq, color, Pieces::BISHOP, board); break;
			case Pieces::ROOK:   pieceMoves = SlidingMoves(sq, color, Pieces::ROOK, board); break;
			case Pieces::QUEEN:  pieceMoves = SlidingMoves(sq, color, Pieces::QUEEN, board); break;
			case Pieces::KING:   pieceMoves = (onlyNoisy && onlyCaptures) ? kingAttacks[sq] & board.allPieces[1 - c] : KingMoves(sq, color, board); break;
			default: break;
			}

			// Drop empty move squares. Enpassant and promotion go to empty square so keep
			if (onlyNoisy && onlyCaptures)
			{
				Bitboard keep = board.allPieces[1 - c];
				// Pawn stuff
				if (type == Pieces::PAWN)
				{
					keep |= 0xFF000000000000FFULL; // Rows 0 and 7
					if (GameState::enPassantTarget != -1)
						Set(keep, GameState::enPassantTarget);
				}

				pieceMoves &= keep;
			}

			Bitboard pm = pieceMoves;
			while (pm)
			{
				int endSq = PopLSB(pm);

				bool isCastle = (type == Pieces::KING && std::abs(endSq - sq) == 2);
				bool isEnPassant = (type == Pieces::PAWN && (ToCol(sq) != ToCol(endSq)) &&
					!IsSet(board.occupied, endSq));

				// Promotions
				if (type == Pieces::PAWN && (ToRow(endSq) == 0 || ToRow(endSq) == 7))
				{
					for (Pieces promo : {Pieces::QUEEN, Pieces::ROOK, Pieces::BISHOP, Pieces::KNIGHT})
						moves.push_back(EncodeMove(sq, endSq, (int)promo, isEnPassant, isCastle));
					continue;
				}

				Move move = EncodeMove(sq, endSq, (int)Pieces::NONE, isEnPassant, isCastle);

				if (onlyNoisy)
				{
					// Captures
					if (MoveIsCapture(move, board))
					{
						moves.push_back(move);
						continue;
					}
					if (onlyCaptures)
						continue;

					// Checks
					bool givesCheck;
					if (!useCheckSquares || isCastle || (discoverers & (1ULL << sq)))
						givesCheck = engine->GivesCheck(move); // Full test for castling or discovered check
					else
						givesCheck = (checkSquares[t] >> endSq) & 1;

					if (givesCheck)
						moves.push_back(move);
				}
				else
					moves.push_back(move);
			}
		}
	}
}

void Movegen::GetQuietMoves(std::vector<Move>& moves, Color color, const BitboardBoard& board)
{
	moves.clear();
	int c = IsWhite(color) ? 0 : 1;

	for (int t = 0; t < 6; ++t)
	{
		Bitboard bb = board.pieceBitboards[c][t];
		while (bb)
		{
			int sq = PopLSB(bb);
			Pieces type = (Pieces)(t + 1);

			Bitboard targets = EMPTY_BITBOARD;
			switch (type)
			{
			case Pieces::PAWN: targets = PawnMoves(sq, color, board); break;
			case Pieces::KNIGHT: targets = KnightMoves(sq, color, board); break;
			case Pieces::KING: targets = KingMoves(sq, color, board); break;
			default: targets = SlidingMoves(sq, color, type, board); break;
			}
			targets &= ~board.occupied; // Only empty squares

			while (targets)
			{
				int endSq = PopLSB(targets);

				// En passant and promotion land on empty squares but are noisy
				if (type == Pieces::PAWN && (ToCol(sq) != ToCol(endSq) || ToRow(endSq) == 0 || ToRow(endSq) == 7))
					continue;

				bool isCastle = (type == Pieces::KING && std::abs(endSq - sq) == 2);
				moves.push_back(EncodeMove(sq, endSq, (int)Pieces::NONE, false, isCastle));
			}
		}
	}
}

bool Movegen::IsPseudoLegal(Move move, Color color, const BitboardBoard& board)
{
	int from = GetStart(move);
	int to = GetEnd(move);
	int c = IsWhite(color) ? 0 : 1;

	if (!IsSet(board.allPieces[c], from))
		return false;
	int t = 0;
	while (!IsSet(board.pieceBitboards[c][t], from))
		++t;
	Pieces type = (Pieces)(t + 1);

	Bitboard targets = EMPTY_BITBOARD;
	switch (type)
	{
	case Pieces::PAWN:   targets = PawnMoves(from, color, board); break;
	case Pieces::KNIGHT: targets = KnightMoves(from, color, board); break;
	case Pieces::KING:   targets = KingMoves(from, color, board); break;
	default:             targets = SlidingMoves(from, color, type, board); break;
	}
	if (!IsSet(targets, to))
		return false;

	// A pawn reaching the last row has to promote
	int promotion = GetPromotion(move);
	bool promotes = (type == Pieces::PAWN && (ToRow(to) == 0 || ToRow(to) == 7));
	if (promotes ? (promotion < (int)Pieces::KNIGHT || promotion > (int)Pieces::QUEEN) : promotion != (int)Pieces::NONE)
		return false;

	// The flags have to be the ones GetAllMoves would give
	bool isCastle = (type == Pieces::KING && std::abs(to - from) == 2);
	bool isEnPassant = (type == Pieces::PAWN && ToCol(from) != ToCol(to) && !IsSet(board.occupied, to));
	return move == EncodeMove(from, to, promotion, isEnPassant, isCastle);
}

std::vector<Move> Movegen::GetAllMoves(Color color, const BitboardBoard& board, Engine* engine, bool onlyCaptures)
{
	std::vector<Move> moves;
	GetAllMoves(moves, color, board, engine, onlyCaptures);
	return moves;
}

Bitboard Movegen::GetPseudoAttacks(Pieces piece, int sq, const Bitboard& allOcc, bool isWhite)
{
	// TODO: Might need to fill in the pieces bitboards too for accurate movegen
	Bitboard moves = EMPTY_BITBOARD;
	Color color = isWhite ? Color::WHITE : Color::BLACK;
	switch (piece)
	{
	case Pieces::PAWN:   moves = PawnMoves(sq, color, BitboardBoard{ {}, {}, allOcc }); break;
	case Pieces::KNIGHT: moves = KnightMoves(sq, color, BitboardBoard{ {}, {}, allOcc }); break;
	case Pieces::BISHOP: moves = SlidingMoves(sq, color, Pieces::BISHOP, BitboardBoard{ {}, {}, allOcc }); break;
	case Pieces::ROOK:   moves = SlidingMoves(sq, color, Pieces::ROOK, BitboardBoard{ {}, {}, allOcc }); break;
	case Pieces::QUEEN:  moves = SlidingMoves(sq, color, Pieces::QUEEN, BitboardBoard{ {}, {}, allOcc }); break;
	case Pieces::KING:   moves = KingMoves(sq, color, BitboardBoard{ {}, {}, allOcc }); break;
	default: break;
	}
	return moves;
}

//Bitboard Movegen::RookAttacks(int sq, Bitboard occ)
//{
//	return rookAttacks[sq][((occ & rookMasks[sq]) * rookMagics[sq]) >> rookShifts[sq]];
//}
//
//Bitboard Movegen::BishopAttacks(int sq, Bitboard occ)
//{
//	return bishopAttacks[sq][((occ & bishopMasks[sq]) * bishopMagics[sq]) >> bishopShifts[sq]];
//}
Bitboard Movegen::RookAttacks(int sq, Bitboard occ)
{
	return rookPextAttacks[rookPextOffset[sq] + _pext_u64(occ, rookMasks[sq])];
}

Bitboard Movegen::BishopAttacks(int sq, Bitboard occ)
{
	return bishopPextAttacks[bishopPextOffset[sq] + _pext_u64(occ, bishopMasks[sq])];
}

const Bitboard(&Movegen::GetPawnAttacks())[2][64]
{
	return pawnAttacks;
}
const Bitboard(&Movegen::GetKnightAttacks())[64]
{
	return knightAttacks;
}
const Bitboard(&Movegen::GetKingAttacks())[64]
{
	return kingAttacks;
}
const Bitboard(&Movegen::GetRookAttacks())[64][4096]
{
	return rookAttacks;
}
const Bitboard(&Movegen::GetBishopAttacks())[64][512]
{
	return bishopAttacks;
}

Bitboard Movegen::KingMoves(int sq, Color color, const BitboardBoard& board)
{
	Bitboard moves = kingAttacks[sq] & ~board.allPieces[(int)color];

	if (BoardCalculator::IsCastlingValid(true, board, color)) // Kingside
		Set(moves, ToIndex(ToRow(sq), ToCol(sq) + 2));
	if (BoardCalculator::IsCastlingValid(false, board, color)) // Queenside
		Set(moves, ToIndex(ToRow(sq), ToCol(sq) - 2));

	return moves;
}

Bitboard Movegen::PawnMoves(int sq, Color color, const BitboardBoard& board)
{
	Bitboard moves = EMPTY_BITBOARD;
	int r = ToRow(sq);
	int c = ToCol(sq);
	int dir = IsWhite(color) ? -8 : 8;

	int oneStep = sq + dir;
	if (IdxInBounds(oneStep) && !IsSet(board.occupied, oneStep))
		Set(moves, oneStep);

	// Double push
	if ((IsWhite(color) && r == 6) || IsBlack(color) && r == 1)
	{
		int twoStep = sq + (IsWhite(color) ? -16 : 16);

		if (!IsSet(board.occupied, oneStep) && !IsSet(board.occupied, twoStep))
			Set(moves, twoStep);
	}

	// Captures
	Bitboard attacks = pawnAttacks[IsWhite(color) ? 0 : 1][sq] & board.allPieces[IsWhite(color) ? 1 : 0]; // Opponent pieces
	moves |= attacks;

	// En passant
	if (GameState::enPassantTarget != -1)
	{
		//std::cout << "En passant target: " << GameState::enPassantTarget << '\n';
		int epSq = GameState::enPassantTarget;
		if (ToRow(epSq) == r + (IsWhite(color) ? -1 : 1) && std::abs(ToCol(epSq) - c) == 1)
			Set(moves, epSq);
	}

	return moves;
}

Bitboard Movegen::KnightMoves(int sq, Color color, const BitboardBoard& board)
{
	return knightAttacks[sq] & ~board.allPieces[(int)color];
}

Bitboard Movegen::SlidingMoves(int sq, Color color, const Pieces piece,
	const BitboardBoard& board, bool includeBlockers)
{
	Bitboard occ = board.occupied;
	Bitboard moves = 0ULL;

	bool rookMoves = (piece == Pieces::ROOK || piece == Pieces::QUEEN);
	bool bishopMoves = (piece == Pieces::BISHOP || piece == Pieces::QUEEN);

	if (rookMoves)
	{
		moves |= RookAttacks(sq, occ);
	}
	if (bishopMoves)
	{
		moves |= BishopAttacks(sq, occ);
	}

	if (!includeBlockers)
		moves &= ~board.allPieces[(int)color];

	return moves;
}

// Precompute knight and king attacks
void Movegen::InitPrecomputedAttacks()
{
	for (int sq = 0; sq < 64; ++sq)
	{
		int r = ToRow(sq);
		int c = ToCol(sq);

		// Knight moves
		Bitboard mask = EMPTY_BITBOARD;

		int dr[8] = { 2,  2, 1,  1, -1, -1, -2, -2 };
		int dc[8] = { 1, -1, 2, -2,  2, -2,  1, -1 };

		for (int i = 0; i < 8; ++i)
		{
			int nr = r + dr[i];
			int nc = c + dc[i];

			if (InBounds(nr, nc))
				mask |= 1ULL << ToIndex(nr, nc);
		}
		knightAttacks[sq] = mask;

		// King moves
		mask = EMPTY_BITBOARD;
		for (int dr = -1; dr <= 1; ++dr)
		{
			for (int dc = -1; dc <= 1; ++dc)
			{
				if (dr == 0 && dc == 0) continue;
				int nr = r + dr;
				int nc = c + dc;
				if (InBounds(nr, nc))
					mask |= 1ULL << ToIndex(nr, nc);
			}
		}
		kingAttacks[sq] = mask;

		// Pawn attacks
		// White pawns
		mask = EMPTY_BITBOARD;
		if (InBounds(r - 1, c - 1)) mask |= 1ULL << ToIndex(r - 1, c - 1);
		if (InBounds(r - 1, c + 1)) mask |= 1ULL << ToIndex(r - 1, c + 1);
		pawnAttacks[(int)Color::WHITE][sq] = mask;

		mask = EMPTY_BITBOARD;
		if (InBounds(r + 1, c - 1)) mask |= 1ULL << ToIndex(r + 1, c - 1);
		if (InBounds(r + 1, c + 1)) mask |= 1ULL << ToIndex(r + 1, c + 1);
		pawnAttacks[(int)Color::BLACK][sq] = mask;
	}

	BuildMagicAttackTables();
}

static Bitboard RookMask(int sq)
{
	Bitboard mask = 0ULL;
	int rank = ToRow(sq);
	int file = ToCol(sq);

	// North
	for (int r = rank + 1; r <= 6; r++) mask |= 1ULL << ToIndex(r, file);
	// South
	for (int r = rank - 1; r >= 1; r--) mask |= 1ULL << ToIndex(r, file);
	// East
	for (int f = file + 1; f <= 6; f++) mask |= 1ULL << ToIndex(rank, f);
	// West
	for (int f = file - 1; f >= 1; f--) mask |= 1ULL << ToIndex(rank, f);

	return mask;
}

static Bitboard BishopMask(int sq)
{
	Bitboard mask = 0ULL;
	int r = ToRow(sq), c = ToCol(sq);

	const int dr[4] = { -1, -1, 1, 1 };
	const int dc[4] = { -1, 1, -1, 1 };

	for (int dir = 0; dir < 4; ++dir)
	{
		int tr = r + dr[dir];
		int tc = c + dc[dir];
		while (InBounds(tr, tc))
		{
			// Stop before the edge square
			if (tr == 0 || tr == 7 || tc == 0 || tc == 7) break;
			Set(mask, ToIndex(tr, tc));
			tr += dr[dir];
			tc += dc[dir];
		}
	}
	return mask;
}

static Bitboard SetOccupancy(int index, int bitsCount, Bitboard mask)
{
	Bitboard occ = 0ULL;
	int bitIndex = 0;
	for (int sq = 0; sq < 64; ++sq)
	{
		if (mask & (1ULL << sq))
		{
			if (index & (1 << bitIndex))
				occ |= (1ULL << sq);
			++bitIndex;
			if (bitIndex >= bitsCount) break;
		}
	}
	return occ;
}

static Bitboard ComputeRookAttacks(int sq, Bitboard occ)
{
	Bitboard attacks = 0ULL;
	int r = ToRow(sq), c = ToCol(sq);

	const int dr[4] = { -1, 0, 0, 1 };
	const int dc[4] = { 0, -1, 1, 0 };

	for (int dir = 0; dir < 4; ++dir)
	{
		int tr = r + dr[dir];
		int tc = c + dc[dir];
		while (InBounds(tr, tc))
		{
			int t = ToIndex(tr, tc);
			attacks |= (1ULL << t);
			if (occ & (1ULL << t)) break; // Blocked
			tr += dr[dir];
			tc += dc[dir];
		}
	}
	return attacks;
}

static Bitboard ComputeBishopAttacks(int sq, Bitboard occ)
{
	Bitboard attacks = 0ULL;
	int r = ToRow(sq), c = ToCol(sq);

	const int dr[4] = { -1, -1, 1, 1 };
	const int dc[4] = { -1, 1, -1, 1 };

	for (int dir = 0; dir < 4; ++dir)
	{
		int tr = r + dr[dir];
		int tc = c + dc[dir];
		while (InBounds(tr, tc))
		{
			int t = ToIndex(tr, tc);
			attacks |= (1ULL << t);
			if (occ & (1ULL << t)) break; // Blocked
			tr += dr[dir];
			tc += dc[dir];
		}
	}
	return attacks;
}

void Movegen::BuildMagicAttackTables()
{
	for (int sq = 0; sq < 64; ++sq)
	{
		// Rook
		{
			Bitboard mask = RookMask(sq);
			rookMasks[sq] = mask;

			int bits = PopCount64(mask);
			int tableSize = 1 << bits;

			for (int i = 0; i < tableSize; ++i)
			{
				Bitboard occ = SetOccupancy(i, bits, mask);
				uint64_t index = (occ * rookMagics[sq]) >> rookShifts[sq];
				rookAttacks[sq][index] = ComputeRookAttacks(sq, occ);
			}

			// Handle trivial edge squares (no relevant bits)
			if (bits == 0)
				rookAttacks[sq][0] = ComputeRookAttacks(sq, 0ULL);
		}

		// Bishop
		{
			Bitboard mask = BishopMask(sq);
			bishopMasks[sq] = mask;

			int bits = PopCount64(mask);
			int tableSize = 1 << bits;

			for (int i = 0; i < tableSize; ++i)
			{
				Bitboard occ = SetOccupancy(i, bits, mask);
				uint64_t index = (occ * bishopMagics[sq]) >> bishopShifts[sq];
				bishopAttacks[sq][index] = ComputeBishopAttacks(sq, occ);
			}

			if (bits == 0)
				bishopAttacks[sq][0] = ComputeBishopAttacks(sq, 0ULL);
		}
	}

	// PEXT tables: walk every subset of each mask (carry-rippler) and store its attacks
	uint32_t rookOffset = 0;
	uint32_t bishopOffset = 0;
	for (int sq = 0; sq < 64; ++sq)
	{
		rookPextOffset[sq] = rookOffset;
		Bitboard occ = 0;
		do
		{
			rookPextAttacks[rookOffset + _pext_u64(occ, rookMasks[sq])] = ComputeRookAttacks(sq, occ);
			occ = (occ - rookMasks[sq]) & rookMasks[sq];
		} while (occ);
		rookOffset += 1u << PopCount64(rookMasks[sq]);

		bishopPextOffset[sq] = bishopOffset;
		occ = 0;
		do
		{
			bishopPextAttacks[bishopOffset + _pext_u64(occ, bishopMasks[sq])] = ComputeBishopAttacks(sq, occ);
			occ = (occ - bishopMasks[sq]) & bishopMasks[sq];
		} while (occ);
		bishopOffset += 1u << PopCount64(bishopMasks[sq]);
	}
}
