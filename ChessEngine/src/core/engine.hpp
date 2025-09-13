#pragma once

#include <unordered_map>
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

	void LoadPosition(std::string fen);

	void Update();
	void Render();

	void MakeMove(const Move move);
	void UndoMove();
	void UndoTurn();

	void SetBot(Bot* bot);
	void CheckKingInCheck(); // Sets checkStatus
	void CheckCheckmate();

	const Square(&GetBoard() const)[8][8]{ return board; }
	const Color GetCurrentPlayer() const { return GameState::currentPlayer; }
	const uint64_t GetZobristKey() { return zobristKey; }
	std::string GetFEN() const; // Get current position in FEN notation
	uint64_t ComputeFullHash() const;

	bool IsThreefold() const;
	inline const bool IsOver() const { return GameState::checkmate || GameState::draw; }
	inline const bool InCheck(Color color) { return (GameState::checkStatus & (color == Color::WHITE ? 0b10 : 0b01)) != 0; }
	inline const bool IsCheckmate(Color color) { return GameState::checkmate && InCheck(color); }

	int PieceToIndex(const Piece& p) const;
private:
	bool StoreMove(); // Returns if there was a second click to make a move
	void ProcessMove(Move& move); // Validates move
	bool ValidMove(const Piece piece, const Move move); // Checks if the move is valid for the piece

	void UpdateEndgameStatus();
	void UpdateCastlingRights(const Move move, const Piece movingPiece, const Piece targetPiece);
	void UpdateEnPassantSquare(const Move move);
	void AppendUndoList(BoardState state, const Move move);
	bool HandleSpecialNotation(); // Returns if there was notation or not

	inline void ChangePlayers() { GameState::currentPlayer = (GameState::currentPlayer == Color::WHITE) ? Color::BLACK : Color::WHITE; }
	

	Square board[8][8];
	GraphicsEngine graphics;

	Zobrist zobrist;
	uint64_t zobristKey = 0;

	// For 3 move rep
	std::unordered_map<uint64_t, int> positionCounts;
	std::vector<uint64_t> positionStack; // To know what to decrement on undo

	std::vector<Move> moveHistory;
	std::vector<BoardState> undoHistory;

	std::string notationMove; // Clicks are converted to notation
	std::pair<int, int> firstClick; // Not static variable for rendering purposes
	std::pair<int, int> whiteKingPos = { -1, -1 };
	std::pair<int, int> blackKingPos = { -1, -1 };

	Bot* bots[2] = { nullptr, nullptr };
	bool botPlaying[2] = { false, false };
};