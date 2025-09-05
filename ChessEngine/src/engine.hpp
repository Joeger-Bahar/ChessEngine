#pragma once

#include <vector>
#include <string>
//#include <chrono>

#include "piece.hpp"
#include "square.hpp"
#include "move.hpp"
#include "graphics/graphicsEngine.hpp"
#include "boardCalculator.hpp"
#include "gameState.hpp"

class Engine
{
public:
	Engine();
	Engine(std::string fen);
	~Engine();

	void Update();
	bool IsOver();
	//void Benchmark();

private:
	void LoadPosition(std::string fen);
	void Render();
	bool StoreMove(); // Returns if there was a second click to make a move
	void ProcessMove();
	bool HandleSpecialNotation(); // Returns if there was notation or not
	void HandleCastling();
	bool ValidMove(const Piece piece); // Checks if the move is valid for the piece
	std::string GetFEN() const; // Get current position in FEN notation
	void UndoMove();
	void CheckKingInCheck(); // Sets checkStatus
	inline void ChangePlayers() { GameState::currentPlayer = (GameState::currentPlayer == Color::WHITE) ? Color::BLACK : Color::WHITE; }

	std::vector<Move> moveHistory;
	Square board[8][8];
	GraphicsEngine graphics;
	std::string notationMove; // Clicks are converted to notation
	Move move; // Current move being processed
	std::pair<int, int> firstClick; // Not static variable for rendering purposes
	std::pair<int, int> whiteKingPos = { -1, -1 };
	std::pair<int, int> blackKingPos = { -1, -1 };
};