#include "core/engine.hpp"
#include "bot/bot.hpp"

// TODO: Promotion GUI
// TODO: Optimization with bitboards
// TODO: Don't clear and refill queued renders every frame

int main()
{
	Engine chessEngine;
	Bot chessBot(&chessEngine, Color::BLACK);
	chessEngine.SetBot(&chessBot);
	while (1)
	{
		chessEngine.Update();
	}
	return 0;
}