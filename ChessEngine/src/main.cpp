#include "core/engine.hpp"
#include "bot/bot.hpp"
#include "uci/uci.hpp"
#include "bot/opening.hpp"

#include "test.hpp"

#include <memory>

// TODO: Don't clear and refill queued renders every frame
// TODO: Sometimes using uint8_t for moves has very weird memory bugs, like setting the move col to 204 in GetAllMoves
// TODO: Logging
// 
// TODO: Move extension: If opponent in check/move is promotion extend search 1 further
// TODO: Tablebase
// TODO: Magic bitboards
// TODO: Pondering (search while opponent is moving)

int main()
{
    const char* fen = "4k2r/6r1/8/8/8/8/3R4/R3K3 w Qk - 0 1";
    const char* fen2 = "4r1k1/4r1p1/8/p2R1P1K/5P1P/1QP3q1/1P6/3R4 b - - 0 1";
    const char* fen3 = "8/K7/8/8/2k5/8/8/1B6 w - - 33 64";
    const char* fen4 = "3k3r/6r1/8/8/8/8/8/2KR4 b - - 0 1";
    const char* fen5 = "8/8/6p1/3k4/1P6/7P/3K4/8 w - - 0 1";
    const char* fenMis = "rnb1kb1r/pppQpppp/5n2/8/8/8/PPPP1PPP/RNB1KBNR b KQkq - 0 5";
    const char* mis = "6K1/2RP4/1B6/pp2r1Rp/1kp2Np1/1p1PN3/8/8 w - - 0 1";
    const char* start = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    const char* enMis = "7r/1Q3ppp/p7/2k4q/3PB3/8/1PP1NP2/1NB1KR2 w - - 1 24";
    const char* pawn = "8/bP1pkr2/5p2/1r3pPB/3p4/p6K/1R4P1/3b4 w - - 0 1";
    GameState::uci = true; // Used for bot v. bot iteration testing
    std::unique_ptr<Engine> chessEngine = std::make_unique<Engine>();
    // Smart ptr so I don't need delete at end of file (lazy)
	std::unique_ptr<Bot> whiteBot = std::make_unique<Bot>(chessEngine.get(), Color::WHITE);
    std::unique_ptr<Bot> blackBot = std::make_unique<Bot>(chessEngine.get(), Color::BLACK);

    //PerftDebug(chessEngine.get(), 4);

    if (GameState::uci)
    {
        Uci uci(chessEngine.get(), whiteBot.get());
        uci.Loop();
    }
    else
    {
        chessEngine->SetBot(whiteBot.get());
        chessEngine->SetBot(blackBot.get());

        while (1)
        {
            //PerftDebug(chessEngine.get(), 3);
            chessEngine->Update();
        }
    }

	return 0;
}