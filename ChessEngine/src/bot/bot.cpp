#include "bot.hpp"

#include "core/boardCalculator.hpp"

#include <limits>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <Windows.h>
#include <omp.h>
#undef min
#undef max
using namespace std::chrono;

constexpr int INF = std::numeric_limits<int>::max() / 4;
constexpr int MATE_VAL = 1000000;

uint32_t PackMove(const Move& m)
{
	uint32_t s = m.startSquare;
	uint32_t t = m.endSquare;
	// TODO: This is an incorrect packing of promotion
	uint32_t promo = (m.promotion ? (m.promotion & 0x7) : 0); // user-defined
	return (s) | (t << 6) | (promo << 12) | ((m.wasEnPassant ? 1u : 0u) << 15) | ((m.wasCastle ? 1u : 0u) << 16);
}

Move UnpackMove(uint32_t code)
{
	Move m;
	uint32_t s = code & 0x3F;
	uint32_t t = (code >> 6) & 0x3F;
	m.startSquare = s;
	m.endSquare = t;
	m.promotion = (uint8_t)((code >> 12) & 0x7);
	m.wasEnPassant = ((code >> 15) & 1);
	m.wasCastle = ((code >> 16) & 1);
	return m;
}

Bot::Bot(Engine* engine, Color color)
{
	this->engine = engine;
	this->botColor = color;

	for (int i = 0; i < 128; ++i)
		moveLists[i].clear();
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

// Move ordering scores
const int captureBonus = 1000000;
const int killerBonus = 500000;

int Bot::ScoreMove(const Move move, int ply, bool onlyMVVLVA)
{
	Piece moved = engine->GetBoard()[move.startSquare].GetPiece();
	Piece captured = engine->GetBoard()[move.endSquare].GetPiece();

	// MVV-LVA: Most Valuable Victim - Least Valuable Attacker
	if (captured.GetType() != Pieces::NONE)
		return captureBonus + PieceValue(captured) * 16 - PieceValue(moved);

	if (onlyMVVLVA) return 0;
	if (move == killerMoves[ply][0]) return killerBonus;
	else if (move == killerMoves[ply][1]) return killerBonus - 100;

	return historyHeuristic[(int)moved.GetColor()][(int)moved.GetType()][move.startSquare][move.endSquare];
}

// Orders moves in-place, best first
void Bot::OrderMoves(std::vector<Move>& moves, int ply, bool onlyMVVLVA, Move firstMove)
{
	std::sort(moves.begin(), moves.end(),
		[&](const Move& a, const Move& b) {
			return ScoreMove(a, ply, onlyMVVLVA) > ScoreMove(b, ply, onlyMVVLVA);
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

void Bot::Clear()
{
	for (int i = 0; i < 32; ++i)
		moveLists[i].clear();

	nodesSearched = 0;
	quitEarly = false;

	tt.Clear();
}

Move Bot::GetMoveUCI(int wtime, int btime)
{
	int timeForMove;
	if (IsWhite(botColor))
		timeForMove = wtime / 20;
	else
		timeForMove = btime / 20;

	timePerTurn = 500;
	Move move = GetMove();
	return move;
}

Move Bot::GetMove()
{
	Move bookMove = GetBookMove(engine, "res/openings.bin");
	if (!bookMove.IsNull())
	{
		std::cout << "Using opening move\n";
		return bookMove; // Play instantly
	}

	quitEarly = false;
	startTime = steady_clock::now();
	tt.NewSearch(); // age TT entries for this root search

	// Clear killer moves before each search
	memset(killerMoves, 0, sizeof(killerMoves));

	//// Decay history heuristic scores
	for (int i = 0; i < 2; ++i)
		for (int j = 0; j < NUM_PIECES; ++j)
			for (int x = 0; x < 64; ++x)
				for (int y = 0; y < 64; ++y)
					historyHeuristic[i][j][x][y] /= 2;

	int maxDepth = 8;
	Move bestMove = Move();
	int bestScore = -INF;

	for (int depth = 1; depth <= maxDepth; ++depth)
	{
		int alpha = -INF;
		int beta = INF;
		bool foundLegal = false;

		Move currentBestMove = Move();
		int currentBestScore = -INF;

		std::vector<Move> moves = BoardCalculator::GetAllMoves(botColor, engine->GetBoard(), engine);
		if (!bestMove.IsNull()) // Current best move first to help with pruning
			OrderMoves(moves, 0, false, bestMove);
		else
		{
			OrderMoves(moves, 0, false);
			if (!moves.empty()) bestMove = moves[0]; // If not enough time to find move, set default after sorting
		}

		for (const Move& move : moves)
		{
			Piece movingPiece = engine->GetBoard()[move.startSquare].GetPiece();

			engine->MakeMove(move);

			if (engine->InCheck(botColor)) { engine->UndoMove(); continue; }

			foundLegal = true;

			int score = -Search(depth - 1, 1, -beta, -alpha);

			engine->UndoMove();

			if (quitEarly)
				break;

			auto elapsed = duration_cast<milliseconds>(steady_clock::now() - startTime).count();
			if (elapsed >= timePerTurn)
			{
				quitEarly = true;
				break;
			}

			if (score > currentBestScore)
			{
				currentBestScore = score;
				currentBestMove = move;
			}

			alpha = std::max(alpha, currentBestScore);
			if (alpha >= beta)
				break;
		}

		if (foundLegal)
		{
			if (!quitEarly)
			{
				bestMove = currentBestMove;
				bestScore = currentBestScore;
			}
			else
			{
				// If better move found in partial search
				if (currentBestScore > bestScore)
				{
					bestScore = currentBestScore;
					bestMove = currentBestMove;
				}
				break; // If quit early break
			}
		}

		// Extradite mate for bot
		// Cast to avoid integer overflow
		if ((long long)bestScore >= (MATE_VAL - MAX_PLY))
			break;

		auto elapsed = duration_cast<milliseconds>(steady_clock::now() - startTime).count();
		// Keep searching until limit
		if (depth == maxDepth)
			if (elapsed < timePerTurn)
				++maxDepth;

		// Stop searching if time limit is reached
		if (elapsed >= timePerTurn)
			break;
	}

	std::cout << "Making " << bestMove.ToUCIString() << " with score " << bestScore << '\n';
	nodesSearched = 0;

	Piece movingPiece = engine->GetBoard()[bestMove.startSquare].GetPiece();
	if (!engine->ValidMove(movingPiece, bestMove)) throw "Invalid move";

	if (!bestMove.IsNull())
		return bestMove;
	else
		throw "Move was null\n";
}

int Bot::Search(int depth, int ply, int alpha, int beta)
{
	++nodesSearched;
	// Check time every 1028 nodes (& faster than %)
	if ((nodesSearched & 0x1027) == 0)
	{
		auto elapsed = duration_cast<milliseconds>(steady_clock::now() - startTime).count();
		if (elapsed >= timePerTurn)
		{
			quitEarly = true;
			return 0;
		}
	}

	if (engine->IsDraw())
		return 0;

	if (depth == 0 || engine->IsOver())
	{
		return Qsearch(alpha, beta, 1);
		//return Eval(GameState::currentPlayer, engine->GetBoard());
	}

	uint64_t key = engine->GetZobristKey();
	int ttScore;
	uint32_t ttMove;
	if (tt.ttProbe(key, depth, alpha, beta, ttScore, ttMove))
		return ttScore;

	Color movingColor = GameState::currentPlayer;
	bool foundLegal = false;

	std::vector<Move>& moves = moveLists[ply];
	BoardCalculator::GetAllMoves(moves, movingColor, engine->GetBoard(), engine);
	OrderMoves(moves, ply, false);

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

	int originalAlpha = alpha;
	uint32_t bestMove32 = 0;
	uint8_t flag = TT_ALPHA;

	int bestEval = -INF;

	//int moveCount = 0;

	// Loop through moves
	for (const Move& move : moves)
	{
		engine->MakeMove(move);

		if (engine->InCheck(movingColor)) { engine->UndoMove(); continue; }


		bool opponentInCheck = engine->InCheck(Opponent(movingColor));

		foundLegal = true;

		int eval = -Search(depth - 1, ply + 1, -beta, -alpha);

		engine->UndoMove();

		if (quitEarly)
			return 0;

		if (eval > bestEval)
		{
			bestEval = eval;
			bestMove32 = PackMove(move);
		}

		alpha = std::max(alpha, eval);
		if (alpha >= beta)
		{
			if (!move.IsCapture(engine->GetBoard()) && (Pieces)move.promotion == Pieces::NONE && !opponentInCheck) // Beta cutoff = good
			{
				if (killerMoves[ply][0] != move)
				{
					killerMoves[ply][1] = killerMoves[ply][0];
					killerMoves[ply][0] = move;
				}

				int from = move.startSquare;
				int to = move.endSquare;
				Piece movingPiece = engine->GetBoard()[from].GetPiece();
				int pieceType = (int)movingPiece.GetType();

				historyHeuristic[(int)movingPiece.GetColor()][pieceType][from][to] += depth * depth;
			}
			break;
		}
	}

	// Check checkmate/stalemate
	if (!foundLegal)
	{
		engine->CheckKingInCheck();
		if (engine->InCheck(movingColor))
		{
			bestEval = -(MATE_VAL - ply);
		}
		else bestEval = 0; // Stalemate
	}

	// Update TT
	int bestScore = bestEval;
	if (bestScore <= originalAlpha) flag = TT_ALPHA;
	else if (bestScore >= beta) flag = TT_BETA;
	else flag = TT_EXACT;

	tt.ttStore(key, depth, bestScore, bestMove32, flag);
	return bestScore;
}

int Bot::Qsearch(int alpha, int beta, int ply)
{
	nodesSearched++;

	int standPat = Eval(GameState::currentPlayer, engine->GetBoard());

	const int QSEARCH_PLY_MAX = 8;
	if (ply >= QSEARCH_PLY_MAX)
		return standPat;

	// Fail-hard beta cutoff
	if (standPat >= beta)
		return beta;
	if (standPat > alpha)
		alpha = standPat;
	
	Color movingColor = GameState::currentPlayer;

	const int DELTA_MARGIN = 200; // Centipawns
	if (standPat + DELTA_MARGIN < alpha)
		return alpha; // Position too bad, don’t bother with captures

	// Generate only "noisy" moves (captures, promotions, checks)
	bool onlyNoisy = true;
	std::vector<Move> moves = BoardCalculator::GetAllMoves(movingColor, engine->GetBoard(), engine, onlyNoisy);
	OrderMoves(moves, 0, true);

	for (const Move& move : moves)
	{
		engine->MakeMove(move);
		if (engine->InCheck(Opponent(GameState::currentPlayer))) // Skip illegal moves
		{
			engine->UndoMove();
			continue;
		}

		int score = -Qsearch(-beta, -alpha, ply + 1);
		engine->UndoMove();

		if (score >= beta)
			return beta; // Cutoff

		if (score > alpha)
			alpha = score;
	}

	return alpha;
}