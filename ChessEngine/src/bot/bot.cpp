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
constexpr int NO_EVAL = -99999;

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

static inline int PieceIndex(Piece p)
{
	return (IsWhite(p.GetColor()) ? 0 : 6) + (int)p.GetType() - 1;
}

// History "gravity". Idk what this does
static inline void UpdateHistory(int16_t& entry, int bonus)
{
	int v = entry;
	v += bonus - v * std::abs(bonus) / 16384;
	entry = v;
}

static inline void UpdateHistory(int& entry, int bonus)
{
	entry += bonus - entry * std::abs(bonus) / 16384;
}

bool Bot::SEE(Move move, int threshold)
{
	const int pieceValues[] = { 0, 100, 320, 330, 500, 900, 0 };

	if (IsEnPassant(move) || IsCastle(move) || GetPromotion(move) != (int)Pieces::NONE)
		return 0 >= threshold;

	int from = GetStart(move);
	int to = GetEnd(move);
	const BitboardBoard& b = engine->GetBitboardBoard();
	Piece movingPiece = engine->GetBoard()[from].GetPiece();

	// We capture and they don't recapture
	int swap = pieceValues[(int)engine->GetBoard()[to].GetPiece().GetType()] - threshold;
	if (swap < 0) return false;

	// They capture and we can't
	swap = pieceValues[(int)movingPiece.GetType()] - swap;
	if (swap <= 0) return true;

	// Get bitboard occ for pieces of a certain type
	auto getPieceOccOfType = [&](Pieces piece) { return b.pieceBitboards[0][(int)piece - 1] | b.pieceBitboards[1][(int)piece - 1]; };
	Bitboard straight = getPieceOccOfType(Pieces::ROOK) | getPieceOccOfType(Pieces::QUEEN);
	Bitboard diagonal = getPieceOccOfType(Pieces::BISHOP) | getPieceOccOfType(Pieces::QUEEN);
	// Get diagonal attackers
	auto diagonalFrom = [&](Bitboard occ) {return Movegen::BishopAttacks(to, occ); };
	// Get straight attackers
	auto straightFrom = [&](Bitboard occ) {return Movegen::RookAttacks(to, occ); };

	// Remove captured and capturing piece
	Bitboard occupied = b.occupied ^ (1ULL << from) ^ (1ULL << to);

	Bitboard attackers =
		(Movegen::GetPawnAttacks()[1][to] & b.pieceBitboards[0][(int)Pieces::PAWN - 1]) | // White pawns
		(Movegen::GetPawnAttacks()[0][to] & b.pieceBitboards[1][(int)Pieces::PAWN - 1]) | // Black pawns
		(Movegen::GetKnightAttacks()[to] & getPieceOccOfType(Pieces::KNIGHT)) |
		(Movegen::GetKingAttacks()[to] & getPieceOccOfType(Pieces::KING)) |
		(diagonalFrom(occupied) & diagonal) |
		(straightFrom(occupied) & straight);

	int side = IsWhite(movingPiece.GetColor()) ? 0 : 1;
	int result = 1; // If the side that played the move is happy with the result, swaps every time

	while (true)
	{
		side ^= 1;
		attackers &= occupied; // Drop captured pieces
		Bitboard ours = b.allPieces[side] & attackers;
		if (!ours) break; // Can't recapture so exchange is over
		result ^= 1;

		Bitboard bb;
		if ((bb = ours & b.pieceBitboards[side][(int)Pieces::PAWN - 1]))
		{
			if ((swap = pieceValues[(int)Pieces::PAWN] - swap) < result) break;
			occupied ^= bb & (~bb + 1);                   // Remove the capturing piece (lowest bit)
			attackers |= diagonalFrom(occupied) & diagonal; // A bishop/queen behind it can now join in
		}
		else if ((bb = ours & b.pieceBitboards[side][(int)Pieces::KNIGHT - 1]))
		{
			if ((swap = pieceValues[(int)Pieces::KNIGHT] - swap) < result) break;
			occupied ^= bb & (~bb + 1);
		}
		else if ((bb = ours & b.pieceBitboards[side][(int)Pieces::BISHOP - 1]))
		{
			if ((swap = pieceValues[(int)Pieces::BISHOP] - swap) < result) break;
			occupied ^= bb & (~bb + 1);
			attackers |= diagonalFrom(occupied) & diagonal;
		}
		else if ((bb = ours & b.pieceBitboards[side][(int)Pieces::ROOK - 1]))
		{
			if ((swap = pieceValues[(int)Pieces::ROOK] - swap) < result) break;
			occupied ^= bb & (~bb + 1);
			attackers |= straightFrom(occupied) & straight;
		}
		else if ((bb = ours & b.pieceBitboards[side][(int)Pieces::QUEEN - 1]))
		{
			if ((swap = pieceValues[(int)Pieces::QUEEN] - swap) < result) break;
			occupied ^= bb & (~bb + 1);
			attackers |= (diagonalFrom(occupied) & diagonal) | (straightFrom(occupied) & straight);
		}
		else // Only the king is left: it can recapture only if the other side has nothing left
			return (attackers & ~b.allPieces[side]) ? (result ^ 1) : result;
	}

	return result;
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

	// Fill LMR table with values
	for (int d = 0; d < 64; ++d)
		for (int m = 0; m < 64; ++m)
			// Deeper move and deeper depth = bigger reduction
			lmrTable[d][m] = (d == 0 || m == 0) ? 0 : int(0.75 + std::log(d) * std::log(m) / 2.25); // Standard tuned magic numbers

	contHistory.assign(768 * 768, 0);
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
	int promotionBonus = promotion == (int)Pieces::NONE ? 0 :
		PieceValue(Piece((Pieces)promotion, moved.GetColor())) - PieceValue(Piece(Pieces::PAWN, moved.GetColor()));

	// MVV-LVA: Most Valuable Victim - Least Valuable Attacker
	if (captured.GetType() != Pieces::NONE)
	{
		int capScore = PieceValue(captured) * 16 - PieceValue(moved) + promotionBonus
			+ captureHistory[PieceIndex(moved)][GetEnd(move)][(int)captured.GetType() - 1] / 8;

		if (onlyMVVLVA || SEE(move, 0)) return captureBonus + capScore;
		return -captureBonus + capScore;
	}


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

	int score = historyHeuristic[(int)moved.GetColor()][(int)moved.GetType() - 1][GetStart(move)][GetEnd(move)];

	// Continuation history: how this move has done as a reply to 1 and 2 plies ago historically
	SearchStackEntry* ss = &searchStack[ply + 4];
	int current = PieceIndex(moved) * 64 + GetEnd(move);
	for (int back = 1; back <= 2; ++back)
	{
		SearchStackEntry* prev = ss - back;
		if (prev->pieceIndex >= 0)
		{
			score += contHistory[(prev->pieceIndex * 64 + GetEnd(prev->move)) * 768 + current];
		}
	}
	return score;
}

// Orders moves in-place, best first
void Bot::OrderMoves(std::vector<Move>& moves, int ply, bool onlyMVVLVA, Move firstMove)
{
	// Score every move once and keep the scores in a parallel array. Calling ScoreMove inside
	// the sort comparator re-scored both moves on every comparison (~2 * n * log2(n) calls).
	const size_t n = moves.size();
	thread_local std::vector<int> scores; // Reused between calls, so no allocation after warm-up
	scores.resize(n);
	for (size_t i = 0; i < n; ++i)
		scores[i] = ScoreMove(moves[i], ply, onlyMVVLVA);

	// Insertion sort, highest score first, moving each move together with its score.
	// Move lists are short, where this is faster than std::sort. Equal scores keep their order.
	for (size_t i = 1; i < n; ++i)
	{
		const int s = scores[i];
		const Move m = moves[i];
		size_t j = i;
		while (j > 0 && scores[j - 1] < s)
		{
			scores[j] = scores[j - 1];
			moves[j] = moves[j - 1];
			--j;
		}
		scores[j] = s;
		moves[j] = m;
	}

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

int& Bot::CaptureHistoryEntry(Move move)
{
	Piece movingPiece = engine->GetBoard()[GetStart(move)].GetPiece();
	int capturedPiece = IsEnPassant(move) ? (int)Pieces::PAWN : (int)engine->GetBoard()[GetEnd(move)].GetPiece().GetType();
	return captureHistory[PieceIndex(movingPiece)][GetEnd(move)][capturedPiece - 1];
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
	memset(previousMoves, 0, sizeof(previousMoves));

	memset(counterMoves, 0, sizeof(counterMoves));

	tt.Clear();

	std::fill(contHistory.begin(), contHistory.end(), 0);

	memset(captureHistory, 0, sizeof(captureHistory));
}

// One UCI "info" line per finished iteration. CuteChess shows it and uses the score to adjudicate
// games. It's also a quick way to compare two builds: if they reach the same depth with the same
// node count, they ran the same search, and only the time should differ.
void Bot::PrintInfo(int depth, int score, Move rootMove)
{
	long long ms = duration_cast<milliseconds>(steady_clock::now() - startTime).count();
	long long nps = (long long)nodesSearched * 1000 / std::max(ms, 1LL);

	std::cout << "info depth " << depth;
	if (std::abs(score) >= MATE_VAL - MAX_PLY)
	{
		// Mate scores count plies; UCI wants full moves, negative when we're the side getting mated
		int plies = MATE_VAL - std::abs(score);
		std::cout << " score mate " << (score > 0 ? (plies + 1) / 2 : -(plies / 2));
	}
	else
		std::cout << " score cp " << score;

	std::cout << " nodes " << nodesSearched << " nps " << nps << " time " << ms
		<< " pv " << GetPV(rootMove, depth) << std::endl; // endl flushes, so the GUI sees it right away
}

// The principal variation: the root move, then the best move stored in the TT for each position
// after it. Every move is checked to be legal before it's played. The walk stops at the first
// missing entry, at a draw (so a repetition can't loop), or after maxLength moves, and then
// every move is undone again.
std::string Bot::GetPV(Move rootMove, int maxLength)
{
	std::string pv = MoveToUCI(rootMove);
	engine->MakeMove(rootMove);
	int played = 1;

	std::vector<Move> moves;
	while (played < maxLength && !engine->IsDraw())
	{
		Move move = tt.GetBestMove(engine->GetZobristKey());
		if (MoveIsNull(move))
			break;

		Color us = GameState::currentPlayer;
		Movegen::GetAllMoves(moves, us, engine->GetBitboardBoard(), engine);
		if (std::find(moves.begin(), moves.end(), move) == moves.end())
			break; // Not a move in this position

		engine->MakeMove(move);
		if (engine->InCheck(us)) // Illegal
		{
			engine->UndoMove();
			break;
		}

		pv += ' ';
		pv += MoveToUCI(move);
		++played;
	}

	while (played-- > 0)
		engine->UndoMove();

	return pv;
}

Move Bot::GetMoveUCI(int timeForMove, int softTime)
{
	timePerTurn = timeForMove;
	this->softTime = softTime;
	Move move = GetMove();
	return move;
}

Move Bot::GetMove()
{
	Move bookMove = GetBookMove(engine, "res/openings.bin");
	if (!MoveIsNull(bookMove))
	{
		std::cout << "info string Using opening move" << std::endl;
		return bookMove; // Play instantly
	}

	quitEarly = false;
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

	// Clear search stack
	for (SearchStackEntry& se : searchStack)
	{
		se = SearchStackEntry();
		se.staticEval = NO_EVAL;   // "unknown" for the spare entries and the root
	}

	int maxDepth = 8;
	Move bestMove = Move();
	int bestScore = -INF;

	if (tb.Probeable(engine->GetBitboardBoard()))
	{
		Move tbMove = tb.GetMove(engine);
		if (!MoveIsNull(tbMove))
			return tbMove; // A null move means the probe failed: search normally instead
	}

	for (int depth = 1; depth <= maxDepth; ++depth)
	{
		rootDepth = depth;
		bool foundLegal = false;

		Move currentBestMove = Move();
		int currentBestScore = -INF;

		std::vector<Move> moves = moveLists[0];
		Movegen::GetAllMoves(moves, botColor, engine->GetBitboardBoard(), engine);
		if (!MoveIsNull(bestMove)) // Current best move first to help with pruning
			OrderMoves(moves, 0, false, bestMove);
		else
		{
			OrderMoves(moves, 0, false);
			if (!moves.empty()) bestMove = moves[0]; // If not enough time to find move, set default after sorting
		}

		// Aspiration window
		// From depth 5, search a narrow window around last iteration score
		// Mostly lands inside so more pruning
		int delta = 25;
		int windowAlpha = -INF;
		int windowBeta = INF;

		if (depth >= 5 && std::abs(bestScore) < MATE_VAL - MAX_PLY)
		{
			windowAlpha = bestScore - delta;
			windowBeta = bestScore + delta;
		}

		while (true)
		{
			int alpha = windowAlpha;
			int beta = windowBeta;
			int moveCount = 0;
			foundLegal = false;
			currentBestMove = Move();
			currentBestScore = -INF;

			for (const Move& move : moves)
			{
				searchStack[4].move = move;
				searchStack[4].pieceIndex = PieceIndex(engine->GetBoard()[GetStart(move)].GetPiece());

				engine->MakeMove(move);

				if (engine->InCheck(botColor)) { engine->UndoMove(); continue; }

				foundLegal = true;
				++moveCount;
				previousMoves[1] = move;

				// PVS, assuming good move ordering
				int score;
				if (moveCount == 1)
				{
					score = -Search(depth - 1, 1, -beta, -alpha);
				}
				else
				{
					score = -Search(depth - 1, 1, -alpha - 1, -alpha);
					if (!quitEarly && score > alpha && score < beta) // Move was good, full research
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

			if (quitEarly || !foundLegal)
				break;

			if (currentBestScore <= windowAlpha)
			{
				// Fail low. Scored below window, so lower bottom
				windowBeta = (windowAlpha + windowBeta) / 2;
				windowAlpha = std::max(currentBestScore - delta, -INF);
			}
			else if (currentBestScore >= windowBeta)
			{
				// Fail high. Score above window, so raise top and search that first
				windowBeta = std::min(currentBestScore + delta, INF);
				std::vector<Move>::iterator it = std::find(moves.begin(), moves.end(), currentBestMove);
				if (it != moves.end())
					std::rotate(moves.begin(), it, it + 1);
			}
			else
				break; // Score inside window

			delta += delta / 2; // Widen window faster every time
			if (delta > 500) { windowAlpha = -INF; windowBeta = INF; }
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

		// Report the finished iteration (an unfinished one has already left the loop)
		if (foundLegal)
			PrintInfo(depth, bestScore, bestMove);

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

		// The next depth would probably take 2-3x longer, so don't start it
		if (softTime >= 0 && elapsed >= softTime)
			break;
	}

	std::cout << "info string Making " << MoveToUCI(bestMove) << " with score " << bestScore << std::endl;
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
	if (ply >= MAX_PLY - 1) // Per-ply buffers end at MAX_PLY
		return Eval(GameState::currentPlayer, engine);

	++nodesSearched;
	// Check time every 512 nodes (& faster than %)
	if ((nodesSearched & 511) == 0)
	{
		auto elapsed = duration_cast<milliseconds>(steady_clock::now() - startTime).count();
		if (timePerTurn >= 0 && elapsed >= timePerTurn) // Negative = no limit ("go infinite")
		{
			quitEarly = true;
			return alpha;
		}
	}
	Color movingColor = GameState::currentPlayer;

	// +4 for buffer so we never need to worry about indexing
	SearchStackEntry* ss = &searchStack[ply + 4];

	bool followingNullMove = (ss - 1)->nullMove; // Used for this search only
	bool pvNode = (beta - alpha) > 1;
	bool inCheck = engine->InCheck(movingColor);

	if (engine->IsDraw() || engine->IsRepetitionDraw(ply)) return 0;

	if (depth <= 0 || engine->IsOver()) return Qsearch(alpha, beta, ply);

	uint64_t key = engine->GetZobristKey();
	int ttScore = 0;
	uint32_t ttMove = 0; // ttProbe only writes these on a key match
	// No TT cutoffs in PV nodes: they would cut the principal variation short. The probe still
	// runs so the stored move can be searched first.
	// TODO: Use this score instead of staticEval
	bool excluded = ss->excludedMove != 0;
	if (tt.ttProbe(key, depth, alpha, beta, ttScore, ttMove) && !pvNode && !excluded)
		return ScoreFromTT(ttScore, ply);

	int staticEval = inCheck ? NO_EVAL : Eval(movingColor, engine);
	ss->staticEval = staticEval;
	// If the eval has improved since our last term. Used for more aggressive pruning
	bool improving = false;
	if (!inCheck)
	{
		if ((ss - 2)->staticEval != NO_EVAL)
			improving = staticEval > (ss - 2)->staticEval;
		else if ((ss - 4)->staticEval != NO_EVAL)
			improving = staticEval > (ss - 4)->staticEval;
		else
			improving = true;
	}

	if (!pvNode && !inCheck && !excluded)
	{
		// Razoring
		const int RAZORING_MARGIN = 300;
		if (depth <= 2 && !followingNullMove &&
			(staticEval + (RAZORING_MARGIN * depth) < alpha))
		{
			int qEval = Qsearch(alpha - 1, alpha, ply); // Same position, so same ply
			if (qEval < alpha) return qEval;
		}
	}

	// Reverse futility pruning
	int RFP_MARGIN = 80; // 80 centipawns to assume to opponent can't recover
	if (!pvNode && !inCheck && !excluded && depth <= 7
		&& staticEval - RFP_MARGIN * std::max(0, depth - improving) >= beta
		&& staticEval < MATE_VAL - MAX_PLY)
		return (staticEval + beta) / 2; // Blend to be a bit safer

	// Null move reduction
	const BitboardBoard& board = engine->GetBitboardBoard();
	// Most zugzwang happens in pawn-king endgames, so avoid that
	int c = IsWhite(movingColor) ? 0 : 1;
	bool hasNonPawn = (board.pieceBitboards[c][(int)Pieces::KNIGHT - 1] |
		board.pieceBitboards[c][(int)Pieces::BISHOP - 1] |
		board.pieceBitboards[c][(int)Pieces::ROOK - 1] |
		board.pieceBitboards[c][(int)Pieces::QUEEN - 1]) != 0;

	if (depth >= 3 && !pvNode && !inCheck && !excluded && hasNonPawn && staticEval >= beta && !(ss - 1)->nullMove)
	{
		// Adaptive reduction
		int reduction = 2 + depth / 4 + std::min((staticEval - beta) / 200, 2);

		ss->nullMove = true;
		ss->move = 0;
		ss->pieceIndex = -1;

		engine->MakeNullMove();
		previousMoves[ply + 1] = Move();
		int nullScore = -Search(depth - reduction, ply + 1, -beta, -beta + 1);
		engine->UndoNullMove();

		ss->nullMove = false;
		if (quitEarly) return alpha;

		if (nullScore >= beta)
			return nullScore >= MATE_VAL - MAX_PLY ? beta : nullScore; // Fail-hard beta cutoff
	}

	// ProbCut, if a capture beats beta by a big margin even in a shallow search,
	// the full search would probably fail high too
	const int probCutBeta = beta + 200;
	if (!pvNode && !inCheck && !excluded && depth >= 5 && std::abs(beta) < MATE_VAL - MAX_PLY)
	{
		TTEntry entry;
		bool ttSaysNo = tt.ttLookup(key, entry) && entry.depth >= depth - 3
			&& ScoreFromTT(entry.score, ply) < probCutBeta;

		if (!ttSaysNo)
		{
			std::vector<Move> captures;
			Movegen::GetAllMoves(captures, movingColor, engine->GetBitboardBoard(), engine, true, true);
			OrderMoves(captures, ply, true);

			for (const Move& move : captures)
			{
				// Only captures that win enough material to probably get there
				if (!SEE(move, probCutBeta - staticEval))
					continue;

				ss->move = move;
				ss->pieceIndex = PieceIndex(engine->GetBoard()[GetStart(move)].GetPiece());

				engine->MakeMove(move);
				if (engine->InCheck(movingColor))
				{
					engine->UndoMove();
					continue;
				}
				previousMoves[ply + 1] = move;

				// Cheap qsearch, and confirm with shallow search if it passes
				int score = -Qsearch(-probCutBeta, -probCutBeta + 1, ply + 1);
				if (!quitEarly && score >= probCutBeta)
					score = -Search(depth - 4, ply + 1, -probCutBeta, -probCutBeta + 1);
				engine->UndoMove();

				if (quitEarly) return alpha;
				if (score >= probCutBeta)
				{
					tt.ttStore(key, depth - 3, ScoreToTT(score, ply), move, TT_BETA);
					return score;
				}
			}
		}
	}

	// Singular extension. If every move except TT fails below TT score
	// the TT move is the only good move so search it one ply deeper
	int singularExtension = 0;
	TTEntry ttEntry;
	if (!excluded && ttMove && depth >= 6 && ply < 2 * rootDepth
		&& tt.ttLookup(key, ttEntry)
		&& ttEntry.depth >= depth - 3
		&& ttEntry.flag != TT_ALPHA // Score is exact or lower bound
		&& std::abs(ttEntry.score) < MATE_VAL - MAX_PLY)
	{
		int singularBeta = ttEntry.score - 2 * depth;
		int singularDepth = (depth - 1) / 2;

		ss->excludedMove = ttMove;
		int score = Search(singularDepth, ply, singularBeta - 1, singularBeta);
		ss->excludedMove = 0;
		if (quitEarly) return alpha;

		if (score < singularBeta)
			singularExtension = 1; // No move comes close, extend the TT move
		else if (singularBeta >= beta)
			return singularBeta; // Multicut. Without TT move, anothre move beats beta
	}

	// Pseudo legal
	std::vector<Move>& moves = moveLists[ply];
	//std::vector<Move> pseudoMoves;
	//pseudoMoves.reserve(256);
	Movegen::GetAllMoves(moves, movingColor, engine->GetBitboardBoard(), engine);

	//moves.clear();

	//for (const Move& move : pseudoMoves)
	//{
	//	engine->MakeMove(move);
	//	if (!engine->InCheck(movingColor))
	//		moves.push_back(move);
	//	engine->UndoMove();
	//}

	OrderMoves(moves, ply, false);

	// If we probed a move from TT, move it to the front
	if (ttMove)
	{
		std::vector<Move>::iterator it = std::find(moves.begin(), moves.end(), (Move)ttMove);
		if (it != moves.end())
			std::rotate(moves.begin(), it, it + 1); // TT move first, shift others down
	}

	int originalAlpha = alpha;
	uint32_t bestMove32 = 0;
	uint8_t flag = TT_ALPHA;

	int bestEval = -INF;

	int moveCount = 0;
	bool foundLegal = false;

	// For continuation history, record quiet moves that didn't cause a cutoff
	Move quietsTried[64];
	int quietCount = 0;

	// For capture history, record captures that didn't cause a cutoff
	Move capturesTried[32];
	int captureCount = 0;

	// For lmp
	int lmpLimit = (3 + depth * depth) / (improving ? 1 : 2);
	bool skipQuiets = false;

	// Loop through moves
	for (const Move& move : moves)
	{
		// Skip excluded move
		if (move == ss->excludedMove)
			continue;

		bool captureMove = MoveIsCapture(move, engine->GetBitboardBoard());
		bool isKillerMove = (move == killerMoves[ply][0] || move == killerMoves[ply][1]);
		bool isQuiet = !captureMove && GetPromotion(move) == (int)Pieces::NONE;

		// Late move pruning
		if (!pvNode && !inCheck && isQuiet && bestEval > -(MATE_VAL - MAX_PLY) && !engine->GivesCheck(move))
		{
			if (skipQuiets) continue;
			if (depth <= 8 && moveCount >= lmpLimit) { skipQuiets = true; continue; }

			// Futility pruning: even a good quiet move can't lift the static eval up to alpha
			int lmrDepth = std::max(0, depth - 1 - lmrTable[std::min(depth, 63)][std::min(moveCount + 1, 63)]);
			if (lmrDepth <= 6 && staticEval + 100 + 110 * lmrDepth <= alpha)
			{
				skipQuiets = true;
				continue;
			}

			// SEE pruning quiet moves. Skip quiet moves that hang material
			if (lmrDepth <= 6 && !SEE(move, -30 * lmrDepth * lmrDepth))
				continue;
		}

		// SEE pruning captures. Skip captures that lose a lot of material
		if (!pvNode && !inCheck && !isQuiet && depth <= 6 && bestEval > -(MATE_VAL - MAX_PLY)
			&& !SEE(move, -100 * depth))
			continue;

		ss->move = move;
		ss->pieceIndex = PieceIndex(engine->GetBoard()[GetStart(move)].GetPiece());

		engine->MakeMove(move);
		// Check for illegal moves
		if (engine->InCheck(movingColor))
		{
			engine->UndoMove();
			continue;
		}

		++moveCount;
		bool opponentInCheck = engine->InCheck(Opponent(movingColor));
		foundLegal = true;

		int eval;

		int newDepth = depth - 1;
		if (move == ttMove)
			newDepth += singularExtension;

		previousMoves[ply + 1] = move;

		// Reduce later moves more because we assume they are worse from move ordering
		int reduction = 0;
		if (moveCount > (pvNode ? 2 : 1) && depth >= 3 && !inCheck &&
			!captureMove && GetPromotion(move) == (int)Pieces::NONE)
		{
			// Get fine tuned adjustment
			reduction = lmrTable[std::min(depth, 63)][std::min(moveCount, 63)];

			// Tuned adjustments
			if (!pvNode)         reduction++;
			if (!improving)		 reduction++;
			if (opponentInCheck) reduction--;
			if (isKillerMove)    reduction--;

			// Stop from jumping into qsearch
			reduction = std::clamp(reduction, 0, newDepth - 1);
		}

		// Principal variation search (see GetMove). In a non-PV node the window is already
		// (alpha, alpha + 1), so the re-search condition can never be true there.
		if (moveCount == 1)
		{
			eval = -Search(newDepth, ply + 1, -beta, -alpha);
		}
		else
		{
			eval = -Search(newDepth - reduction, ply + 1, -alpha - 1, -alpha);

			// Research, still null window
			if (!quitEarly && reduction > 0 && eval > alpha)
				eval = -Search(newDepth, ply + 1, -alpha - 1, -alpha);

			// Full PV research
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
			int bonus = std::min(16 * depth * depth + 64 * depth - 64, 1500);

			// Capture history, reward a capture that cut off, and penalize the cutoff move
			if (captureMove)
				UpdateHistory(CaptureHistoryEntry(move), bonus);
			for (int i = 0; i < captureCount; ++i)
				UpdateHistory(CaptureHistoryEntry(capturesTried[i]), -bonus);

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

				// Reward the move that caused the cutoff
				UpdateHistory(historyHeuristic[(int)movingPiece.GetColor()][pieceType][from][to], bonus);

				// Penalise the quiet moves searched before it that didn't cut off
				for (int i = 0; i < quietCount; ++i)
				{
					Move q = quietsTried[i];
					Piece p = engine->GetBoard()[GetStart(q)].GetPiece();
					UpdateHistory(historyHeuristic[(int)p.GetColor()][(int)p.GetType() - 1][GetStart(q)][GetEnd(q)], -bonus);
				}

				// Countermove heuristic
				Move prev = previousMoves[ply]; // The move that led to this node (moveLists[ply - 1][0] was just the parent's first move)
				if (ply > 0 && !MoveIsNull(prev))
				{
					int prevFrom = GetStart(prev);
					int prevTo = GetEnd(prev);
					counterMoves[(int)movingPiece.GetColor()][prevFrom][prevTo] = move;
				}

				// Continuation history: reward the cutoff move, penalise the quiet moves tried before it
				auto updateCont = [&](Move m, int b)
					{
						int current = PieceIndex(engine->GetBoard()[GetStart(m)].GetPiece()) * 64 + GetEnd(m);
						for (int back = 1; back <= 2; ++back)
						{
							SearchStackEntry* prev = ss - back;
							if (prev->pieceIndex >= 0)
								UpdateHistory(contHistory[(prev->pieceIndex * 64 + GetEnd(prev->move)) * 768 + current], b);
						}
					};
				updateCont(move, bonus);
				for (int i = 0; i < quietCount; ++i)
					updateCont(quietsTried[i], -bonus);
			}
			break;
		}

		if (isQuiet && quietCount < 64)
			quietsTried[quietCount++] = move;
		else if (captureMove && captureCount < 32)
			capturesTried[captureCount++] = move;
	}

	// Check checkmate/stalemate
	if (!foundLegal)
	{
		// Excluded was only legal move
		if (excluded) return alpha;
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

	if (!excluded)
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

	bool pvNode = (beta - alpha) > 1;
	uint64_t key = engine->GetZobristKey();
	int ttScore = 0;
	uint32_t ttMove = 0;
	if (tt.ttProbe(key, 0, alpha, beta, ttScore, ttMove) && !pvNode)
		return ScoreFromTT(ttScore, ply);

	auto storeTT = [&](int score, uint8_t flag, Move move)
	{
		TTEntry existing;
		if (tt.ttLookup(key, existing) && existing.depth > 0)
			return;
		tt.ttStore(key, 0, ScoreToTT(score, ply), move, flag);
	};

	const int originalAlpha = alpha;
	Move bestMove = Move();

	int bestScore = -INF;

	bool inCheck = engine->InCheck(movingColor);
	if (!inCheck)
	{
		int standPat = Eval(movingColor, engine);
		if (standPat >= beta)
		{
			storeTT(standPat, TT_BETA, 0);
			return standPat;
		}
		bestScore = standPat;
		if (standPat > alpha)
			alpha = standPat;
	}

	std::vector<Move>& moves = qMoveLists[ply];
	Movegen::GetAllMoves(moves, movingColor, engine->GetBitboardBoard(), engine,
		!inCheck, !inCheck && qply >= 2);
	OrderMoves(moves, ply, true);

	if (ttMove)
	{
		std::vector<Move>::iterator it = std::find(moves.begin(), moves.end(), (Move)ttMove);
		if (it != moves.end())
			std::rotate(moves.begin(), it, it + 1);
	}

	bool foundLegal = false;

	for (const Move& move : moves)
	{
		// Losing exchange
		if (!inCheck && !SEE(move, 0))
			continue;

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
		{
			storeTT(score, TT_BETA, move);
			return score; // Cutoff
		}

		if (score > bestScore)
		{
			bestScore = score;
			bestMove = move;
		}

		if (score > alpha)
			alpha = score;
	}

	if (inCheck && !foundLegal)
		return -(MATE_VAL - ply);

	storeTT(bestScore, bestScore > originalAlpha ? TT_EXACT : TT_ALPHA, bestMove);
	return bestScore;
}