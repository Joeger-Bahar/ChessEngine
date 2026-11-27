#include "bot.hpp"

#include "core/boardCalculator.hpp"
#include "core/movegen.hpp"

#include "tablebase.hpp"

#include <limits>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <bit>
#include <Windows.h>
#undef min
#undef max
using namespace std::chrono;

constexpr int INF = std::numeric_limits<int>::max() / 4;
constexpr int MATE_VAL = 1000000;
constexpr int NO_EVAL = -99999;
constexpr int TB_WIN = MATE_VAL - 2 * MAX_PLY; // Tablebase win, above any eval but below mate

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

// Shared by search threads
static std::atomic<bool> helpersStop{ false }; // Set to true when search is done
static std::atomic<long long> helperNodes{ 0 }; // Nodes searched by helpers, for info

void Bot::ResetHelperState()
{
	helpersStop.store(false);
	helperNodes.store(0);
}

void Bot::StopHelpers()
{
	helpersStop.store(true);
}

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

const int ttMoveScore = 10 * captureBonus; // Above every other score so TT move goes first

void Bot::ScoreMoves(const std::vector<Move>& moves, int* scores, int ply, bool onlyMVVLVA, Move ttMove)
{
	// These are the same for every move in this position
	const Square* board = engine->GetBoard();
	Color us = GameState::currentPlayer;
	Move killer1 = killerMoves[ply][0];
	Move killer2 = killerMoves[ply][1];

	Move counter = Move();
	if (ply > 0 && !MoveIsNull(previousMoves[ply]))
		counter = counterMoves[(int)us][GetStart(previousMoves[ply])][GetEnd(previousMoves[ply])];

	// Continuation history rows for the moves 1 and 2 plies ago
	SearchStackEntry* ss = &searchStack[ply + 4];
	int16_t* cont1 = nullptr;
	int16_t* cont2 = nullptr;
	if ((ss - 1)->pieceIndex >= 0)
		cont1 = &contHistory[((ss - 1)->pieceIndex * 64 + GetEnd((ss - 1)->move)) * 768];
	if ((ss - 2)->pieceIndex >= 0)
		cont2 = &contHistory[((ss - 2)->pieceIndex * 64 + GetEnd((ss - 2)->move)) * 768];

	for (size_t i = 0; i < moves.size(); ++i)
	{
		Move move = moves[i];
		if (move == ttMove)
		{
			scores[i] = ttMoveScore;
			continue;
		}
		
		Piece moved = board[GetStart(move)].GetPiece();
		Piece captured = IsEnPassant(move) ? Piece(Pieces::PAWN, Opponent(us)) : board[GetEnd(move)].GetPiece();
		int promotion = GetPromotion(move);
		int promotionBonus = promotion == (int)Pieces::NONE ? 0 : PieceValue(Piece((Pieces)promotion, moved.GetColor())) - PieceValue(Piece(Pieces::PAWN, moved.GetColor()));

		// MVV LVA
		if (captured.GetType() != Pieces::NONE)
		{
			int capScore = PieceValue(captured) * 16 - PieceValue(moved) + promotionBonus
				+ captureHistory[PieceIndex(moved)][GetEnd(move)][(int)captured.GetType() - 1] / 8;

			scores[i] = (onlyMVVLVA || SEE(move, 0)) ? captureBonus + capScore : -captureBonus + capScore;
			continue;
		}

		if (promotionBonus > 0) { scores[i] = captureBonus - 1 + promotionBonus; continue; }
		if (onlyMVVLVA)			{ scores[i] = 0;				 continue; }
		if (move == killer1)	{ scores[i] = killerBonus;		 continue; }
		if (move == killer2)	{ scores[i] = killerBonus - 100; continue; }
		if (move == counter)	{ scores[i] = killerBonus - 200; continue; } // Slightly below killer moves

		int score = historyHeuristic[(int)us][(int)moved.GetType() - 1][GetStart(move)][GetEnd(move)];

		int current = PieceIndex(moved) * 64 + GetEnd(move);
		if (cont1) score += cont1[current];
		if (cont2) score += cont2[current];
		scores[i] = score;
	}
}

static void PickMove(std::vector<Move>& moves, int* scores, size_t i)
{
	if (i >= 2 || moves.empty())
		return; // After the first two calls the list is sorted

	size_t n = moves.size();
	if (i == 0)
	{
		// Find the best move
		size_t best = 0;
		for (size_t j = 1; j < n; ++j)
			if (scores[j] > scores[best])
				best = j;

		// Bring it to the front. The moves before it shift back by one, so moves with equal
		// scores keep their order (the same order sorting the whole list gives)
		Move bestMove = moves[best];
		int bestScore = scores[best];
		for (size_t j = best; j > 0; --j)
		{
			moves[j] = moves[j - 1];
			scores[j] = scores[j - 1];
		}
		moves[0] = bestMove;
		scores[0] = bestScore;
	}
	else
	{
		// The first move didn't cut off: sort the rest once.
		// Insertion sort, highest score first, moving each move together with its score
		for (size_t a = 2; a < n; ++a)
		{
			int s = scores[a];
			Move m = moves[a];
			size_t j = a;
			while (j > 1 && scores[j - 1] < s)
			{
				scores[j] = scores[j - 1];
				moves[j] = moves[j - 1];
				--j;
			}
			scores[j] = s;
			moves[j] = m;
		}
	}
}

// Sorts the whole list, best first. firstMove (if it's in the list) goes to the front
void Bot::OrderMoves(std::vector<Move>& moves, int ply, bool onlyMVVLVA, Move firstMove)
{
	int scores[MAX_MOVES];
	ScoreMoves(moves, scores, ply, onlyMVVLVA, firstMove);
	PickMove(moves, scores, 0);
	PickMove(moves, scores, 1);
}

// Is a killer or counter move worth trying
bool Bot::IsRefutation(Move move, const MovePicker& mp)
{
	return move != mp.ttMove && Movegen::IsPseudoLegal(move, GameState::currentPlayer, engine->GetBitboardBoard())
		&& !MoveIsCapture(move, engine->GetBitboardBoard()); // Capture here is in the capture list
}

Move Bot::NextMove(MovePicker& mp)
{
	Color us = GameState::currentPlayer;
	const BitboardBoard& board = engine->GetBitboardBoard();
	std::vector<Move>& captures = moveLists[mp.ply];
	std::vector<Move>& quiets = quietLists[mp.ply];
	int* captureScores = moveScores[mp.ply];
	int* quietScores = quietMoveScores[mp.ply];

	switch (mp.stage)
	{
	case Stage::TT_MOVE:
		mp.stage = Stage::GEN_CAPTURES;
		if (Movegen::IsPseudoLegal(mp.ttMove, us, board))
			return mp.ttMove;
		[[fallthrough]]; // Avoid compiler warning
	case Stage::GEN_CAPTURES:
		Movegen::GetAllMoves(captures, us, board, engine, true, true);
		ScoreMoves(captures, captureScores, mp.ply, false, mp.ttMove);
		mp.stage = Stage::GOOD_CAPTURES;
		[[fallthrough]];
	case Stage::GOOD_CAPTURES:
		while (mp.captureIndex < captures.size())
		{
			PickMove(captures, captureScores, mp.captureIndex);
			if (captureScores[mp.captureIndex] < 0)
				break; // This one and the rest lose material by SEE so come after quiets
			Move move = captures[mp.captureIndex++];
			if (move != mp.ttMove)
				return move;
		}
		mp.stage = Stage::KILLER1;
		[[fallthrough]];
	case Stage::KILLER1:
		mp.stage = Stage::KILLER2;
		if (!mp.skipQuiets && IsRefutation(mp.killer1, mp))
			return mp.killer1;
		[[fallthrough]];
	case Stage::KILLER2:
		mp.stage = Stage::COUNTER;
		if (!mp.skipQuiets && IsRefutation(mp.killer2, mp))
			return mp.killer2;
		[[fallthrough]];
	case Stage::COUNTER:
		mp.stage = Stage::GEN_QUIETS;
		if (!mp.skipQuiets && mp.counter != mp.killer1 && mp.counter != mp.killer2 && IsRefutation(mp.counter, mp))
			return mp.counter;
		[[fallthrough]];
	case Stage::GEN_QUIETS:
		if (!mp.skipQuiets)
		{
			Movegen::GetQuietMoves(quiets, us, board);
			ScoreMoves(quiets, quietScores, mp.ply, false, mp.ttMove);
		}
		mp.stage = Stage::QUIETS;
		[[fallthrough]];
	case Stage::QUIETS:
		while (!mp.skipQuiets && mp.quietIndex < quiets.size())
		{
			PickMove(quiets, quietScores, mp.quietIndex);
			Move move = quiets[mp.quietIndex++];
			// Skip the move if the earlier stages returned it
			if (move != mp.ttMove && move != mp.killer1 && move != mp.killer2 && move != mp.counter)
				return move;
		}
		mp.stage = Stage::BAD_CAPTURES;
		[[fallthrough]];
	case Stage::BAD_CAPTURES:
		while (mp.captureIndex < captures.size())
		{
			PickMove(captures, captureScores, mp.captureIndex);
			Move move = captures[mp.captureIndex++];
			if (move != mp.ttMove)
				return move;
		}
		mp.stage = Stage::DONE;
		[[fallthrough]];
	case Stage::DONE:
		break;
	}
	return Move();
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
	long long nodes = nodesSearched + helperNodes.load(std::memory_order_relaxed); // Every thread
	long long nps = nodes * 1000 / std::max(ms, 1LL);

	std::cout << "info depth " << depth;
	if (std::abs(score) >= MATE_VAL - MAX_PLY)
	{
		// Mate scores count plies; UCI wants full moves, negative when we're the side getting mated
		int plies = MATE_VAL - std::abs(score);
		std::cout << " score mate " << (score > 0 ? (plies + 1) / 2 : -(plies / 2));
	}
	else
		std::cout << " score cp " << score;

	std::cout << " nodes " << nodes << " nps " << nps << " time " << ms
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

void Bot::HelperSearch()
{
	// No clock, the main thread decides when to stop
	timePerTurn = -1;
	softTime = -1;
	GetMove();
}

Move Bot::GetMove()
{
	// Only main thread uses opening book, root tb probe, the single-legal-move shortcut
	Move bookMove = isHelper ? Move() : GetBookMove(engine, "res/openings.bin");
	if (!MoveIsNull(bookMove))
	{
		std::cout << "info string Using opening move" << std::endl;
		return bookMove; // Play instantly
	}

	quitEarly = false;
	memset(previousMoves, 0, sizeof(previousMoves));
	startTime = steady_clock::now();

	// Only main ages it for everyone
	if (!isHelper)
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

	// Used for time management
	Move previousBestMove = Move();
	int previousScore = -INF;

	if (!isHelper && tb.Probeable(engine->GetBitboardBoard()))
	{
		Move tbMove = tb.GetMove(engine);
		if (!MoveIsNull(tbMove))
			return tbMove; // A null move means the probe failed: search normally instead
	}

	// Don't use up full time if only one forced move avaiable
	if (timePerTurn >= 0)
	{
		std::vector<Move> rootMoves;
		Movegen::GetAllMoves(rootMoves, botColor, engine->GetBitboardBoard(), engine);
		Move onlyMove = Move();
		int legalMoves = 0;
		for (const Move& move : rootMoves)
		{
			engine->MakeMove(move);
			bool legal = !engine->InCheck(botColor);
			engine->UndoMove();
			if (legal && ++legalMoves == 1)
				onlyMove = move;
		}
		if (legalMoves == 1)
		{
			std::cout << "info string Only one legal move" << std::endl;
			return onlyMove;
		}
	}

	if (onSearchStart)
		onSearchStart();

	for (int depth = isHelper ? 1 + helperIndex % 2 : 1; depth <= maxDepth; ++depth)
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
		if (foundLegal && !isHelper)
			PrintInfo(depth, bestScore, bestMove);

		// Extradite mate for bot
		// Cast to avoid integer overflow
		if ((long long)bestScore >= (MATE_VAL - MAX_PLY))
			break;

		auto elapsed = duration_cast<milliseconds>(steady_clock::now() - startTime).count();
		// Keep searching until limit
		if (depth == maxDepth)
			if (isHelper ? maxDepth < MAX_PLY - 2 : elapsed < timePerTurn)
				++maxDepth;

		// Stop searching if time limit is reached
		if (timePerTurn >= 0 && elapsed >= timePerTurn)
			break;

		// If the best move changed or the score dropped, spend more time on this position
		bool bestMoveChanged = !(bestMove == previousBestMove);
		previousBestMove = bestMove;

		double scale = 1.0;
		if (depth >= 5)
		{
			if (bestMoveChanged)
				scale = 1.5;
			if (previousScore != -INF && bestScore < previousScore - 30)
				scale *= 1.25;
		}
		previousScore = bestScore;

		// The next depth would probably take 2-3x longer, so don't start it
		if (softTime >= 0 && elapsed >= softTime * scale)
			break;
	}

	if (isHelper)
	{
		nodesSearched = 0;
		return bestMove; // Only the main thread plays a move
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
		if (isHelper)
		{
			// Helpers don't use the clock
			helperNodes.fetch_add(512, std::memory_order_relaxed);
			if (helpersStop.load(std::memory_order_relaxed))
			{
				quitEarly = true;
				return alpha;
			}
		}
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
	bool excluded = ss->excludedMove != 0;
	ss->doubleExtensions = (ss - 1)->doubleExtensions;
	int ttEval = NO_EVAL;
	if (tt.ttProbe(key, depth, alpha, beta, ttScore, ttMove, &ttEval) && !pvNode && !excluded)
		return ScoreFromTT(ttScore, ply);

	// Check if the tablebase knows the exact result of this position
	static const int tbPieces = tb.MaxPieces();
	if (!excluded && tbPieces > 0 && GameState::halfmoves == 0
		&& std::popcount(engine->GetBitboardBoard().occupied) <= tbPieces
		&& !GameState::whiteCastlingRights[0] && !GameState::whiteCastlingRights[1]
		&& !GameState::blackCastlingRights[0] && !GameState::blackCastlingRights[1])
	{
		int wdl = tb.ProbeWDLSearch(engine->GetBitboardBoard(), IsWhite(movingColor), GameState::enPassantTarget);
		if (wdl != -99)
		{
			// A win is "at least this good", a loss "at most"
			int score = wdl > 0 ? TB_WIN - ply : wdl < 0 ? -(TB_WIN - ply) : 0;
			uint8_t bound = wdl > 0 ? TT_BETA : wdl < 0 ? TT_ALPHA : TT_EXACT;
			if (bound == TT_EXACT || (bound == TT_BETA && score >= beta) || (bound == TT_ALPHA && score <= alpha))
			{
				tt.ttStore(key, std::min(depth + 6, MAX_PLY - 1), ScoreToTT(score, ply), 0, bound);
				return score;
			}
		}
	}

	int staticEval = inCheck ? NO_EVAL : (ttEval != NO_EVAL ? ttEval : Eval(movingColor, engine));
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
		tt.Prefetch(engine->GetZobristKey());
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
			std::vector<Move>& captures = probCutLists[ply];
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
				tt.Prefetch(engine->GetZobristKey());

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
					tt.ttStore(key, depth - 3, ScoreToTT(score, ply), move, TT_BETA, staticEval);
					return score;
				}
			}
		}
	}

	// Internal iterative reduction. If there's no TT move, this position hasn't been searched
	// so move ordering is poor and a search is expensive. So search it shallower.
	int iirReduction = (depth >= 4 && !ttMove && !excluded) ? 1 : 0;

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
		{
			singularExtension = 1; // No move comes close, extend the TT move

			// Double extension, nothing comes near, so the TT move is forced
			if (!pvNode && score < singularBeta - 20 && (ss - 1)->doubleExtensions < 6)
				singularExtension = 2;
		}
		else if (singularBeta >= beta)
			return singularBeta; // Multicut
		else if (ScoreFromTT(ttEntry.score, ply) >= beta)
			singularExtension = -1; // Negative extension, search shallower
	}

	// Moves come from the staged move generation
	MovePicker mp;
	mp.ply = ply;
	mp.ttMove = ttMove;
	mp.killer1 = killerMoves[ply][0];
	mp.killer2 = killerMoves[ply][1];
	if (ply > 0 && !MoveIsNull(previousMoves[ply]))
		mp.counter = counterMoves[(int)movingColor][GetStart(previousMoves[ply])][GetEnd(previousMoves[ply])];

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

	// Loop through moves
	for (Move move = NextMove(mp); !MoveIsNull(move); move = NextMove(mp))
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
			if (depth <= 8 && moveCount >= lmpLimit) { mp.skipQuiets = true; continue; }

			// Futility pruning: even a good quiet move can't lift the static eval up to alpha
			int lmrDepth = std::max(0, depth - 1 - lmrTable[std::min(depth, 63)][std::min(moveCount + 1, 63)]);
			int futilityValue = staticEval + 100 + 110 * lmrDepth;
			if (lmrDepth <= 6 && futilityValue <= alpha)
			{
				bestEval = std::max(bestEval, futilityValue);
				mp.skipQuiets = true;
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
		tt.Prefetch(engine->GetZobristKey());

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

		int newDepth = depth - 1 - iirReduction;
		if (move == ttMove)
			newDepth += singularExtension;
		ss->doubleExtensions = (ss - 1)->doubleExtensions + (move == ttMove &&
			singularExtension == 2 ? 1 : 0);

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

			// Reduce moves that have often caused cutoffs less
			int piece = ss->pieceIndex;
			int to = GetEnd(move);
			int history = historyHeuristic[piece / 6][piece % 6][GetStart(move)][to];
			for (int back = 1; back <= 2; ++back)
			{
				SearchStackEntry* prev = ss - back;
				if (prev->pieceIndex >= 0)
					history += contHistory[(prev->pieceIndex * 64 + GetEnd(prev->move)) * 768 + piece * 64 + to];
			}
			reduction -= history / 8192;

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

			if (!captureMove && (Pieces)GetPromotion(move) == Pieces::NONE) // Beta cutoff = good
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
		tt.ttStore(key, depth - iirReduction, ScoreToTT(bestScore, ply), bestMove32, flag, staticEval);

	return bestScore;
}

int Bot::Qsearch(int alpha, int beta, int ply, int qply)
{
	if (ply >= MAX_PLY - 1)
		return Eval(GameState::currentPlayer, engine);

	++nodesSearched;
	if ((nodesSearched & 511) == 0) // Check time every 512 nodes
	{
		if (isHelper)
		{
			helperNodes.fetch_add(512, std::memory_order_relaxed);
			if (helpersStop.load(std::memory_order_relaxed))
			{
				quitEarly = true;
				return alpha;
			}
		}
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
	int ttEval = NO_EVAL;
	if (tt.ttProbe(key, 0, alpha, beta, ttScore, ttMove, &ttEval) && !pvNode)
		return ScoreFromTT(ttScore, ply);

	int standPat = -INF;
	auto storeTT = [&](int score, uint8_t flag, Move move)
	{
		TTEntry existing;
		if (tt.ttLookup(key, existing) && existing.depth > 0)
			return;
		tt.ttStore(key, 0, ScoreToTT(score, ply), move, flag, standPat == -INF ? NO_EVAL : standPat);
	};

	const int originalAlpha = alpha;
	Move bestMove = Move();

	int bestScore = -INF;

	bool inCheck = engine->InCheck(movingColor);
	if (!inCheck)
	{
		standPat = ttEval != NO_EVAL ? ttEval : Eval(movingColor, engine);
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

	int* scores = qMoveScores[ply];
	ScoreMoves(moves, scores, ply, true, ttMove);

	bool foundLegal = false;

	for (size_t moveIndex = 0; moveIndex < moves.size(); ++moveIndex)
	{
		PickMove(moves, scores, moveIndex);
		const Move move = moves[moveIndex];

		// Losing exchange
		if (!inCheck && !SEE(move, 0))
			continue;

		// Delta pruning. If a free capture can't get score up to alpha
		if (!inCheck && GetPromotion(move) == (int)Pieces::NONE && !engine->GivesCheck(move))
		{
			Piece captured = engine->GetBoard()[GetEnd(move)].GetPiece();
			int gain = IsEnPassant(move) ? 100 : PieceValue(captured);
			int futilityValue = standPat + gain + 200;
			if (gain > 0 && futilityValue <= alpha)
			{
				bestScore = std::max(bestScore, futilityValue);
				continue;
			}
		}

		engine->MakeMove(move);
		tt.Prefetch(engine->GetZobristKey());

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