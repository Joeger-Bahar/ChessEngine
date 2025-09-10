#pragma once

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
	const Color GetColor() const { return botColor; }

private:
	int Search(int depth, Color maximizingColor, int alpha, int beta);
	int Qsearch(int depth, int alpha, int beta, Color maximizingPlayer);
	int ScoreMove(const Move move);
	void OrderMoves(std::vector<Move>& moves);

	TranspositionTable tt;
	Engine* engine;
	Color botColor;
	int nodesSearched;
};
