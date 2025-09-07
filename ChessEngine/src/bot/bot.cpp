#include "bot.hpp"

#include <ctime>
#include <iostream>

Bot::Bot(Engine* engine, Color color)
{
	this->engine = engine;
	this->botColor = color;
	srand(time(0));
}

Move Bot::GetMove()
{
	std::vector<Move> moves = BoardCalculator::GetAllMoves(botColor, engine->GetBoard());
	// Highlight every possible move in it's own color
	//for (Move move : moves)
	//{
	//	unsigned char color = (move.startCol * 20) % 255;
	//	engine->graphics.QueueRender([=]() { engine->graphics.DrawSquareHighlight(move.startRow, move.startCol, { 0, color, 0, 255 }); });
	//	engine->graphics.QueueRender([=]() { engine->graphics.DrawSquareHighlight(move.endRow, move.endCol, { 0, color, 255, 255 }); });
	//}

	while (moves.size() > 0)
	{
		int index = rand() % moves.size();

		engine->MakeMove(moves[index]);
		engine->CheckKingInCheck();
		if (engine->InCheck(botColor))
		{
			engine->UndoMove();
			// Remove move from list and try again
			moves.erase(moves.begin() + index);
			continue;
		}
		engine->UndoMove();

		return moves[index];
	}

	return Move();
}

