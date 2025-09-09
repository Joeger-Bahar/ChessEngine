#include "bot.hpp"

#include "core/boardCalculator.hpp"

#include <limits>
#include <iostream>
#include <vector>
#include <algorithm>
#include <Windows.h>
#undef min
#undef max

uint32_t packMove(const Move& m) {
	uint32_t s = m.startRow * 8 + m.startCol;
	uint32_t t = m.endRow * 8 + m.endCol;
	uint32_t promo = (m.promotion ? (m.promotion & 0x7) : 0); // user-defined
	return (s) | (t << 6) | (promo << 12) | ((m.wasEnPassant ? 1u : 0u) << 15) | ((m.wasCastle ? 1u : 0u) << 16);
}

Move unpackMove(uint32_t code) {
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

// Negamax search
Move Bot::GetMove()
{
	int depth = 7;

	Move bestMove = Move();
	int alpha = std::numeric_limits<int>::min();
	int beta = std::numeric_limits<int>::max();
	bool foundLegal = false;

	int bestScore;

	std::vector<Move> moves = BoardCalculator::GetAllMoves(botColor, engine->GetBoard());
	OrderMoves(moves);

	if (botColor == Color::WHITE)
	{
		bestScore = std::numeric_limits<int>::min();

		for (Move move : moves)
		{
			engine->MakeMove(move);

			engine->CheckKingInCheck();
			bool illegal = engine->InCheck(botColor);

			if (illegal)
			{
				engine->UndoMove();
				continue; // skip illegal move
			}

			foundLegal = true;

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
		bestScore = std::numeric_limits<int>::max();

		for (Move move : moves)
		{
			engine->MakeMove(move);

			engine->CheckKingInCheck();
			bool illegal = engine->InCheck(botColor);

			if (illegal)
			{
				engine->UndoMove();
				continue; // skip illegal move
			}

			foundLegal = true;

			// White is always maximizing because eval is static (positive = white winning)
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

	//std::cout << "Eval: " << bestScore << "\n";

	return bestMove;
}

//int Bot::Search(int depth, Color maximizingColor, int alpha, int beta)
//{
//	if (depth == 0 || engine->IsOver())
//		return engine->Eval();
//
//	Color movingColor = GameState::currentPlayer;
//	bool foundLegal = false;
//
//	auto moves = BoardCalculator::GetAllMoves(movingColor, engine->GetBoard());
//	OrderMoves(moves);
//
//	int bestEval = (movingColor == maximizingColor) ? std::numeric_limits<int>::min()
//		: std::numeric_limits<int>::max();
//
//	for (Move move : moves)
//	{
//		engine->MakeMove(move);
//
//		if (engine->InCheck(movingColor)) {
//			engine->UndoMove();
//			continue; // skip illegal
//		}
//
//		foundLegal = true;
//
//		int eval = -Search(depth - 1, maximizingColor, -beta, -alpha);
//
//		engine->UndoMove();
//
//		if (movingColor == maximizingColor) {
//			bestEval = std::max(bestEval, eval);
//			alpha = std::max(alpha, eval);
//		}
//		else {
//			bestEval = std::min(bestEval, eval);
//			beta = std::min(beta, eval);
//		}
//
//		if (alpha >= beta)
//			break; // prune
//	}
//
//	if (!foundLegal)
//	{
//		// Checkmate or stalemate
//		if (engine->InCheck(movingColor))
//			return (movingColor == maximizingColor) ? (std::numeric_limits<int>::min()+1000) - depth
//			: (std::numeric_limits<int>::max()-1000) + depth;
//		else
//			return 0; // stalemate
//	}
//
//	return bestEval;
//}

int Bot::Search(int depth, Color maximizingColor, int alpha, int beta)
{
	if (depth == 0 || engine->IsOver())
		return engine->Eval();

	Color movingColor = GameState::currentPlayer;
	bool foundLegal = false;

	std::vector<Move> moves = BoardCalculator::GetAllMoves(movingColor, engine->GetBoard());

	OrderMoves(moves);

	if (movingColor == maximizingColor)
	{
		int maxEval = std::numeric_limits<int>::min();

		for (Move move : moves)
		{
			engine->MakeMove(move);

			engine->CheckKingInCheck();
			bool illegal = engine->InCheck(movingColor);

			if (illegal)
			{
				engine->UndoMove();
				continue; // Skip illegal move
			}

			foundLegal = true;

			int eval = Search(depth - 1, maximizingColor, alpha, beta);
			engine->UndoMove();

			maxEval = std::max(maxEval, eval);
			alpha = std::max(alpha, eval);

			if (beta <= alpha)
				break; // Prune
		}
		if (!foundLegal)
		{
			engine->CheckKingInCheck();
			if (engine->InCheck(movingColor))
			{
				// This is +-1000 to provide a buffer, so punishing depth doesn't overflow
				if (movingColor == Color::BLACK)
					return (std::numeric_limits<int>::max() - 1000) + depth;
				else
					return (std::numeric_limits<int>::min() + 1000) - depth;
			}
			else return 0; // Stalemate
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
			bool illegal = engine->InCheck(movingColor);

			if (illegal)
			{
				engine->UndoMove();
				continue; // Skip illegal move
			}

			foundLegal = true;

			int eval = Search(depth - 1, maximizingColor, alpha, beta);
			engine->UndoMove();

			minEval = std::min(minEval, eval);
			beta = std::min(beta, eval);

			if (beta <= alpha)
				break; // Prune
		}
		if (!foundLegal)
		{
			engine->CheckKingInCheck();
			if (engine->InCheck(movingColor))
			{
				// These are possible mates, not guarunteed
				if (movingColor == Color::BLACK)
					return (std::numeric_limits<int>::max() - 1000) + depth;
				else
					return (std::numeric_limits<int>::min() + 1000) - depth;
			}
			return 0; // Stalemate
		}

		return minEval;
	}
}
//int Bot::Search(int depth, Color maximizingColor, int alpha, int beta)
//{
//	nodesSearched++;
//	if (depth == 0 || engine->IsOver())
//		return engine->Eval();
//
//	uint64_t key = engine->GetZobristKey();
//	uint32_t ttMove32 = 0;
//	int ttScore = 0;
//
//	int alphaOrig = alpha;
//
//	// Probe TT
//	if (tt.ttProbe(key, depth, alpha, beta, ttScore, ttMove32))
//		return ttScore;
//		//NULL;
//
//	Color movingColor = GameState::currentPlayer;
//	bool foundLegal = false;
//
//	std::vector<Move> moves = BoardCalculator::GetAllMoves(movingColor, engine->GetBoard());
//
//	// Move ordering: TT move first, then heuristics
//	//if (ttMove32 != 0) {
//	//	Move ttMove = unpackMove(ttMove32);
//	//	auto it = std::find(moves.begin(), moves.end(), ttMove);
//	////	if (it != moves.end())
//	////		std::swap(moves[0], *it);
//	//}
//	OrderMoves(moves);
//
//	int bestScore = (movingColor == maximizingColor) ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
//	Move bestMove;
//
//	for (Move move : moves)
//	{
//		engine->MakeMove(move);
//		if (engine->InCheck(movingColor)) { engine->UndoMove(); continue; }
//
//		foundLegal = true;
//		int eval = Search(depth - 1, maximizingColor, alpha, beta);
//		engine->UndoMove();
//
//		if (movingColor == maximizingColor) {
//			if (eval > bestScore) { bestScore = eval; bestMove = move; }
//			alpha = std::max(alpha, eval);
//		}
//		else {
//			if (eval < bestScore) { bestScore = eval; bestMove = move; }
//			beta = std::min(beta, eval);
//		}
//
//		if (beta <= alpha) break; // alpha-beta prune
//	}
//
//	if (!foundLegal) { // mate/stalemate handling
//		engine->CheckKingInCheck();
//		if (engine->InCheck(movingColor))
//			return (movingColor == maximizingColor) ? std::numeric_limits<int>::min() + depth : std::numeric_limits<int>::max() - depth;
//		else
//			return 0; // stalemate
//	}
//
//	// Store in TT
//	uint8_t flag;
//	if (bestScore <= alphaOrig) flag = TT_ALPHA;
//	else if (bestScore >= beta) flag = TT_BETA;
//	else flag = TT_EXACT;
//
//	tt.ttStore(key, depth, bestScore, packMove(bestMove), flag);
//
//	return bestScore;
//}
