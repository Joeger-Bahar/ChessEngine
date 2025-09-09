#pragma once

#include <vector>
#include <array>

#include "piece.hpp"
#include "square.hpp"
#include "move.hpp"

#define Opponent(c) ((c) == Color::WHITE ? Color::BLACK : Color::WHITE)

class BoardCalculator
{
public:
	static bool IsSquareAttacked(int row, int col, Color byColor, const Square board[8][8]);
	static bool IsCastlingValid(bool kingside, const Square board[8][8]);
	static bool InBounds(int row, int col);
	// This gets moves for a piece, so it checks for checks as well
	static std::vector<uint8_t> GetValidMoves(int row, int col, const Square board[8][8]);
	// This is pseudo-legal moves, it does not check for checks for faster engine calculations
	static std::vector<Move> GetAllLegalMoves(Color color, const Square board[8][8]);
	static std::vector<Move> GetAllMoves(Color color, const Square board[8][8]);
	static uint8_t FindPiece(Piece piece, const Square board[8][8]);


	//static std::vector<uint8_t> GetAttackedSquares(Color color, const Square board[8][8]);
private:

	// Functions to get moves (instead of attacks)
	static void AddKingMoves(int row, int col, Color color, std::array<bool, 64>& moves, const Square board[8][8]);
	static void AddPawnMoves(int row, int col, Color color, std::array<bool, 64>& moves, const Square board[8][8]);
	static void AddKnightMoves(int row, int col, Color color, std::array<bool, 64>& moves, const Square board[8][8]);
	static void AddSlidingMoves(int row, int col, Color color, std::array<bool, 64>& moves, const int dirs[][2], int numDirs,
		const Square board[8][8]);

	// Direction and offset tables
	static const int knightOffsets[8][2];
	static const int kingOffsets[8][2];
	static const int bishopDirs[4][2];
	static const int rookDirs[4][2];
	static const int queenDirs[8][2];

	// These were generated with AI (ChatGPT 5)
	//static void AddKingAttacks(int row, int col, std::array<bool, 64>& attacked);
	//static void AddPawnAttacks(int row, int col, Color color, std::array<bool, 64>& attacked);
	//static void AddKnightAttacks(int row, int col, std::array<bool, 64>& attacked);
	//static void AddSlidingAttacks(int row, int col, std::array<bool, 64>& attacked, const int dirs[][2], int numDirs,
	//	const Square board[8][8]);
};