#include "boardCalculator.hpp"

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

#define NOMINMAX
#include <Windows.h>

#include "gameState.hpp"
#include "engine.hpp"

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
const int BoardCalculator::bishopDirs[4] = {
	-9,   -7,

	7,	   9
};
const int BoardCalculator::rookDirs[4] = {
		-8,
	 -1,   1,
		 8
};
const int BoardCalculator::queenDirs[8] = {
	-9, -8, -7,
	-1,      1,
	 7,  8,  9
};

Bitboard pawnAttacks[2][64];
Bitboard knightAttacks[64];
Bitboard kingAttacks[64];

bool BoardCalculator::IsSquareAttacked(int sq, Color byColor, const BitboardBoard& board)
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
	Bitboard bishopMoves = SlidingMoves(sq, byColor, bishopDirs, 4, board, true);
	if (bishopMoves & (board.pieceBitboards[c][static_cast<int>(Pieces::BISHOP) - 1] |
		board.pieceBitboards[c][static_cast<int>(Pieces::QUEEN) - 1]))
		return true;

	// Rooks / Queens (orthogonal)
	Bitboard rookMoves = SlidingMoves(sq, byColor, rookDirs, 4, board, true);
	if (rookMoves & (board.pieceBitboards[c][static_cast<int>(Pieces::ROOK) - 1] |
		board.pieceBitboards[c][static_cast<int>(Pieces::QUEEN) - 1]))
		return true;

	return false;
}

// Valid moves for a single piece
std::vector<uint8_t> BoardCalculator::GetValidMoves(int sq, const BitboardBoard& board)
{
	Piece piece;
	if (!GetPieceAt(sq, board, piece)) return {};
	
	Color color = piece.GetColor();
	int c = IsWhite(color) ? 0 : 1;
	Bitboard movesMask = EMPTY_BITBOARD;

	switch (piece.GetType())
	{
	case Pieces::PAWN:   movesMask = PawnMoves   (sq, color, board); break;
	case Pieces::KNIGHT: movesMask = KnightMoves (sq, color, board); break;
	case Pieces::BISHOP: movesMask = SlidingMoves(sq, color, bishopDirs, 4, board); break;
	case Pieces::ROOK:   movesMask = SlidingMoves(sq, color, rookDirs, 4, board); break;
	case Pieces::QUEEN:  movesMask = SlidingMoves(sq, color, queenDirs, 8, board); break;
	case Pieces::KING:   movesMask = KingMoves   (sq, color, board); break;
	default: return {};
	}

	// Now filter out moves that would put or leave the king in check
	// Find king position
	int kingSq = FirstLSBIndex(board.pieceBitboards[c][static_cast<int>(Pieces::KING) - 1]);

	std::vector<uint8_t> validMoves;
	Bitboard bb = movesMask;

	while (bb)
	{
		int endSq = PopLSB(bb);

		BitboardBoard temp = board;

		// Make move
		Piece captured;
		GetPieceAt(endSq, temp, captured);
		temp.Remove(piece, sq);
		if (captured.GetType() != Pieces::NONE)
			temp.Remove(captured, endSq);
		temp.Add(piece, endSq);

		// Update king square if moved
		int kingSqNew = (piece.GetType() == Pieces::KING ? endSq : kingSq);

		// Legality check
		if (!IsSquareAttacked(kingSqNew, Opponent(color), temp))
			validMoves.push_back(endSq);
	}
	
	return validMoves;
}

std::vector<Move> BoardCalculator::GetAllLegalMoves(Color color, const BitboardBoard& board, Engine* engine)
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

void BoardCalculator::GetAllMoves(std::vector<Move>& moves, Color color, const BitboardBoard& board, Engine* engine,
	bool onlyNoisy)
{
	moves.clear();
	int c = IsWhite(color) ? 0 : 1;

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
			case Pieces::PAWN:   pieceMoves = PawnMoves   (sq, color, board); break;
			case Pieces::KNIGHT: pieceMoves = KnightMoves (sq, color, board); break;
			case Pieces::BISHOP: pieceMoves = SlidingMoves(sq, color, bishopDirs, 4, board); break;
			case Pieces::ROOK:   pieceMoves = SlidingMoves(sq, color, rookDirs,   4, board); break;
			case Pieces::QUEEN:  pieceMoves = SlidingMoves(sq, color, queenDirs,  8, board); break;
			case Pieces::KING:   pieceMoves = KingMoves   (sq, color, board); break;
			default: break;
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
					}

					// Checks
					engine->MakeMove(move);
					if (engine->InCheck(GameState::currentPlayer))
						moves.push_back(move);
					engine->UndoMove();
				}
				else
					moves.push_back(move);
			}
		}
	}
}

std::vector<Move> BoardCalculator::GetAllMoves(Color color, const BitboardBoard& board, Engine* engine, bool onlyCaptures)
{
	std::vector<Move> moves;
	GetAllMoves(moves, color, board, engine, onlyCaptures);
	return moves;
}

bool BoardCalculator::GetPieceAt(int sq, const BitboardBoard& board, Piece& piece)
{
	if (!IdxInBounds(sq))
	{
		return false;
	}

	uint64_t mask = 1ULL << sq;

	// Check white pieces
	for (int pt = (int)Pieces::PAWN; pt <= (int)Pieces::KING; ++pt)
	{
		if (board.pieceBitboards[(int)Color::WHITE][pt - 1] & mask)
		{
			piece = Piece(static_cast<Pieces>(pt), Color::WHITE);
			return true;
		}
	}

	// Check black pieces
	for (int pt = (int)Pieces::PAWN; pt <= (int)Pieces::KING; ++pt)
	{
		if (board.pieceBitboards[(int)Color::BLACK][pt - 1] & mask)
		{
			piece = Piece(static_cast<Pieces>(pt), Color::BLACK);
			return true;
		}
	}

	return false; // Empty square
}

uint8_t BoardCalculator::FindPiece(Piece piece, const BitboardBoard& board)
{
	if (piece.GetType() == Pieces::NONE || piece.GetColor() == Color::NONE)
		return -1;

	uint64_t bb = board.pieceBitboards[(int)piece.GetColor()][(int)piece.GetType() - 1];
	if (!bb) return -1;

	// Return the least significant square index
	return FirstLSBIndex(bb);
}

bool BoardCalculator::IsEmptyAt(int sq, const BitboardBoard& board)
{
	Piece temp;
	return !GetPieceAt(sq, board, temp); // Returns false if no piece there
}

bool BoardCalculator::IsCastlingValid(bool kingside, const BitboardBoard& board)
{
	Color player = GameState::currentPlayer;
	Color enemy = Opponent(player);
	int row = IsWhite(player) ? 7 : 0;

	// 1. Check if castling rights exist
	if ((player == Color::WHITE && !GameState::whiteCastlingRights[kingside ? 1 : 0]) ||
		(player == Color::BLACK && !GameState::blackCastlingRights[kingside ? 1 : 0]))
		return false;

	// 2. King is in check
	int kingSq = ToIndex(row, 4);

	if (IsSquareAttacked(kingSq, enemy, board))
		return false;

	int kingCol = 4;
	int step = kingside ? 1 : -1;

	// 3. Squares between king and rook are not empty
	int squaresToMove = kingside ? 2 : 3;
	for (int i = 1; i <= squaresToMove; ++i)
	{
		int sq = ToIndex(row, kingCol + step * i);
		if (!IsEmptyAt(sq, board))
			return false;
	}

	// 4. Squares king moves through are attacked
	int squaresToMoveThrough = 2;
	for (int i = 1; i <= squaresToMoveThrough; ++i)
	{
		int sq = ToIndex(row, kingCol + step * i);
		if (IsSquareAttacked(sq, enemy, board))
			return false;
	}

	return true;
}

Bitboard BoardCalculator::KingMoves(int sq, Color color, const BitboardBoard& board)
{
	Bitboard moves = kingAttacks[sq] &~board.allPieces[(int)color];

	if (IsCastlingValid(true, board)) // Kingside
		Set(moves, ToIndex(ToRow(sq), ToCol(sq) + 2));
	if (IsCastlingValid(false, board)) // Queenside
		Set(moves, ToIndex(ToRow(sq), ToCol(sq) - 2));

	return moves;
}

Bitboard BoardCalculator::PawnMoves(int sq, Color color, const BitboardBoard& board)
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

Bitboard BoardCalculator::KnightMoves(int sq, Color color, const BitboardBoard& board)
{
	return knightAttacks[sq] & ~board.allPieces[(int)color];
}

Bitboard BoardCalculator::SlidingMoves(int sq, Color color, const int dirs[], int numDirs,
	const BitboardBoard& board, bool includeBlockers)
{
	Bitboard moves = EMPTY_BITBOARD;

	for (int i = 0; i < numDirs; ++i)
	{
		int dir = dirs[i];
		int target = sq;
		while (true)
		{
			int prevCol = ToCol(target);
			target += dir;
			if (!IdxInBounds(target)) break;
			if (std::abs(ToCol(target) - prevCol) > 1 &&
				(dir == -1 || dir == 1 || dir == -9 || dir == 7 || dir == -7 || dir == 9)) break;

			Set(moves, target); // Always add square (friendly's get maybe removed later)
			if (IsSet(board.occupied, target)) break; // Blocked ray, last square added
		}
	}

	// Remove friendly pieces
	if (!includeBlockers)
		moves &= ~board.allPieces[(int)color];

	return moves;
}

// Precompute knight and king attacks
void BoardCalculator::InitPrecomputedAttacks()
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
}