#pragma once

#include <vector>
#include <array>

#include "piece.hpp"
#include "square.hpp"
#include "move.hpp"

class BoardCalculator
{
public:
	static bool IsSquareAttacked(int row, int col, Color byColor, const Square board[8][8]);
	static bool InBounds(int row, int col);
	static std::vector<uint8_t> GetAttackedSquares(Color color, const Square board[8][8]);
	static std::vector<uint8_t> GetValidMoves(int row, int col, const Square board[8][8]);
	static std::vector<uint8_t> GetAllMoves(Color color, const Square board[8][8]);

private:
	// These were generated with AI (ChatGPT 5)
	static void AddKingAttacks(int row, int col, std::array<bool, 64>& attacked);
	static void AddPawnAttacks(int row, int col, Color color, std::array<bool, 64>& attacked);
	static void AddKnightAttacks(int row, int col, std::array<bool, 64>& attacked);
	static void AddSlidingAttacks(int row, int col, std::array<bool, 64>& attacked, const int dirs[][2], int numDirs,
		const Square board[8][8]);

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
};