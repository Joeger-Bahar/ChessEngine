#pragma once

#include "piece.hpp"

namespace GameState
{
	inline Color currentPlayer = Color::WHITE;
	inline Color checkStatus = Color::NONE;					 // NONE, WHITE, BLACK - who is in check
	inline int   enPassantTarget[2] = { -1, -1 };			 // { row, column }, -1 if no target
	inline int   halfmoves = 0;								 // Number of halfmoves since last capture or pawn move (for 50-move rule)
	inline bool  checkmate = false, draw = false;			 // Can use check status for color
	inline bool  invalidMove = false;						 // If the last move was invalid
	inline bool  whiteCastlingRights[2] = { true, true };	 // { queenside, kingside }
	inline bool  blackCastlingRights[2] = { true, true };
};

struct BoardState
{
	uint8_t capturedPiece : 3; // 0-7 (PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONE)
	uint8_t movedPiece : 3;
	uint8_t promotion : 3; // NONE if no promotion
	uint8_t fromSquare;
	uint8_t toSquare;
	uint8_t enPassantTarget; // 0-63 for square, 64 = none
	uint8_t castlingRights : 4; // 1000 = white queenside, 0100 = white kingside, 0010 = black queenside, 0001 = black kingside
	uint8_t halfmoveClock;
	bool wasEnPassant : 1;
	bool wasCastling  : 1;
	bool playerToMove : 1; // true = white, false = black
};

struct s
{
	int a : 1;
	int b : 1;
	int c : 1;
	int d : 1;
	int e : 1;
	int f : 1;
	int g : 1;
	int h : 1;
};