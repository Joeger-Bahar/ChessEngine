#include "bot.hpp"

#include "core/boardCalculator.hpp"

#include <limits>
#include <iostream>
#include <vector>
#include <Windows.h>
#undef min
#undef max

Bot::Bot(Engine* engine, Color color)
{
	this->engine = engine;
	this->botColor = color;
}

Move Bot::GetMove()
{
	int depth = 4;

	Move bestMove = Move();
	if (botColor == Color::WHITE)
	{
		int bestScore = std::numeric_limits<int>::min();
		std::vector<Move> moves = BoardCalculator::GetAllMoves(Color::WHITE, engine->GetBoard());

		for (Move move : moves)
		{
			engine->MakeMove(move);
			int score = Search(depth - 1, Color::WHITE);
			engine->UndoMove();
			if (score > bestScore)
			{
				bestScore = score;
				bestMove = move;
			}
		}
	}
	else
	{
		int bestScore = std::numeric_limits<int>::max();
		std::vector<Move> moves = BoardCalculator::GetAllMoves(Color::BLACK, engine->GetBoard());

		for (Move move : moves)
		{
			engine->MakeMove(move);
			int score = Search(depth - 1, Color::WHITE);
			engine->UndoMove();
			if (score < bestScore)
			{
				bestScore = score;
				bestMove = move;
			}
		}
	}

	return bestMove;
}

int Bot::Search(int depth, Color maximizingColor)
{
	if (depth == 0 || engine->IsOver())
		return engine->Eval();

	Color currentColor = GameState::currentPlayer;

	if (currentColor == maximizingColor)
	{
		int maxEval = std::numeric_limits<int>::min();
		std::vector<Move> moves = BoardCalculator::GetAllMoves(currentColor, engine->GetBoard());

		for (Move move : moves)
		{
			engine->MakeMove(move);
			int eval = Search(depth - 1, maximizingColor);
			engine->UndoMove();
			maxEval = std::max(maxEval, eval);
		}
		return maxEval;
	}
	else
	{
		int minEval = std::numeric_limits<int>::max();
		std::vector<Move> moves = BoardCalculator::GetAllMoves(currentColor, engine->GetBoard());

		for (Move move : moves)
		{
			engine->MakeMove(move);
			int eval = Search(depth - 1, maximizingColor);
			engine->UndoMove();
			minEval = std::min(minEval, eval);
		}
		return minEval;
	}
}
