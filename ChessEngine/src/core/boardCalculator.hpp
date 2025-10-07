#pragma once

#include <vector>
#include <array>
#include <map>

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
	static bool IsSquareAttacked(int sq, Color byColor, const BitboardBoard& board);
	static bool IsCastlingValid(bool kingside, const BitboardBoard& board);
	
	// This gets moves for a piece, so it checks for checks as well
	static std::vector<uint8_t> GetValidMoves(int sq, const BitboardBoard& board);

	// This is pseudo-legal moves, it does not check for checks for faster engine calculations
	static std::vector<Move> GetAllLegalMoves(Color color, const BitboardBoard& board, class Engine* engine);
	static void GetAllMoves(std::vector<Move>& moves, Color color, const BitboardBoard& board, Engine* engine,
		bool onlyCaptures = false);
	static std::vector<Move> GetAllMoves(Color color, const BitboardBoard& board, Engine* engine, bool onlyCaptures = false);

	static bool GetPieceAt(int sq, const BitboardBoard& board, Piece& piece);
	static uint8_t FindPiece(Piece piece, const BitboardBoard& board);
	static bool IsEmptyAt(int sq, const BitboardBoard& board);

	//static std::vector<uint8_t> GetAttackedSquares(Color color, const Square board[8][8]);
	static void InitPrecomputedAttacks();
private:

	// Functions to get moves (instead of attacks)
	static Bitboard KingMoves(int sq, Color color, const BitboardBoard& board);
	static Bitboard PawnMoves(int sq, Color color, const BitboardBoard& board);
	static Bitboard KnightMoves(int sq, Color color, const BitboardBoard& board);
	static Bitboard SlidingMoves(int sq, Color color, const int dirs[], int numDirs, const BitboardBoard& board, bool includeBlockers = false);

	// Direction and offset tables
	static const int knightOffsets[8];
	static const int kingOffsets[8];
	static const int bishopDirs[4];
	static const int rookDirs[4];
	static const int queenDirs[8];
};