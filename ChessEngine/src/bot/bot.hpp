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
	Move TTGetMove();
	Move GetMove();
	const Color GetColor() const { return botColor; }

private:
	int TTSearch(int depth, Color maximizingColor, int alpha, int beta);
	int Search(int depth, Color maximizingColor, int alpha, int beta);
	int ScoreMove(const Move move);
	void OrderMoves(std::vector<Move>& moves);

	TranspositionTable tt;
	Engine* engine;
	Color botColor;
	int nodesSearched;
};
