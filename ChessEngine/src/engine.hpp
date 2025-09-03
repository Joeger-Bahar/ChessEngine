#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <array>

#include "piece.hpp"
#include "square.hpp"
#include "move.hpp"
#include "graphics/graphicsEngine.hpp"

class Engine
{
public:
	Engine();
	~Engine();

	void Update();
	//void Benchmark();

private:
	void Render();
	bool StoreMove();
	void ProcessMove();
	bool ValidMove(const Piece piece);
	bool IsSquareAttacked(int row, int col, Color byColor);
	//std::vector<uint8_t> GetAttackedSquares(Color color); // Returns a list of squares (0-63) attacked by the given color
	std::string GetFEN() const;
	void UndoMove();
	inline void ChangePlayers() { currentPlayer = (currentPlayer == Color::WHITE) ? Color::BLACK : Color::WHITE; }
	bool KingInCheck(Color color);

	// These were generated with AI (ChatGPT 5)
	void AddKnightAttacks(int row, int col, std::array<bool, 64>& attacked);
	void AddSlidingAttacks(int row, int col, std::array<bool, 64>& attacked, const std::vector<std::pair<int, int>>& dirs);
	void AddPawnAttacks(int row, int col, Color color, std::array<bool, 64>& attacked);
	void AddKingAttacks(int row, int col, std::array<bool, 64>& attacked);

	std::vector<Move> moveHistory;
	Square board[8][8];
	GraphicsEngine graphics;
	std::string notationMove;
	Move move; // Current move being processed
	std::pair<int, int> firstClick; // Not static variable for rendering purposes
	Color currentPlayer = Color::WHITE;
	Color checkStatus = Color::NONE; // NONE, WHITE, BLACK
	int enPassantTarget[2] = { -1, -1 }; // { row, column }, -1 if no target
	bool checkmate = false, draw = false; // Can use check status for color
	bool invalidMove = false; // If the last move was invalid
	bool whiteCastlingRights[2] = { true, true }; // { queenside, kingside }
	bool blackCastlingRights[2] = { true, true };
};