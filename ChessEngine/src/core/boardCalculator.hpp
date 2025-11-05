#pragma once

#include "piece.hpp"
#include "square.hpp"
#include "move.hpp"
#include "bitboard.hpp"

#define Opponent(c) ((c) == Color::WHITE ? Color::BLACK : Color::WHITE)
#define IsWhite(c)  (c == Color::WHITE)
#define IsBlack(c)  (!IsWhite(c))

#define ToIndex(row, col) (((row) << 3) | (col))
#define ToRow(idx) ((idx) >> 3)   // 0 = rank 8, 7 = rank 1 ( / 8)
#define ToCol(idx) ((idx) & 7)    // 0 = file a, 7 = file h ( % 8)
#define IdxInBounds(idx)   ((idx) >= 0 && (idx) < 64)
#define InBounds(row, col) ((row) >= 0 && (row) < 8 && (col) >= 0 && (col) < 8)

class Engine;

class BoardCalculator
{
public:
	static bool IsCastlingValid(bool kingside, const BitboardBoard& board);

	static bool GetPieceAt(int sq, const BitboardBoard& board, Piece& piece);
	static uint8_t FindPiece(Piece piece, const BitboardBoard& board);
	static bool IsEmptyAt(int sq, const BitboardBoard& board);

	static int TotalPieces(const BitboardBoard& board);

	static bool IsPassedPawn(const Move move, Color movingColor, const BitboardBoard& board);
};