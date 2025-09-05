#pragma once

#include <vector>
#include <string>
//#include <chrono>

#include "piece.hpp"
#include "square.hpp"
#include "move.hpp"
#include "graphics/graphicsEngine.hpp"
#include "boardCalculator.hpp"

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
	inline void ChangePlayers() { currentPlayer = (currentPlayer == Color::WHITE) ? Color::BLACK : Color::WHITE; }

	std::vector<Move> moveHistory;
	Square board[8][8];
	GraphicsEngine graphics;
	std::string notationMove; // Clicks are converted to notation
	Move move; // Current move being processed
	std::pair<int, int> firstClick; // Not static variable for rendering purposes
	// From here
	Color currentPlayer = Color::WHITE;
	Color checkStatus = Color::NONE; // NONE, WHITE, BLACK - who is in check
	int enPassantTarget[2] = { -1, -1 }; // { row, column }, -1 if no target
	int halfmoves = 0; // Number of halfmoves since last capture or pawn move (for 50-move rule)
	bool checkmate = false, draw = false; // Can use check status for color
	bool invalidMove = false; // If the last move was invalid
	bool whiteCastlingRights[2] = { true, true }; // { queenside, kingside }
	bool blackCastlingRights[2] = { true, true };
	// To here
	// Could be in a gameState struct
	std::pair<int, int> whiteKingPos = { -1, -1 };
	std::pair<int, int> blackKingPos = { -1, -1 };
};