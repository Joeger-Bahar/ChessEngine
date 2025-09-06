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
	Piece capturedPiece;
	Piece movedPiece;
	Piece promotion; // NONE if no promotion
	uint8_t fromSquare;
	uint8_t toSquare;
	uint8_t enPassantTarget; // 0-63 for square, 64 = none
	uint8_t castlingRights : 4; // 1000 = white queenside, 0100 = white kingside, 0010 = black queenside, 0001 = black kingside
	uint8_t halfmoveClock;
	bool wasEnPassant;
	bool wasCastling;
	bool playerToMove; // true = white, false = black
};