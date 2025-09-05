//#include "gameState.hpp"
//
//Color currentPlayer = Color::WHITE;
//Color checkStatus = Color::NONE; // NONE, WHITE, BLACK - who is in check
//int enPassantTarget[2] = { -1, -1 }; // { row, column }, -1 if no target
//int halfmoves = 0; // Number of halfmoves since last capture or pawn move (for 50-move rule)
//bool checkmate = false, draw = false; // Can use check status for color
//bool invalidMove = false; // If the last move was invalid
//bool whiteCastlingRights[2] = { true, true }; // { queenside, kingside }
//bool blackCastlingRights[2] = { true, true };
//bool moveWasEnPassant = false;