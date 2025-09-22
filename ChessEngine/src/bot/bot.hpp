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

class Bot
{
public:
	Bot(Engine* engine, Color color);
	Move GetMove();
	Move GetMoveUCI(int wtime, int btime);
	void SetColor(Color color);
	const Color GetColor() const { return botColor; }

	void Clear();

private:
	int Search(int depth, int ply, int alpha, int beta);
	int Qsearch(int alpha, int beta, int ply);
	int ScoreMove(const Move move, int ply, bool onlyMVVLVA);
	void OrderMoves(std::vector<Move>& moves, int ply, bool onlyMVVLVA, Move firstMove = Move());

	TranspositionTable tt;
	// [color][pieces][start square][end square]
	int historyHeuristic[2][NUM_PIECES][64][64] = { 0 };
	Engine* engine;
	Color botColor;
	std::vector<Move> moveLists[MAX_PLY];
	//Move killerMoves[MAX_PLY][2];
	// Start time of the search, used for time control
	std::chrono::time_point<std::chrono::steady_clock> startTime;
	int nodesSearched;
	int timePerTurn = 500; // In milliseconds
	bool quitEarly = false;
	bool uci = false;
};
