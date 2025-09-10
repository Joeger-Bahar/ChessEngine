#pragma once

#include <vector>
#include <string>

#include "piece.hpp"
#include "square.hpp"
#include "move.hpp"
#include "boardCalculator.hpp"
#include "gameState.hpp"
#include "zobrist.hpp"

#include "graphics/graphicsEngine.hpp"

class Bot;

class Engine
{
public:
	Engine();
	Engine(std::string fen);
	~Engine();

	void Update();
	void Render();
	void MakeMove(const Move move);
	void UndoMove();
	void UndoTurn();
	void SetBot(Bot* bot);
	void CheckKingInCheck(); // Sets checkStatus
	void CheckCheckmate();
	const bool IsOver() const { return GameState::checkmate || GameState::draw; }
	inline bool InCheck(Color color) { return (GameState::checkStatus & (color == Color::WHITE ? 0b10 : 0b01)) != 0; }
	inline bool IsCheckmate(Color color) { return GameState::checkmate && InCheck(color); }
	const Square(&GetBoard() const)[8][8]{ return board; }
	const Color GetCurrentPlayer() const { return GameState::currentPlayer; }
	const uint64_t GetZobristKey() { return zobristKey; }
	int Eval(); // Simple evaluation function for bot
	std::string GetFEN() const; // Get current position in FEN notation
	//void Benchmark();

	GraphicsEngine graphics;
private:
	void UpdateEndgameStatus();
	int PieceToIndex(const Piece& p) const;
	uint64_t ComputeFullHash() const;
	bool StoreMove(); // Returns if there was a second click to make a move
	void ProcessMove(Move& move); // Validates move
	bool HandleSpecialNotation(); // Returns if there was notation or not
	bool ValidMove(const Piece piece, const Move move); // Checks if the move is valid for the piece

	void UpdateCastlingRights(const Move move, const Piece movingPiece, const Piece targetPiece);
	void UpdateEnPassantSquare(const Move move);
	void AppendUndoList(BoardState state, const Move move);

	void LoadPosition(std::string fen);
	inline void ChangePlayers() { GameState::currentPlayer = (GameState::currentPlayer == Color::WHITE) ? Color::BLACK : Color::WHITE; }

	Zobrist zobrist;
	uint64_t zobristKey = 0;
	Square board[8][8];
	std::vector<Move> moveHistory;
	std::vector<BoardState> undoHistory;
	std::string notationMove; // Clicks are converted to notation
	std::pair<int, int> firstClick; // Not static variable for rendering purposes
	std::pair<int, int> whiteKingPos = { -1, -1 };
	std::pair<int, int> blackKingPos = { -1, -1 };
	Bot* bot = nullptr;
	bool botPlaying = false;
};