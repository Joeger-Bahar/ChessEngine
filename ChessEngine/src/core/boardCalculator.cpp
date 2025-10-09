#include "boardCalculator.hpp"

#include <iostream>

#include "gameState.hpp"
#include "engine.hpp"
#include "movegen.hpp"

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

	if (Movegen::IsSquareAttacked(kingSq, enemy, board))
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
		if (Movegen::IsSquareAttacked(sq, enemy, board))
			return false;
	}

	return true;
}
