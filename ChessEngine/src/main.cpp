#include "core/engine.hpp"
#include "bot/bot.hpp"
#include "uci/uci.hpp"
#include <iostream>

// TODO: Don't clear and refill queued renders every frame
// 
// TODO: Opening book
// TODO: Move extension: If opponent in check/move is promotion extend search 1 further
// TODO: Tablebase
// TODO: Optimization with bitboards
// TODO: ^ Magic bitboards
// TODO: Pondering (search while opponent is moving)

int main()
{
    const char* fen = "r1bqkb1r/ppp2ppp/2n2n2/3Pp1N1/2B5/8/PPPP1PPP/RNBQK2R b KQkq - 0 5";
    bool uci = false; // Used for bot v. bot iteration testing
	Engine chessEngine;
	Bot whiteBot(&chessEngine, Color::WHITE);
	Bot blackBot(&chessEngine, Color::BLACK);

    if (uci)
    {
        Uci uci(&chessEngine, &whiteBot);
        uci.Loop();
    }
    else
    {
        chessEngine.SetBot(&whiteBot);
        chessEngine.SetBot(&blackBot);
        while (1)
        {
            chessEngine.Update();
        }
    }

	return 0;
}