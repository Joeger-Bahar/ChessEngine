#pragma once

#include <chrono>
#include <vector>

#include "core/piece.hpp"
#include "core/boardCalculator.hpp"
#include "core/engine.hpp"
#include "graphics/graphicsEngine.hpp"
#include "core/tt.hpp"

class Bot
{
public:
	Bot(Engine* engine, Color color);
	Move GetMove();
	Move GetMoveUCI(int wtime, int btime);
	void SetColor(Color color);
	const Color GetColor() const { return botColor; }

private:
	int Search(int depth, Color maximizingColor, int alpha, int beta);
	int Qsearch(int alpha, int beta, Color maximizingPlayer);
	int ScoreMove(const Move move);
	void OrderMoves(std::vector<Move>& moves, Move firstMove = Move());

	TranspositionTable tt;
	Engine* engine;
	Color botColor;
	std::vector<Move> moveLists[32];
	// Start time of the search, used for time control
	std::chrono::time_point<std::chrono::steady_clock> startTime;
	int nodesSearched;
	int timePerTurn = 10; // In seconds
	bool quitEarly = false;
	bool uci = false;
};
