#include "bot.hpp"

#include <iostream>
#include <random>

std::mt19937 rng(std::random_device{}());
std::uniform_int_distribution<int> dist(-100, 100);

Bot::Bot(Engine* engine, Color color)
{
	this->engine = engine;
	this->botColor = color;
	srand(time(0));
}

Move Bot::GetMove()
{
	Move bestMove;
	int bestEval = -100000;

	std::vector<Move> moves = BoardCalculator::GetAllMoves(engine->GetCurrentPlayer(), engine->GetBoard());
	for (const Move& move : moves)
	{
		engine->MakeMove(move);

		engine->CheckKingInCheck();
		if (engine->InCheck((engine->GetCurrentPlayer() == Color::WHITE) ? Color::BLACK : Color::WHITE))
		{
			engine->UndoMove();
			continue; // Illegal move, try next
		}

		int eval = Search(3, false, -100000, 100000);
		engine->UndoMove();

		if (eval > bestEval)
		{
			bestEval = eval;
			bestMove = move;
		}
	}

	return bestMove;
}

int Bot::Search(int depth, bool maximizingPlayer, int alpha, int beta)
{
	if (depth == 0 || engine->IsOver())
		return Eval();
	

	int bestEval = maximizingPlayer ? -100000 : 100000;

	std::vector<Move> moves = BoardCalculator::GetAllMoves(engine->GetCurrentPlayer(), engine->GetBoard());
	for (const Move& move : moves)
	{
		engine->MakeMove(move);

		engine->CheckKingInCheck();
		if (engine->InCheck((engine->GetCurrentPlayer() == Color::WHITE) ? Color::BLACK : Color::WHITE))
		{
			engine->UndoMove();
			continue; // Illegal move, try next
		}

		int eval = Search(depth - 1, !maximizingPlayer, alpha, beta);
		engine->UndoMove();

		if (maximizingPlayer)
		{
			bestEval = std::max(bestEval, eval);
			alpha = std::max(alpha, eval);
		}
		else
		{
			bestEval = std::min(bestEval, eval);
			beta = std::min(beta, eval);
		}
		if (beta <= alpha)
			break; // Alpha-beta pruning
	}

	return bestEval;
}

int Bot::Eval()
{
	return dist(rng); // Placeholder random evaluation between -100 and 100
}

