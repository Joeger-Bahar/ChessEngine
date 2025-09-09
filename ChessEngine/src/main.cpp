#include "core/engine.hpp"
#include "bot/bot.hpp"

// TODO: Promotion GUI
// TODO: Optimization with bitboards
// TODO: Don't clear and refill queued renders every frame
// TODO: Probably won't find mate if it's in the same number of moves as the depth

int main()
{
	const char* fen = "rnb1kb1r/pppppppp/8/2q5/6n1/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	const char* fen3 = "rnbqkbnr/pppppppp/8/6N1/2Q5/8/PPPPPPPP/RNB1KB1R w KQkq - 0 1";
	const char* forkFen = "3r1b1r/Rk3ppp/1p2b2n/4p3/Qn1q4/2N5/3PPPPP/2B1KBNR b K - 0 1";
	const char* castleMate = "K7/2k2ppp/2r1p3/3r4/8/6p1/8/8 w - 0 1";
	const char* liefGame = "r4knr/1p3ppp/p2bb3/6BQ/3P4/5N2/Pq3PPP/2R1R1K1 w - 0 1";
	const char* ladderFen = "8/8/8/5k2/&p/7K/6PP/4b1q1 b - 0 1";
	const char* fen4 = "6k1/5ppp/8/8/8/4r3/5PPP/6K1 b - 0 1";
	const char* mateIn3 = "3b4/8/7p/5K1k/8/6p1/6P1/2R5 w - 0 1";
	const char* mateIn2 = "8/8/8/2P3R1/5B2/2rP1p2/p1P1PP2/RnQ1K2k w Q - 5 3";
	Engine chessEngine(fen3);
	Bot chessBot(&chessEngine, Color::WHITE);
	chessEngine.SetBot(&chessBot);
	while (1)
	{
		chessEngine.Update();
	}
	return 0;
}