#pragma once

#include "piece.hpp"

#include <cstdint>

// TODO: Can make enPassant target only record col + 1 bit for whether there was one (use last player for row)
namespace GameState
{
	inline bool uci = false;
	inline Color currentPlayer = Color::WHITE;
	inline int   checkStatus = 0;							 // 10 - white, 01 - black
	inline int   enPassantTarget = 0;						 // Index of ep square, 0 if no target
	inline int   halfmoves = 0;								 // Number of halfmoves since last capture or pawn move (for 50-move rule)
	inline bool	 endgame = false;						 // If the game can be considered endgame
	inline bool  checkmate = false, draw = false;			 // Can use check status for color
	inline bool  invalidMove = false;						 // If the last move was invalid
	inline bool  whiteCastlingRights[2] = { true, true };	 // { queenside, kingside }
	inline bool  blackCastlingRights[2] = { true, true };
};

struct BoardState
{
	BoardState();
	uint64_t zobristKey;
	uint8_t capturedPiece : 3; // 0-7 (PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONE)
	uint8_t movedPiece : 3;
	uint8_t promotion : 3; // NONE if no promotion
	uint8_t fromSquare;
	uint8_t toSquare;
	uint8_t enPassantTarget; // 1-63 for square, 0 = none
	uint8_t castlingRights : 4; // 1000 = white queenside, 0100 = white kingside, 0010 = black queenside, 0001 = black kingside
	uint8_t halfmoveClock;
	bool wasEnPassant : 1;
	bool wasCastling  : 1;
	bool playerToMove : 1; // true = white, false = black
};
