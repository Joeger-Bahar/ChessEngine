#include "gameState.hpp"

BoardState::BoardState()
	: capturedPiece(0), movedPiece(0), promotion(0), fromSquare(0), toSquare(0),
	enPassantTarget(64), castlingRights(0), halfmoveClock(0),
	wasEnPassant(false), wasCastling(false), playerToMove(true)
{
}
