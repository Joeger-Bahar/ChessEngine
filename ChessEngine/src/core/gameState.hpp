#pragma once

#include "piece.hpp"

#include <cstdint>

namespace GameState
{
	inline bool uci = false;
	inline thread_local Color currentPlayer = Color::WHITE;
	inline thread_local int   checkStatus = 0;							 // 10 - white, 01 - black
	inline thread_local int   enPassantTarget = 0;						 // Index of ep square, 0 if no target
	inline thread_local int   halfmoves = 0;							 // Number of halfmoves since last capture or pawn move (for 50-move rule)
	inline thread_local bool  endgame = false;						     // If the game can be considered endgame
	inline thread_local bool  checkmate = false, draw = false;			 // Can use check status for color
	inline thread_local bool  invalidMove = false;						 // If the last move was invalid
	inline thread_local bool  whiteCastlingRights[2] = { true, true };	 // { queenside, kingside }
	inline thread_local bool  blackCastlingRights[2] = { true, true };
};

struct BoardState
{
	BoardState();
	uint64_t zobristKey;
	uint8_t capturedPiece; // 0-7 (PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONE)
	uint8_t movedPiece;
	uint8_t promotion; // NONE if no promotion
	uint8_t fromSquare;
	uint8_t toSquare;
	uint8_t enPassantTarget; // 1-63 for square, 0 = none
	uint8_t castlingRights; // 1000 = white queenside, 0100 = white kingside, 0010 = black queenside, 0001 = black kingside
	uint8_t checkStatus;
	uint8_t halfmoveClock;
	bool wasEnPassant;
	bool wasCastling;
	bool playerToMove; // true = white, false = black
};
