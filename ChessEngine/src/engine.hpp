#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <array>

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
	void InitializeBoard(std::string fen);
	void Render();
	bool StoreMove();
	void ProcessMove();
	bool HandleSpecialNotation();
	void HandleCastling();
	bool ValidMove(const Piece piece);
	std::string GetFEN() const;
	void UndoMove();
	inline void ChangePlayers() { currentPlayer = (currentPlayer == Color::WHITE) ? Color::BLACK : Color::WHITE; }
	void CheckKingInCheck();

	std::vector<Move> moveHistory;
	Square board[8][8];
	GraphicsEngine graphics;
	std::string notationMove;
	Move move; // Current move being processed
	std::pair<int, int> firstClick; // Not static variable for rendering purposes
	Color currentPlayer = Color::WHITE;
	Color checkStatus = Color::NONE; // NONE, WHITE, BLACK - who is in check
	int enPassantTarget[2] = { -1, -1 }; // { row, column }, -1 if no target
	int halfmoves = 0; // Number of halfmoves since last capture or pawn move (for 50-move rule)
	bool checkmate = false, draw = false; // Can use check status for color
	bool invalidMove = false; // If the last move was invalid
	bool whiteCastlingRights[2] = { true, true }; // { queenside, kingside }
	bool blackCastlingRights[2] = { true, true };
	std::pair<int, int> whiteKingPos = { -1, -1 };
	std::pair<int, int> blackKingPos = { -1, -1 };
};