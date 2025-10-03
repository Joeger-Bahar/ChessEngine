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
#include "bitboard.hpp"

class Bot;

class Engine
{
public:
	Engine();
	Engine(std::string fen);
	~Engine();
	void Reset();
	void LoadPosition(std::string fen);

	void Update();
	void Render();

	void MakeMove(const Move move);
	void UndoMove();
	void MakeNullMove();
	void UndoNullMove();
	void UndoTurn();

	void SetBot(Bot* bot);
	void CheckKingInCheck(); // Sets checkStatus
	void CheckCheckmate();

	const Square(&GetBoard() const)[64]{ return board; }
	const Color GetCurrentPlayer() const { return GameState::currentPlayer; }
	const uint64_t GetZobristKey() { return zobristKey; }
	std::string GetFEN() const; // Get current position in FEN notation
	uint64_t ComputeFullHash() const;

	bool IsDraw() const;
	bool IsThreefold() const;
	bool Is50Move() const;
	inline const bool IsOver() const { return GameState::checkmate || GameState::draw; }
	inline const bool InCheck(Color color) { return (GameState::checkStatus & (IsWhite(color) ? 0b10 : 0b01)) != 0; }
	inline const bool IsCheckmate(Color color) { return GameState::checkmate && InCheck(color); }

	int PieceToIndex(const Piece& p) const;
	bool ValidMove(const Piece piece, const Move move); // Checks if the move is valid for the piece
private:
	bool StoreMove(Move& move);   // Returns if there was a second click to make a move
	void ProcessMove(Move& move); // Validates move

	//void RemoveFromBitboards(const Piece& p, int sq);
	//void AddToBitboards(const Piece& p, int sq);

	void UpdateEndgameStatus();
	void UpdateCastlingRights(const Move move, const Piece movingPiece, const Piece targetPiece);
	void UpdateEnPassantSquare(const Move move);
	void AppendUndoList(BoardState state, const Move move);

	inline void ChangePlayers() { GameState::currentPlayer = Opponent(GameState::currentPlayer); }

	Square board[64];
	GraphicsEngine graphics;

	BitboardBoard bitboards;

	Zobrist zobrist;
	uint64_t zobristKey = 0;

	// For 3 move rep
	std::unordered_map<uint64_t, int> positionCounts;
	std::vector<uint64_t> positionStack; // To know what to decrement on undo

	std::vector<Move> moveHistory;
	std::vector<BoardState> undoHistory;

	int firstClick;			  // Not static variable for rendering purposes
	int whiteKingPos = -1;
	int blackKingPos = -1;

	Bot* bots[2] = { nullptr, nullptr };
	bool botPlaying[2] = { false, false };
};