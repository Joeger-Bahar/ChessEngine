#pragma once

#include <vector>
#include <array>
#include <map>

#include "piece.hpp"
#include "square.hpp"
#include "move.hpp"

#define Opponent(c) ((c) == Color::WHITE ? Color::BLACK : Color::WHITE)
#define IsWhite(c)  (c == Color::WHITE)
#define IsBlack(c)  (!IsWhite(c))

#define ToIndex(row, col) (((row) << 3) | (col))
#define ToRow(idx) ((idx) >> 3)   // 0 = rank 8, 7 = rank 1 ( / 8)
#define ToCol(idx) ((idx) & 7)    // 0 = file a, 7 = file h ( % 8)
#define InBounds(idx) ((idx) >= 0 && (idx) < 64)

class BoardCalculator
{
public:
	static bool IsSquareAttacked(int sq, Color byColor, const Square board[64]);
	static bool IsCastlingValid(bool kingside, const Square board[64]);
	
	// This gets moves for a piece, so it checks for checks as well
	static std::vector<uint8_t> GetValidMoves(int sq, const Square board[64]);

	// This is pseudo-legal moves, it does not check for checks for faster engine calculations
	static std::vector<Move> GetAllLegalMoves(Color color, const Square board[64], class Engine* engine);
	static void GetAllMoves(std::vector<Move>& moves, Color color, const Square board[64], Engine* engine,
		bool onlyCaptures = false);
	static std::vector<Move> GetAllMoves(Color color, const Square board[64], Engine* engine, bool onlyCaptures = false);

	static uint8_t FindPiece(Piece piece, const Square board[64]);

	//static std::vector<uint8_t> GetAttackedSquares(Color color, const Square board[8][8]);
private:
	// Functions to get moves (instead of attacks)
	static void AddKingMoves(int sq, Color color, std::array<bool, 64>& moves, const Square board[64]);
	static void AddPawnMoves(int sq, Color color, std::array<bool, 64>& moves, const Square board[64]);
	static void AddKnightMoves(int sq, Color color, std::array<bool, 64>& moves, const Square board[64]);
	static void AddSlidingMoves(int sq, Color color, std::array<bool, 64>& moves, const int dirs[], int numDirs,
		const Square board[64]);

	// Direction and offset tables
	static const int knightOffsets[8];
	static const int kingOffsets[8];
	static const int bishopDirs[4];
	static const int rookDirs[4];
	static const int queenDirs[8];
};