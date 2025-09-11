#include "core/engine.hpp"
#include "bot/bot.hpp"

// TODO: Promotion GUI
// TODO: Optimization with bitboards
// TODO: Don't clear and refill queued renders every frame
// TODO: Probably won't find mate if it's in the same number of moves as the depth
// TODO: Opening book
// TODO: Move extension: If opponent in check/move is promotion extend search 1 further

int main()
{
	const char* forkFen = "3r1b1r/Rk3ppp/1p2b2n/4p3/Qn1q4/2N5/3PPPPP/2B1KBNR b K - 0 1";
	const char* fen4 = "3rk2r/pppq1ppp/2nbbn2/3pp3/8/1P1PP1P1/PBPNNPBP/R2QK2R w KQ - 2 9";
	const char* mateIn3 = "3b4/8/7p/5K1k/8/6p1/6P1/2R5 w - 0 1";
	const char* mateIn4 = "2rq1r2/pb2bpp1/1pnppn1p/2p1k3/4p2B/1PPBP3/PP2QPPP/R2NRNK1 w - 0 1";
	const char* mateIn2 = "8/8/8/2P3R1/5B2/2rP1p2/p1P1PP2/RnQ1K2k w Q - 5 3"; // Castle mate
	const char* gameFen = "2b3kr/2p4p/2p1p1p1/p3P3/7b/57P/P1r2PP1/RN1R2K1 w - 0 2";
	const char* enPassantMate = "6Q1/8/p4p1p/2K4k/2PN3p/8/bP3PPP/4R3 w - 1 8"; // White pawn forward, need to take en passant to get out of check
	Engine chessEngine(mateIn4);// (enPassantMate);
	Bot chessBot(&chessEngine, Color::WHITE);
	chessEngine.SetBot(&chessBot);
	while (1)
	{
		chessEngine.Update();
	}
	return 0;
}