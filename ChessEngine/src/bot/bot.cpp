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

// Minimax search
//Move Bot::TTGetMove()
//{
//	tt.newSearch();
//
//	int depth = 8;
//
//	Move bestMove = Move();
//	int alpha = std::numeric_limits<int>::min();
//	int beta = std::numeric_limits<int>::max();
//	bool foundLegal = false;
//
//	int bestScore;
//
//	std::vector<Move> moves = BoardCalculator::GetAllMoves(botColor, engine->GetBoard());
//	OrderMoves(moves);
//
//	if (botColor == Color::WHITE)
//	{
//		bestScore = std::numeric_limits<int>::min();
//
//		for (Move move : moves)
//		{
//			engine->MakeMove(move);
//
//			engine->CheckKingInCheck();
//			bool illegal = engine->InCheck(botColor);
//
//			if (illegal)
//			{
//				engine->UndoMove();
//				continue; // skip illegal move
//			}
//
//			foundLegal = true;
//
//			int score = TTSearch(depth - 1, Color::WHITE, alpha, beta);
//			engine->UndoMove();
//
//			if (score > bestScore)
//			{
//				bestScore = score;
//				bestMove = move;
//			}
//
//			alpha = std::max(alpha, bestScore);
//		}
//	}
//	else
//	{
//		bestScore = std::numeric_limits<int>::max();
//
//		for (Move move : moves)
//		{
//			engine->MakeMove(move);
//
//			engine->CheckKingInCheck();
//			bool illegal = engine->InCheck(botColor);
//
//			if (illegal)
//			{
//				engine->UndoMove();
//				continue; // skip illegal move
//			}
//
//			foundLegal = true;
//
//			// White is always maximizing because eval is static (positive = white winning)
//			int score = TTSearch(depth - 1, Color::WHITE, alpha, beta);
//			engine->UndoMove();
//
//			if (score < bestScore)
//			{
//				bestScore = score;
//				bestMove = move;
//			}
//
//			beta = std::min(beta, bestScore);
//		}
//	}
//
//	std::cout << "TT Nodes searched " << nodesSearched << "\n";
//
//	return bestMove;
//}
Move Bot::TTGetMove()
{
	tt.newSearch(); // age TT entries for this root search

	int maxDepth = 9;
	Move bestMove = Move();
	int bestScore = 0;

	for (int depth = 1; depth <= maxDepth; ++depth) {
		int alpha = std::numeric_limits<int>::min();
		int beta = std::numeric_limits<int>::max();
		bool foundLegal = false;

		Move currentBestMove = Move();
		int currentBestScore;

		std::vector<Move> moves = BoardCalculator::GetAllMoves(botColor, engine->GetBoard());
		OrderMoves(moves);

		if (botColor == Color::WHITE) {
			currentBestScore = std::numeric_limits<int>::min();

			for (Move move : moves) {
				engine->MakeMove(move);

				engine->CheckKingInCheck();
				bool illegal = engine->InCheck(botColor);
				if (illegal) { engine->UndoMove(); continue; }

				foundLegal = true;

				int score = TTSearch(depth - 1, Color::WHITE, alpha, beta);
				engine->UndoMove();

				if (score > currentBestScore) {
					currentBestScore = score;
					currentBestMove = move;
				}

				alpha = std::max(alpha, currentBestScore);
			}
		}
		else {
			currentBestScore = std::numeric_limits<int>::max();

			for (Move move : moves) {
				engine->MakeMove(move);

				engine->CheckKingInCheck();
				bool illegal = engine->InCheck(botColor);
				if (illegal) { engine->UndoMove(); continue; }

				foundLegal = true;

				int score = TTSearch(depth - 1, Color::WHITE, alpha, beta);
				engine->UndoMove();

				if (score < currentBestScore) {
					currentBestScore = score;
					currentBestMove = move;
				}

				beta = std::min(beta, currentBestScore);
			}
		}

		if (foundLegal) {
			bestMove = currentBestMove;
			bestScore = currentBestScore;
		}

		// Optional: print progress for debugging
		std::cout << "Depth " << depth << " bestMove "
			<< currentBestMove.ToString()
			<< " score " << currentBestScore << "\n";

		// Mate
		if (bestScore > 2000000000 || bestScore < -2000000000)
			break;
	}
	std::cout << "TT searched " << nodesSearched << '\n';

	return bestMove;
}

Move Bot::GetMove()
{
	int depth = 8;

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

	std::cout << "Nodes searched " << nodesSearched << "\n";

	return bestMove;
}

int Bot::TTSearch(int depth, Color maximizingColor, int alpha, int beta)
{
	nodesSearched++;
	if (depth == 0 || engine->IsOver())
	{
		return engine->Eval();
	}

	uint64_t key = engine->GetZobristKey();
	int ttScore;
	uint32_t ttMove;
	if (tt.ttProbe(key, depth, alpha, beta, ttScore, ttMove)) {
		return ttScore;
	}

	Color movingColor = GameState::currentPlayer;
	bool foundLegal = false;

	std::vector<Move> moves = BoardCalculator::GetAllMoves(movingColor, engine->GetBoard());
	OrderMoves(moves);

	// If we probed a move from TT, move it to the front
	if (ttMove) {
		Move m = unpackMove(ttMove);
		auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& mv) {
			return packMove(mv) == ttMove;
			});
		if (it != moves.end()) {
			std::swap(moves[0], *it);
		}
	}

	int bestScore;
	uint32_t bestMove32 = 0;
	uint8_t flag;

	if (movingColor == maximizingColor) {
		int maxEval = std::numeric_limits<int>::min();

		for (Move move : moves) {
			engine->MakeMove(move);
			engine->CheckKingInCheck();
			bool illegal = engine->InCheck(movingColor);

			if (illegal) { engine->UndoMove(); continue; }

			foundLegal = true;
			int eval = TTSearch(depth - 1, maximizingColor, alpha, beta);
			engine->UndoMove();

			if (eval > maxEval) {
				maxEval = eval;
				bestMove32 = packMove(move);
			}

			alpha = std::max(alpha, eval);
			if (beta <= alpha) break;
		}

		if (!foundLegal) {
			// handle mate/stalemate same as before
			engine->CheckKingInCheck();
			if (engine->InCheck(movingColor))
			{
				// This is +-1000 to provide a buffer, so punishing depth doesn't overflow
				if (movingColor == Color::BLACK)
					maxEval = (std::numeric_limits<int>::max() - 1000) + depth;
				else
					maxEval = (std::numeric_limits<int>::min() + 1000) - depth;
			}
			else maxEval = 0; // Stalemate
			//maxEval = (engine->InCheck(movingColor) ? /*mate score*/ : 0);
		}

		bestScore = maxEval;
		if (bestScore <= alpha) flag = TT_ALPHA;
		else if (bestScore >= beta) flag = TT_BETA;
		else flag = TT_EXACT;
	}
	else {
		int minEval = std::numeric_limits<int>::max();
		for (Move move : moves) {
			engine->MakeMove(move);
			engine->CheckKingInCheck();
			bool illegal = engine->InCheck(movingColor);

			if (illegal) { engine->UndoMove(); continue; }

			foundLegal = true;
			int eval = TTSearch(depth - 1, maximizingColor, alpha, beta);
			engine->UndoMove();

			if (eval < minEval) {
				minEval = eval;
				bestMove32 = packMove(move);
			}

			beta = std::min(beta, eval);
			if (beta <= alpha) break;
		}

		if (!foundLegal) {
			// handle mate/stalemate same as before
			engine->CheckKingInCheck();
			if (engine->InCheck(movingColor))
			{
				// This is +-1000 to provide a buffer, so punishing depth doesn't overflow
				if (movingColor == Color::BLACK)
					minEval = (std::numeric_limits<int>::max() - 1000) + depth;
				else
					minEval = (std::numeric_limits<int>::min() + 1000) - depth;
			}
			else minEval = 0; // Stalemate
			//minEval = (engine->InCheck(movingColor) ? /*mate score*/ : 0);
		}

		bestScore = minEval;
		if (bestScore <= alpha) flag = TT_ALPHA;
		else if (bestScore >= beta) flag = TT_BETA;
		else flag = TT_EXACT;
	}

	tt.ttStore(key, depth, bestScore, bestMove32, flag);
	return bestScore;
}
int Bot::Search(int depth, Color maximizingColor, int alpha, int beta)
{
	++nodesSearched;
	if (depth == 0 || engine->IsOver())
		return engine->Eval();

	Color movingColor = GameState::currentPlayer;
	bool foundLegal = false;

	std::vector<Move> moves = BoardCalculator::GetAllMoves(movingColor, engine->GetBoard());
	OrderMoves(moves);

	int bestScore;

	if (movingColor == maximizingColor) {
		int maxEval = std::numeric_limits<int>::min();

		for (Move move : moves) {
			engine->MakeMove(move);
			engine->CheckKingInCheck();
			bool illegal = engine->InCheck(movingColor);

			if (illegal) { engine->UndoMove(); continue; }

			foundLegal = true;
			int eval = Search(depth - 1, maximizingColor, alpha, beta);
			engine->UndoMove();

			if (eval > maxEval) {
				maxEval = eval;
			}

			alpha = std::max(alpha, eval);
			if (beta <= alpha) break;
		}

		if (!foundLegal) {
			// handle mate/stalemate same as before
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
			//maxEval = (engine->InCheck(movingColor) ? /*mate score*/ : 0);
		}

		bestScore = maxEval;
	}
	else {
		int minEval = std::numeric_limits<int>::max();
		for (Move move : moves) {
			engine->MakeMove(move);
			engine->CheckKingInCheck();
			bool illegal = engine->InCheck(movingColor);

			if (illegal) { engine->UndoMove(); continue; }

			foundLegal = true;
			int eval = Search(depth - 1, maximizingColor, alpha, beta);
			engine->UndoMove();

			if (eval < minEval) {
				minEval = eval;
			}

			beta = std::min(beta, eval);
			if (beta <= alpha) break;
		}

		if (!foundLegal) {
			// handle mate/stalemate same as before
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
			//minEval = (engine->InCheck(movingColor) ? /*mate score*/ : 0);
		}

		bestScore = minEval;
	}

	return bestScore;
}

//	if (movingColor == maximizingColor)
//	{
//		int maxEval = std::numeric_limits<int>::min();
//
//		for (Move move : moves)
//		{
//			engine->MakeMove(move);
//
//			engine->CheckKingInCheck();
//			bool illegal = engine->InCheck(movingColor);
//
//			if (illegal)
//			{
//				engine->UndoMove();
//				continue; // Skip illegal move
//			}
//
//			foundLegal = true;
//
//			int eval = Search(depth - 1, maximizingColor, alpha, beta);
//			engine->UndoMove();
//
//			maxEval = std::max(maxEval, eval);
//			alpha = std::max(alpha, eval);
//
//			if (beta <= alpha)
//				break; // Prune
//		}
//		if (!foundLegal)
//		{
//			engine->CheckKingInCheck();
//			if (engine->InCheck(movingColor))
//			{
//				// This is +-1000 to provide a buffer, so punishing depth doesn't overflow
//				if (movingColor == Color::BLACK)
//					return (std::numeric_limits<int>::max() - 1000) + depth;
//				else
//					return (std::numeric_limits<int>::min() + 1000) - depth;
//			}
//			else return 0; // Stalemate
//		}
//
//		return maxEval;
//	}
//	else
//	{
//		int minEval = std::numeric_limits<int>::max();
//
//		for (Move move : moves)
//		{
//			engine->MakeMove(move);
//
//			engine->CheckKingInCheck();
//			bool illegal = engine->InCheck(movingColor);
//
//			if (illegal)
//			{
//				engine->UndoMove();
//				continue; // Skip illegal move
//			}
//
//			foundLegal = true;
//
//			int eval = Search(depth - 1, maximizingColor, alpha, beta);
//			engine->UndoMove();
//
//			minEval = std::min(minEval, eval);
//			beta = std::min(beta, eval);
//
//			if (beta <= alpha)
//				break; // Prune
//		}
//		if (!foundLegal)
//		{
//			engine->CheckKingInCheck();
//			if (engine->InCheck(movingColor))
//			{
//				// These are possible mates, not guarunteed
//				if (movingColor == Color::BLACK)
//					return (std::numeric_limits<int>::max() - 1000) + depth;
//				else
//					return (std::numeric_limits<int>::min() + 1000) - depth;
//			}
//			return 0; // Stalemate
//		}
//
//		return minEval;
//	}
//}