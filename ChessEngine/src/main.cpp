#include "core/engine.hpp"
#include "bot/bot.hpp"

// TODO: Promotion GUI
// TODO: Optimization with bitboards
// TODO: Don't clear and refill queued renders every frame

int main()
{
	const char* fen = "rnb1kb1r/pppppppp/1q6/8/6n1/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	const char* forkFen = "3r1b1r/Rk3ppp/1p2b2n/4p3/Qn1q4/2N5/3PPPPP/2B1KBNR b K - 0 1";
	Engine chessEngine;
	Bot chessBot(&chessEngine, Color::WHITE);
	chessEngine.SetBot(&chessBot);
	while (1)
	{
		chessEngine.Update();
	}
	return 0;
}