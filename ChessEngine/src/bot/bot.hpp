#pragma once

#include <chrono>
#include <vector>
#include <atomic>
#include <functional>

#include "opening.hpp"

#include "core/piece.hpp"
#include "core/boardCalculator.hpp"
#include "core/engine.hpp"
#include "core/tt.hpp"
#include "core/eval.hpp"
#include "graphics/graphicsEngine.hpp"

#define MAX_PLY 128
#define MAX_MOVES 256 // Max amount of legal moves is 218
#define NUM_PIECES 6

struct SearchStackEntry
{
	Move move = 0;
	Move excludedMove = 0; // Move to skip during singular extension
	int pieceIndex = -1; // color*6 + (type-1), -1 for null move
	int staticEval = 0;
	bool inCheck = false;
	bool nullMove = false;
	int doubleExtensions = 0; // Double extensions on the path to this node
};

enum class Stage : uint8_t
{
	TT_MOVE, GEN_CAPTURES, GOOD_CAPTURES, KILLER1, KILLER2, COUNTER,
	GEN_QUIETS, QUIETS, BAD_CAPTURES, DONE
};

struct MovePicker
{
	Stage stage = Stage::TT_MOVE;
	int ply = 0;
	Move ttMove = Move();
	Move killer1 = Move();
	Move killer2 = Move();
	Move counter = Move();
	size_t captureIndex = 0; // Next move in moveLists[ply]
	size_t quietIndex = 0; // Next move in quietLists[ply]
	bool skipQuiets = false; // Set by LMP. No more quiet moves
};

class Bot
{
public:
	Bot(Engine* engine, Color color);
	Move GetMove();
	Move GetMoveUCI(int timeForMove, int softTime = -1);
	void SetColor(Color color);
	const Color GetColor() const { return botColor; }

	void Clear();
	inline void SetHashSize(int megabytes) { tt.Resize(megabytes); }
	inline void SetThreadCount(int threads) { this->threads = threads; }

	// Lazy SMP. Helper threads run this same search on their own board, sharing the TT.
	void SetEngine(Engine* newEngine) { engine = newEngine; }
	void MakeHelper(int index) { isHelper = true; helperIndex = index; }
	void HelperSearch(); // Searches until the main thread calls StopHelpers()
	// Called by the main thread once it needs to actually search to start helpers
	void SetSearchStartCallback(std::function<void()> callback) { onSearchStart = std::move(callback); }
	static void ResetHelperState(); // Before helpers start: clears the stop flag and node count
	static void StopHelpers();      // Tells the helper threads to finish

private:
	bool SEE(Move move, int threshold);

	int Search(int depth, int ply, int alpha, int beta);
	int Qsearch(int alpha, int beta, int ply, int qply = 0);
	void ScoreMoves(const std::vector<Move>& moves, int* scores, int ply, bool onlyMVVLVA, Move ttMove = Move());
	void OrderMoves(std::vector<Move>& moves, int ply, bool onlyMVVLVA, Move firstMove = Move());
	// Next move of the staged move gen
	Move NextMove(MovePicker& mp);
	bool IsRefutation(Move move, const MovePicker& mp);

	// UCI output
	void PrintInfo(int depth, int score, Move rootMove);
	std::string GetPV(Move rootMove, int maxLength);

	TranspositionTable& tt = SharedTT(); // Singleton table
	// [color][pieces][start square][end square]
	int historyHeuristic[2][NUM_PIECES][64][64] = { 0 };
	Engine* engine;
	Color botColor;
	
	// Continuation history
	std::vector<int16_t> contHistory;
	std::vector<Move> moveLists[MAX_PLY]; // Has capture/noisy moves
	std::vector<Move> qMoveLists[MAX_PLY];
	int moveScores[MAX_PLY][MAX_MOVES];  // Scores for moveLists[ply]
	int qMoveScores[MAX_PLY][MAX_MOVES]; // Scores for qMoveLists[ply
	std::vector<Move> probCutLists[MAX_PLY];
	std::vector<Move> quietLists[MAX_PLY]; // Quiet moves for the search
	int quietMoveScores[MAX_PLY][MAX_MOVES]; // Scores

	SearchStackEntry searchStack[MAX_PLY + 8];
	Move previousMoves[MAX_PLY] = {};
	Move killerMoves[MAX_PLY][2];
	Move counterMoves[2][64][64];
	int lmrTable[64][64];

	// Capture history [moving piece][to square][captured piece type]
	int captureHistory[12][64][6] = {};
	int& CaptureHistoryEntry(Move move);

	// Start time of the search, used for time control
	std::chrono::time_point<std::chrono::steady_clock> startTime;
	int nodesSearched;
	int rootDepth = 0;
	int timePerTurn = 12000; // In milliseconds
	int softTime = -1;
	int threads = 1;
	bool quitEarly = false;

	// Lazy SMP
	bool isHelper = false;
	int helperIndex = 0;
	std::function<void()> onSearchStart;
};
