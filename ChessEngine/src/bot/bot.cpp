#include "bot.hpp"

#include "core/boardCalculator.hpp"

#include <limits>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <Windows.h>
#undef min
#undef max
using namespace std::chrono;

uint32_t PackMove(const Move& m)
{
	uint32_t s = m.startRow * 8 + m.startCol;
	uint32_t t = m.endRow * 8 + m.endCol;
	// TODO: This is an incorrect packing of promotion
	uint32_t promo = (m.promotion ? (m.promotion & 0x7) : 0); // user-defined
	return (s) | (t << 6) | (promo << 12) | ((m.wasEnPassant ? 1u : 0u) << 15) | ((m.wasCastle ? 1u : 0u) << 16);
}

Move UnpackMove(uint32_t code)
{
	Move m;
	uint32_t s = code & 0x3F;
	uint32_t t = (code >> 6) & 0x3F;
	m.startRow = s / 8; m.startCol = s % 8;
	m.endRow = t / 8;   m.endCol = t % 8;
	m.promotion = (uint8_t)((code >> 12) & 0x7);
	m.wasEnPassant = ((code >> 15) & 1);
	m.wasCastle = ((code >> 16) & 1);
	return m;
}


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
	case Pieces::KING:   return 10000; // Not really captured, but for ordering
	default: return 0;
	}
}

int Bot::ScoreMove(const Move move)
{
	Piece moved = engine->GetBoard()[move.startRow][move.startCol].GetPiece();
	Piece captured = engine->GetBoard()[move.endRow][move.endCol].GetPiece();

	// MVV-LVA: Most Valuable Victim - Least Valuable Attacker
	if (captured.GetType() != Pieces::NONE)
		return (PieceValue(captured) * 10 - PieceValue(moved));

	// TODO: Add heuristics for killer moves / history
	return 0;
}

// Orders moves in-place, best first
void Bot::OrderMoves(std::vector<Move>& moves, Move firstMove)
{
	std::sort(moves.begin(), moves.end(),
		[&](const Move& a, const Move& b) {
			return ScoreMove(a) > ScoreMove(b);
		});

	if (!firstMove.IsNull())
	{
		// Insert at front
		moves.insert(moves.begin(), firstMove);

		// Remove move if it was already in
		auto it = std::find(moves.begin() + 1, moves.end(), firstMove);
		if (it != moves.end()) {
			moves.erase(it);
		}
	}
}

void Bot::SetColor(Color color)
{
	this->botColor = color;
}

Move Bot::GetMoveUCI(int wtime, int btime)
{
	uci = true;
	int time_for_move;
	if (botColor == Color::WHITE) {
		time_for_move = wtime / 20;
	} else {
		time_for_move = btime / 20;
	}
	timePerTurn = 500;
	return GetMove();
}

Move Bot::GetMove()
{
	quitEarly = false;
	startTime = steady_clock::now();
	tt.NewSearch(); // age TT entries for this root search

	int maxDepth = 8;
	Move bestMove = Move();
	int bestScore = 0;

	for (int depth = 1; depth <= maxDepth; ++depth)
	{
		int alpha = std::numeric_limits<int>::min();
		int beta = std::numeric_limits<int>::max();
		bool foundLegal = false;

		Move currentBestMove = Move();
		int currentBestScore;

		std::vector<Move> moves = BoardCalculator::GetAllMoves(botColor, engine->GetBoard());
		if (!bestMove.IsNull()) // Current best move first to help with pruning
			OrderMoves(moves, bestMove);
		else
			OrderMoves(moves);

		bool isWhite = botColor == Color::WHITE;

		if (isWhite) currentBestScore = std::numeric_limits<int>::min();
		else         currentBestScore = std::numeric_limits<int>::max();

		for (Move move : moves)
		{
			engine->MakeMove(move);

			if (engine->InCheck(botColor)) { engine->UndoMove(); continue; }

			foundLegal = true;

			int score = Search(depth - 1, Color::WHITE, alpha, beta);
			engine->UndoMove();

			if (quitEarly)
				break;

			auto elapsed = duration_cast<milliseconds>(steady_clock::now() - startTime).count();
			if (elapsed >= timePerTurn)
			{
				quitEarly = true;
				break;
			}

			if (isWhite)
			{
				if (score > currentBestScore)
				{
					currentBestScore = score;
					currentBestMove = move;
				}
			}
			else
			{
				if (score < currentBestScore)
				{
					currentBestScore = score;
					currentBestMove = move;
				}
			}

			if (isWhite) alpha = std::max(alpha, currentBestScore);
			else		 beta  = std::min(beta, currentBestScore);
		}

		if (foundLegal)
		{
			if (quitEarly)
			{
				if (isWhite)
				{
					if (currentBestScore > bestScore)
					{
						bestScore = currentBestScore;
						bestMove = currentBestMove;
					}
				}
				else
				{
					if (currentBestScore < bestScore)
					{
						bestScore = currentBestScore;
						bestMove = currentBestMove;
					}
				}
			}
			else
			{
				bestMove = currentBestMove;
				bestScore = currentBestScore;
			}
		}

		if (quitEarly)
			break;

		// Extradite mate
		// Cast to avoid integer overflow
		if (std::abs((long long)bestScore) > 2000000000)
			break;

		// Keep searching until limit
		auto elapsed = duration_cast<milliseconds>(steady_clock::now() - startTime).count();
		if (depth == maxDepth)
		{
			if (elapsed < timePerTurn)
				++maxDepth;
		}
		// Stop searching if time limit is reached
		if (elapsed >= timePerTurn)
			break;
	}

	nodesSearched = 0;
	return bestMove;
}

int Bot::Search(int depth, Color maximizingColor, int alpha, int beta)
{
	nodesSearched++;
	// Check time every 1028 nodes (& faster than %)
	if ((nodesSearched & 0x1028) == 0)
	{
		auto elapsed = duration_cast<milliseconds>(steady_clock::now() - startTime).count();
		if (elapsed >= timePerTurn)
		{
			quitEarly = true;
			return 0;
		}
	}

	if (engine->IsThreefold())
		return 0; // Draw by repetition

	if (depth == 0 || engine->IsOver())
	{
		return Qsearch(alpha, beta, maximizingColor);
		//return engine->Eval();
	}

	uint64_t key = engine->GetZobristKey();
	int ttScore;
	uint32_t ttMove;
	if (tt.ttProbe(key, depth, alpha, beta, ttScore, ttMove))
		return ttScore;

	Color movingColor = GameState::currentPlayer;
	bool foundLegal = false;

	std::vector<Move>& moves = moveLists[depth];
	BoardCalculator::GetAllMoves(moves, movingColor, engine->GetBoard());
	OrderMoves(moves);

	// If we probed a move from TT, move it to the front
	if (ttMove)
	{
		Move m = UnpackMove(ttMove);
		auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& mv) {
			return PackMove(mv) == ttMove;
			});
		if (it != moves.end())
			std::swap(moves[0], *it);
	}

	int bestScore;
	uint32_t bestMove32 = 0;
	uint8_t flag;

	bool maximizing = movingColor == maximizingColor;

	int bestEval;
	if (maximizing) bestEval = std::numeric_limits<int>::min();
	else			bestEval = std::numeric_limits<int>::max();

	// Loop through moves
	for (Move move : moves)
	{
		engine->MakeMove(move);

		if (engine->InCheck(movingColor)) { engine->UndoMove(); continue; }

		foundLegal = true;
		int eval = Search(depth - 1, maximizingColor, alpha, beta);
		engine->UndoMove();

		if (quitEarly)
			return 0;

		if (maximizing)
		{
			if (eval > bestEval)
			{
				bestEval = eval;
				bestMove32 = PackMove(move);
			}
		}
		else
		{
			if (eval < bestEval)
			{
				bestEval = eval;
				bestMove32 = PackMove(move);
			}
		}

		if (maximizing) alpha = std::max(alpha, eval);
		else			  beta  = std::min(beta, eval);
		if (beta <= alpha) break;
	}

	// Check checkmate/stalemate
	if (!foundLegal)
	{
		engine->CheckKingInCheck();
		if (engine->InCheck(movingColor))
		{
			if (movingColor == Color::BLACK)
				bestEval = std::numeric_limits<int>::max() - depth;
			else
				bestEval = -(std::numeric_limits<int>::max() - depth);
		}
		else bestEval = 0; // Stalemate
	}

	// Update TT
	bestScore = bestEval;
	if (bestScore <= alpha) flag = TT_ALPHA;
	else if (bestScore >= beta) flag = TT_BETA;
	else flag = TT_EXACT;

	tt.ttStore(key, depth, bestScore, bestMove32, flag);
	return bestScore;
}

int Bot::Qsearch(int alpha, int beta, Color maximizingPlayer)
{
	int baseEval = engine->Eval();  // Static eval

	Color movingColor = GameState::currentPlayer;

	// Fail-hard beta cutoff
	if (movingColor == maximizingPlayer)
	{
		if (baseEval >= beta)
			return beta;
		if (baseEval > alpha)
			alpha = baseEval;
	}
	else
	{
		if (baseEval <= alpha)
			return alpha;
		if (baseEval < beta)
			beta = baseEval;
	}

	// Generate only "noisy" moves (captures, promotions, maybe checks)
	bool onlyNoisy = true;
	std::vector<Move> moves = BoardCalculator::GetAllMoves(movingColor, engine->GetBoard(), onlyNoisy);

	for (Move move : moves)
	{
		engine->MakeMove(move);
		if (engine->InCheck(maximizingPlayer)) // Skip illegal moves
		{
			engine->UndoMove();
			continue;
		}

		int score = Qsearch(alpha, beta, maximizingPlayer);
		engine->UndoMove();

		if (movingColor == maximizingPlayer)
		{
			if (score >= beta)
				return beta; // Cutoff
			if (score > alpha)
				alpha = score;
		}
		else
		{
			if (score <= alpha)
				return alpha; // Cutoff
			if (score < beta)
				beta = score;
		}
	}

	return (movingColor == maximizingPlayer) ? alpha : beta;
}