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