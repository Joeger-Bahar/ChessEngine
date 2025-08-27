#pragma once

#include <vector>
#include <string>

#include "piece.hpp"
#include "square.hpp"
#include "move.hpp"

class Engine
{
public:
	Engine();
	~Engine();

	void RunTurn();

private:
	void Render();
	void StoreMove();
	void ProcessMove();
	bool ValidMove(const Piece piece);
	std::string GetFEN() const;
	inline void ChangePlayers() { currentPlayer = (currentPlayer == Color::WHITE) ? Color::BLACK : Color::WHITE; }

	std::vector<Move> moveHistory;
	Square board[8][8];
	std::string notationMove;
	Move move; // Current move being processed
	Color currentPlayer = Color::WHITE;
	Color checkStatus = Color::NONE; // NONE, WHITE, BLACK
	int enPassantTarget[2] = { -1, -1 }; // { row, column }, -1 if no target
	bool checkmate = false; // Can use check status for color
	bool invalidMove = false; // If the last move was invalid
	bool whiteCastlingRights[2] = { true, true }; // { queenside, kingside }
	bool blackCastlingRights[2] = { true, true }; // { queenside, kingside }
};