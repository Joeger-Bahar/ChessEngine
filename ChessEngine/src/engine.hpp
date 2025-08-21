#pragma once

#include <string>

#include "piece.hpp"
#include "square.hpp"
#include "move.hpp"

class Engine
{
public:
	Engine();
	~Engine();

	void Render();
	void StoreMove();
	bool ProcessMove();
	inline void ChangePlayers() { currentPlayer = (currentPlayer == Color::WHITE) ? Color::BLACK : Color::WHITE; }

private:
	const char* GetFEN() const;

	Square board[8][8];
	std::string notationMove;
	Move move; // Current move being processed
	Color currentPlayer = Color::WHITE;
	Color checkStatus = Color::NONE; // NONE, WHITE, BLACK
	bool checkmate = false; // Can use check status for color
	bool invalidMove = false; // If the last move was invalid
};