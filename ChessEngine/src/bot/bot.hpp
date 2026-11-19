#pragma once

#include <chrono>
#include <vector>

#include "opening.hpp"

#include "core/piece.hpp"
#include "core/boardCalculator.hpp"
#include "core/engine.hpp"
#include "core/tt.hpp"
#include "core/eval.hpp"
#include "graphics/graphicsEngine.hpp"

#define MAX_PLY 128
#define NUM_PIECES 6

struct SearchStackEntry
{
	Move move = 0;
	int pieceIndex = -1; // color*6 + (type-1), -1 for null move
	int staticEval = 0;
	bool inCheck = false;
	bool nullMove = false;
};

class Bot
{
public:
	Bot(Engine* engine, Color color);
	Move GetMove();
	Move GetMoveUCI(int timeForMove);
	void SetColor(Color color);
	const Color GetColor() const { return botColor; }

	void Clear();

private:
	bool SEE(Move move, int threshold);

	int Search(int depth, int ply, int alpha, int beta);
	int Qsearch(int alpha, int beta, int ply, int qply = 0);
	int ScoreMove(const Move move, int ply, bool onlyMVVLVA);
	void OrderMoves(std::vector<Move>& moves, int ply, bool onlyMVVLVA, Move firstMove = Move());

	TranspositionTable tt;
	// [color][pieces][start square][end square]
	int historyHeuristic[2][NUM_PIECES][64][64] = { 0 };
	Engine* engine;
	Color botColor;
	
	// Continuation history
	std::vector<int16_t> contHistory;
	std::vector<Move> moveLists[MAX_PLY];
	std::vector<Move> qMoveLists[MAX_PLY];
	SearchStackEntry searchStack[MAX_PLY + 8];
	Move previousMoves[MAX_PLY] = {};
	Move killerMoves[MAX_PLY][2];
	Move counterMoves[2][64][64];
	int lmrTable[64][64];
	// Start time of the search, used for time control
	std::chrono::time_point<std::chrono::steady_clock> startTime;
	int nodesSearched;
	int rootDepth = 0;
	int timePerTurn = 12000; // In milliseconds
	bool quitEarly = false;
};
