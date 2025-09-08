#include "bot.hpp"

#include "core/boardCalculator.hpp"

#include <limits>
#include <iostream>
#include <vector>
#include <algorithm>
#include <Windows.h>
#undef min
#undef max

Bot::Bot(Engine* engine, Color color)
{
	this->engine = engine;
	this->botColor = color;
}

int PieceValue(Piece piece)
{
	switch (piece.GetType())
	{
	case Pieces::PAWN:   return 100;
	case Pieces::KNIGHT: return 300;
	case Pieces::BISHOP: return 325;
	case Pieces::ROOK:   return 500;
	case Pieces::QUEEN:  return 900;
	case Pieces::KING:   return 10000; // not really captured, but for ordering
	default: return 0;
	}
}

int Bot::ScoreMove(const Move move)
{
	Piece moved = engine->GetBoard()[move.startRow][move.startCol].GetPiece();
	Piece captured = engine->GetBoard()[move.endRow][move.endCol].GetPiece();

	// MVV-LVA: Most Valuable Victim - Least Valuable Attacker
	if (captured.GetType() != Pieces::NONE)
		return 10000 + (PieceValue(captured) * 10 - PieceValue(moved));

	// TODO: Add heuristics for killer moves / history
	return 0;
}

// Orders moves in-place, best first
void Bot::OrderMoves(std::vector<Move>& moves)
{
	std::sort(moves.begin(), moves.end(),
		[&](const Move& a, const Move& b) {
			return ScoreMove(a) > ScoreMove(b);
		});
}

Move Bot::GetMove()
{
	int depth = 7;

	Move bestMove = Move();
	int alpha = std::numeric_limits<int>::min();
	int beta = std::numeric_limits<int>::max();

	std::vector<Move> moves = BoardCalculator::GetAllMoves(botColor, engine->GetBoard());
	OrderMoves(moves);

	if (botColor == Color::WHITE)
	{
		int bestScore = std::numeric_limits<int>::min();

		for (Move move : moves)
		{
			engine->MakeMove(move);

			engine->CheckKingInCheck();
			bool illegal = (GameState::checkStatus & ((botColor == Color::WHITE) ? 0b10 : 0b01));

			if (illegal)
			{
				engine->UndoMove();
				continue; // skip illegal move
			}

			int score = Search(depth - 1, Color::WHITE, alpha, beta);
			engine->UndoMove();

			if (score > bestScore)
			{
				bestScore = score;
				bestMove = move;
			}

			alpha = std::max(alpha, bestScore);
		}
	}
	else
	{
		int bestScore = std::numeric_limits<int>::max();

		for (Move move : moves)
		{
			engine->MakeMove(move);

			engine->CheckKingInCheck();
			bool illegal = (GameState::checkStatus & ((botColor == Color::WHITE) ? 0b10 : 0b01));

			if (illegal)
			{
				engine->UndoMove();
				continue; // skip illegal move
			}

			int score = Search(depth - 1, Color::WHITE, alpha, beta);
			engine->UndoMove();

			if (score < bestScore)
			{
				bestScore = score;
				bestMove = move;
			}

			beta = std::min(beta, bestScore);
		}
	}

	engine->MakeMove(bestMove);

	engine->CheckKingInCheck();
	bool illegal = (GameState::checkStatus & ((botColor == Color::WHITE) ? 0b10 : 0b01));

	if (illegal)
	{
		std::cout << engine->GetFEN() << "\n";
		engine->UndoMove();
		std::cout << "In check\n";
		return bestMove;
	}
	engine->UndoMove();

	return bestMove;
}

int Bot::Search(int depth, Color maximizingColor, int alpha, int beta)
{
	if (depth == 0 || engine->IsOver())
		return engine->Eval();

	Color currentColor = GameState::currentPlayer;

	std::vector<Move> moves = BoardCalculator::GetAllMoves(currentColor, engine->GetBoard());
	OrderMoves(moves);

	if (currentColor == maximizingColor)
	{
		int maxEval = std::numeric_limits<int>::min();

		for (Move move : moves)
		{
			engine->MakeMove(move);

			engine->CheckKingInCheck();
			bool illegal = (GameState::checkStatus & ((currentColor == Color::WHITE) ? 0b10 : 0b01));

			if (illegal)
			{
				engine->UndoMove();
				continue; // skip illegal move
			}

			int eval = Search(depth - 1, maximizingColor, alpha, beta);
			engine->UndoMove();

			maxEval = std::max(maxEval, eval);
			alpha = std::max(alpha, eval);

			if (beta <= alpha)
				break; // Prune
		}
		return maxEval;
	}
	else
	{
		int minEval = std::numeric_limits<int>::max();

		for (Move move : moves)
		{
			engine->MakeMove(move);

			engine->CheckKingInCheck();
			bool illegal = (GameState::checkStatus & ((currentColor == Color::WHITE) ? 0b10 : 0b01));

			if (illegal)
			{
				engine->UndoMove();
				continue; // skip illegal move
			}

			int eval = Search(depth - 1, maximizingColor, alpha, beta);
			engine->UndoMove();

			minEval = std::min(minEval, eval);
			beta = std::min(beta, eval);

			if (beta <= alpha)
				break; // Prune
		}
		return minEval;
	}
}
