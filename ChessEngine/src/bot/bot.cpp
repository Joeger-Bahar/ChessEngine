#include "bot.hpp"

#include "core/boardCalculator.hpp"
#include "core/movegen.hpp"

#include "tablebase.hpp"

#include <limits>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <Windows.h>
#undef min
#undef max
using namespace std::chrono;

constexpr int INF = std::numeric_limits<int>::max() / 4;
constexpr int MATE_VAL = 1000000;

static int ScoreToTT(int score, int ply)
{
	if (score >= MATE_VAL - MAX_PLY) return score + ply;
	if (score <= -MATE_VAL + MAX_PLY) return score - ply;
	return score;
}

static int ScoreFromTT(int score, int ply)
{
	if (score >= MATE_VAL - MAX_PLY) return score - ply;
	if (score <= -MATE_VAL + MAX_PLY) return score + ply;
	return score;
}

static Tablebase tb("res/syzygy/");

Bot::Bot(Engine* engine, Color color)
{
	this->engine = engine;
	this->botColor = color;
	nodesSearched = 0;
	memset(killerMoves, 0, sizeof(killerMoves));
	memset(counterMoves, 0, sizeof(counterMoves));

	for (int i = 0; i < MAX_PLY; ++i)
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
const int killerBonus = 100000;

int Bot::ScoreMove(const Move move, int ply, bool onlyMVVLVA)
{
	Piece moved = engine->GetBoard()[GetStart(move)].GetPiece();
	Piece captured = engine->GetBoard()[GetEnd(move)].GetPiece();
	int promotion = GetPromotion(move);
	int promotionBonus = promotion == static_cast<int>(Pieces::NONE) ? 0 :
		PieceValue(Piece(static_cast<Pieces>(promotion), moved.GetColor())) - PieceValue(Piece(Pieces::PAWN, moved.GetColor()));

	// MVV-LVA: Most Valuable Victim - Least Valuable Attacker
	if (captured.GetType() != Pieces::NONE)
		return captureBonus + PieceValue(captured) * 16 - PieceValue(moved) + promotionBonus;
	if (promotionBonus > 0)
		return captureBonus - 1 + promotionBonus;

	if (onlyMVVLVA) return 0;
	if (move == killerMoves[ply][0]) return killerBonus;
	else if (move == killerMoves[ply][1]) return killerBonus - 100;

	if (ply > 0 && !MoveIsNull(previousMoves[ply]))
	{
		Move prev = previousMoves[ply];

		int prevFrom = GetStart(prev);
		int prevTo = GetEnd(prev);
		Color color = (Color)moved.GetColor();

		if (move == counterMoves[(int)color][prevFrom][prevTo])
			return killerBonus - 200; // Slightly below killer moves
	}

	return historyHeuristic[(int)moved.GetColor()][(int)moved.GetType() - 1][GetStart(move)][GetEnd(move)];
}

// Orders moves in-place, best first
void Bot::OrderMoves(std::vector<Move>& moves, int ply, bool onlyMVVLVA, Move firstMove)
{
	std::sort(moves.begin(), moves.end(),
		[&](const Move& a, const Move& b) {
			return ScoreMove(a, ply, onlyMVVLVA) > ScoreMove(b, ply, onlyMVVLVA);
		});

	if (!MoveIsNull(firstMove))
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
	for (int i = 0; i < MAX_PLY; ++i)
	{
		moveLists[i].clear();
		qMoveLists[i].clear();
	}

	nodesSearched = 0;
	quitEarly = false;
	afterNullMove = false;
	memset(previousMoves, 0, sizeof(previousMoves));

	memset(counterMoves, 0, sizeof(counterMoves));

	tt.Clear();
}

Move Bot::GetMoveUCI(int timeForMove)
{
	timePerTurn = timeForMove;
	Move move = GetMove();
	return move;
}

Move Bot::GetMove()
{
	Move bookMove = GetBookMove(engine, "res/openings.bin");
	if (!MoveIsNull(bookMove))
	{
		std::cout << "Using opening move\n";
		return bookMove; // Play instantly
	}

	quitEarly = false;
	afterNullMove = false;
	memset(previousMoves, 0, sizeof(previousMoves));
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

	if (tb.Probeable(engine->GetBitboardBoard()))
	{
		return tb.GetMove(engine);
	}

	for (int depth = 1; depth <= maxDepth; ++depth)
	{
		std::cout << "Depth: " << depth << '\n';
		int alpha = -INF;
		int beta = INF;
		bool foundLegal = false;

		Move currentBestMove = Move();
		int currentBestScore = -INF;
		int moveCount = 0;

		std::vector<Move> moves = moveLists[0];
		Movegen::GetAllMoves(moves, botColor, engine->GetBitboardBoard(), engine);
		if (!MoveIsNull(bestMove)) // Current best move first to help with pruning
			OrderMoves(moves, 0, false, bestMove);
		else
		{
			OrderMoves(moves, 0, false);
			if (!moves.empty()) bestMove = moves[0]; // If not enough time to find move, set default after sorting
		}

		for (const Move& move : moves)
		{
			engine->MakeMove(move);

			if (engine->InCheck(botColor)) { engine->UndoMove(); continue; }

			foundLegal = true;
			++moveCount;
			previousMoves[1] = move;

			int score;
			if (moveCount == 1)
			{
				score = -Search(depth - 1, 1, -beta, -alpha);
			}
			else
			{
				score = -Search(depth - 1, 1, -alpha - 1, -alpha);
				if (!quitEarly && score > alpha && score < beta)
					score = -Search(depth - 1, 1, -beta, -alpha);
			}

			engine->UndoMove();

			if (quitEarly)
				break;

			auto elapsed = duration_cast<milliseconds>(steady_clock::now() - startTime).count();
			if (timePerTurn >= 0 && elapsed >= timePerTurn)
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
		if (timePerTurn >= 0 && elapsed >= timePerTurn)
			break;
	}

	std::cout << "Making " << MoveToUCI(bestMove) << " with score " << bestScore << '\n';
	nodesSearched = 0;

	Piece movingPiece = engine->GetBoard()[GetStart(bestMove)].GetPiece();
	if (!engine->ValidMove(movingPiece, bestMove)) throw "Invalid move";

	if (!MoveIsNull(bestMove))
		return bestMove;
	else
		throw "Move was null\n";
}

int Bot::Search(int depth, int ply, int alpha, int beta)
{
	if (ply >= MAX_PLY - 1)
		return Eval(GameState::currentPlayer, engine);

	++nodesSearched;
	// Check time every 512 nodes (& faster than %)
	if ((nodesSearched & 511) == 0)
	{
		auto elapsed = duration_cast<milliseconds>(steady_clock::now() - startTime).count();
		if (timePerTurn >= 0 && elapsed >= timePerTurn)
		{
			quitEarly = true;
			return alpha;
		}
	}

	bool followingNullMove = afterNullMove; // Used for this search only
	afterNullMove = false;
	bool pvNode = (beta - alpha) > 1;

	if (engine->IsDraw()) return 0;

	if (depth <= 0 || engine->IsOver()) return Qsearch(alpha, beta, ply);

	if (!pvNode && !engine->InCheck(GameState::currentPlayer))
	{
		int eval = Eval(GameState::currentPlayer, engine);

		// Razoring
		const int RAZORING_MARGIN = 300;
		if (depth <= 2 && !followingNullMove &&
			(eval + (RAZORING_MARGIN * depth) < alpha))
		{
			int qEval = Qsearch(alpha, beta, ply);
			if (qEval < alpha) return qEval;
		}
	}

	uint64_t key = engine->GetZobristKey();
	int ttScore = 0;
	uint32_t ttMove = 0;
	if (tt.ttProbe(key, depth, alpha, beta, ttScore, ttMove))
		return ScoreFromTT(ttScore, ply);

	Color movingColor = GameState::currentPlayer;
	bool inCheck = engine->InCheck(movingColor);
	const BitboardBoard& board = engine->GetBitboardBoard();
	bool hasQueens = (board.pieceBitboards[0][static_cast<int>(Pieces::QUEEN) - 1] |
		board.pieceBitboards[1][static_cast<int>(Pieces::QUEEN) - 1]) != 0;
	bool foundLegal = false;

	// Null move reduction
	if (depth >= 3 && !inCheck && hasQueens && !GameState::endgame && !followingNullMove)
	{
		int reduction = 3;
		engine->MakeNullMove();
		afterNullMove = true;
		previousMoves[ply + 1] = Move();
		int nullScore = -Search(std::max(0, depth - 1 - reduction), ply + 1, -beta, -beta + 1);
		engine->UndoNullMove();

		if (quitEarly) return alpha;

		if (nullScore >= beta)
			return beta; // Fail-hard beta cutoff
	}

	std::vector<Move> pseudoMoves;
	pseudoMoves.reserve(256);
	Movegen::GetAllMoves(pseudoMoves, movingColor, engine->GetBitboardBoard(), engine);

	std::vector<Move>& moves = moveLists[ply];
	moves.clear();

	for (const Move& move : pseudoMoves)
	{
		engine->MakeMove(move);
		if (!engine->InCheck(movingColor))
			moves.push_back(move);
		engine->UndoMove();
	}

	OrderMoves(moves, ply, false);

	// If we probed a move from TT, move it to the front
	if (ttMove)
	{
		auto it = std::find_if(moves.begin(), moves.end(), [&](const Move& mv) {
			return mv == ttMove;
			});
		if (it != moves.end())
			std::swap(moves[0], *it);
	}

	int originalAlpha = alpha;
	uint32_t bestMove32 = 0;
	uint8_t flag = TT_ALPHA;

	int bestEval = -INF;

	int moveCount = 0;

	// Loop through moves
	for (const Move& move : moves)
	{
		bool captureMove = MoveIsCapture(move, engine->GetBitboardBoard());
		bool isKillerMove = (move == killerMoves[ply][0] || move == killerMoves[ply][1]);
		bool isHistoryBest = (ScoreMove(move, ply, false) >= killerBonus);

		engine->MakeMove(move);

		++moveCount;
		bool opponentInCheck = engine->InCheck(Opponent(movingColor));
		foundLegal = true;

		int eval;

		int newDepth = depth - 1;
		int reduction = 0;
		if (moveCount >= 4 && depth >= 3 && !inCheck && !captureMove &&
			GetPromotion(move) == static_cast<int>(Pieces::NONE) &&
			!opponentInCheck && !isKillerMove && !isHistoryBest)
		{
			reduction = 1;
			if (moveCount >= 8 && depth >= 5)
				reduction = 2;
		}

		previousMoves[ply + 1] = move;

		if (moveCount == 1)
		{
			eval = -Search(newDepth, ply + 1, -beta, -alpha);
		}
		else
		{
			eval = -Search(newDepth - reduction, ply + 1, -alpha - 1, -alpha);
			if (!quitEarly && reduction > 0 && eval > alpha)
				eval = -Search(newDepth, ply + 1, -alpha - 1, -alpha);
			if (!quitEarly && eval > alpha && eval < beta)
				eval = -Search(newDepth, ply + 1, -beta, -alpha);
		}

		engine->UndoMove();

		if (quitEarly) return alpha;

		if (eval > bestEval)
		{
			bestEval = eval;
			bestMove32 = move;
		}

		alpha = std::max(alpha, eval);
		if (alpha >= beta)
		{
			if (!captureMove && (Pieces)GetPromotion(move) == Pieces::NONE && !opponentInCheck) // Beta cutoff = good
			{
				if (killerMoves[ply][0] != move)
				{
					killerMoves[ply][1] = killerMoves[ply][0];
					killerMoves[ply][0] = move;
				}

				int from = GetStart(move);
				int to = GetEnd(move);
				Piece movingPiece = engine->GetBoard()[from].GetPiece();
				int pieceType = (int)movingPiece.GetType() - 1;

				historyHeuristic[(int)movingPiece.GetColor()][pieceType][from][to] =
					std::min(historyHeuristic[(int)movingPiece.GetColor()][pieceType][from][to] + depth * depth * 100, 32767);

				// Countermove heuristic
				Move previousMove = previousMoves[ply];
				if (ply > 0 && !MoveIsNull(previousMove))
				{
					int prevFrom = GetStart(previousMove);
					int prevTo = GetEnd(previousMove);
					counterMoves[(int)movingPiece.GetColor()][prevFrom][prevTo] = move;
				}
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

	tt.ttStore(key, depth, ScoreToTT(bestScore, ply), bestMove32, flag);
	return bestScore;
}

int Bot::Qsearch(int alpha, int beta, int ply, int qply)
{
	if (ply >= MAX_PLY - 1)
		return Eval(GameState::currentPlayer, engine);

	++nodesSearched;
	if ((nodesSearched & 511) == 0) // Check time every 512 nodes
	{
		auto elapsed = duration_cast<milliseconds>(steady_clock::now() - startTime).count();
		if (timePerTurn >= 0 && elapsed >= timePerTurn)
		{
			quitEarly = true;
			return alpha;
		}
	}

	Color movingColor = GameState::currentPlayer;
	if (engine->IsDraw())
		return 0;

	bool inCheck = engine->InCheck(movingColor);
	if (!inCheck)
	{
		int standPat = Eval(movingColor, engine);
		if (standPat >= beta)
			return beta;
		if (standPat > alpha)
			alpha = standPat;
	}

	std::vector<Move>& moves = qMoveLists[ply];
	Movegen::GetAllMoves(moves, movingColor, engine->GetBitboardBoard(), engine,
		!inCheck, !inCheck && qply >= 2);
	OrderMoves(moves, ply, true);

	bool foundLegal = false;

	for (const Move& move : moves)
	{
		engine->MakeMove(move);
		if (engine->InCheck(movingColor))
		{
			engine->UndoMove();
			continue;
		}
		foundLegal = true;

		int score = -Qsearch(-beta, -alpha, ply + 1, qply + 1);
		engine->UndoMove();

		if (quitEarly)
			return alpha;

		if (score >= beta)
			return beta; // Cutoff

		if (score > alpha)
			alpha = score;
	}

	if (inCheck && !foundLegal)
		return -(MATE_VAL - ply);

	return alpha;
}