#include "core/engine.hpp"
#include "bot/bot.hpp"
#include "uci/uci.hpp"
#include <iostream>

// TODO: Optimization with bitboards
// TODO: Don't clear and refill queued renders every frame
// 
// TODO: Opening book
// TODO: Move extension: If opponent in check/move is promotion extend search 1 further
// TODO: Pondering (search while opponent is moving)
// TODO: Tablebase

int main()
{
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
        //chessEngine.SetBot(&whiteBot);
        //chessEngine.SetBot(&blackBot);
        while (1)
        {
            chessEngine.Update();
        }
    }

	return 0;
}