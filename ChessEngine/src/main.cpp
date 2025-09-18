#include "core/engine.hpp"
#include "bot/bot.hpp"
#include "uci/uci.hpp"
#include "bot/opening.hpp"

// TODO: Don't clear and refill queued renders every frame
// TODO: Sometimes using uint8_t for moves has very weird memory bugs, like setting the move col to 204 in GetAllMoves
// TODO: Logging
// 
// TODO: Move extension: If opponent in check/move is promotion extend search 1 further
// TODO: Tablebase
// TODO: Optimization with bitboards
// TODO: ^ Magic bitboards
// TODO: Pondering (search while opponent is moving)

int main()
{
    const char* fen = "4k2r/6r1/8/8/8/8/3R4/R3K3 w Qk - 0 1";
    const char* fen2 = "3k3r/6r1/8/8/8/8/8/R3K3 w Q - 0 2";
    const char* fen3 = "3k3r/6r1/8/8/8/8/8/R3K3 w Q - 0 1";
    const char* fen4 = "3k3r/6r1/8/8/8/8/8/2KR4 b - - 0 1";
    const char* fen5 = "8/8/6p1/3k4/1P6/7P/3K4/8 w - - 0 1";
    const char* mis = "6K1/2RP4/1B6/pp2r1Rp/1kp2Np1/1p1PN3/8/8 w - - 0 1";
    const char* start = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    const char* pawn = "pppppppp/8/8/8/8/8/8/PPPPPPPP w KQkq - 0 1";
    bool uci = false; // Used for bot v. bot iteration testing
    Engine* chessEngine = new Engine;
	Bot* whiteBot = new Bot(chessEngine, Color::WHITE);
	//Bot* blackBot = new Bot(chessEngine, Color::BLACK);

    if (uci)
    {
        Uci uci(chessEngine, whiteBot);
        uci.Loop();
    }
    else
    {
        chessEngine->SetBot(whiteBot);
        //chessEngine->SetBot(blackBot);

        while (1) chessEngine->Update();
    }

    delete chessEngine;
    delete whiteBot;
    //delete blackBot;

	return 0;
}