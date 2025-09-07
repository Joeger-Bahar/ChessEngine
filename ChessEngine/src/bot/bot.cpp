#include "bot.hpp"

#include "core/boardCalculator.hpp"

#include <limits>
#include <iostream>
#include <vector>

Bot::Bot(Engine* engine, Color color)
{
	this->engine = engine;
	this->botColor = color;
}

int Bot::Search(int depth, int alpha, int beta)
{
	if (depth == 0 || engine->IsOver())
        return Eval();

    int maxEval = std::numeric_limits<int>::min();
    std::vector<Move> moves = BoardCalculator::GetAllMoves(GameState::currentPlayer, engine->GetBoard());

    for (const Move move : moves) {
        engine->MakeMove(move);
        int score = -Search(depth - 1, -beta, -alpha);
        engine->UndoMove();

        if (score > maxEval) maxEval = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) break; // Prune
    }

    return maxEval;
}

Move Bot::GetMove()
{
	int depth = 4; // Search depth

    Move bestMove;
    int bestScore = std::numeric_limits<int>::min();

    auto moves = BoardCalculator::GetAllMoves(GameState::currentPlayer, engine->GetBoard());
    for (const auto& move : moves)
    {
        engine->MakeMove(move);
        int score = -Search(depth - 1, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        engine->UndoMove();

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = move;
        }
    }

    return bestMove;
}

int Bot::Eval()
{
    int score = 0;

    // Assign standard material values
    const int pieceValues[] = { 100, 320, 330, 500, 900, 20000, 0 };

    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            const Piece& p = engine->GetBoard()[r][c].GetPiece();
            if (p.type == Pieces::NONE) continue;

            int value = pieceValues[(int)p.type];

            if (p.color == Color::WHITE) score += value;
            else if (p.color == Color::BLACK) score -= value;
        }
    }

    // Positive score = advantage for WHITE, negative = advantage for BLACK
	if (botColor == Color::BLACK) score = -score; // Invert for BLACK bot
    return score;
}