#include "core/engine.hpp"
#include "bot/bot.hpp"
#include "uci/uci.hpp"
#include "bot/opening.hpp"
#include <iostream>

// TODO: Don't clear and refill queued renders every frame
// TODO: Sometimes using uint8_t for moves has very weird memory bugs, like setting the move col to 204 in GetAllMoves
// 
// TODO: Opening book
// TODO: Move extension: If opponent in check/move is promotion extend search 1 further
// TODO: Tablebase
// TODO: Optimization with bitboards
// TODO: ^ Magic bitboards
// TODO: Pondering (search while opponent is moving)

int main()
{
    // Load opening book

    const char* fen = "r1bqkb1r/ppp2ppp/2n2n2/3Pp1N1/2B5/8/PPPP1PPP/RNBQK2R w KQkq - 0 5";
    const char* pawn = "pppppppp/8/8/8/8/8/8/PPPPPPPP w KQkq - 0 1";
    bool uci = false; // Used for bot v. bot iteration testing
    Engine chessEngine;
	Bot whiteBot(&chessEngine, Color::WHITE);
	Bot blackBot(&chessEngine, Color::BLACK);
    uint64_t startKey = chessEngine.GetZobristKey();
    //std::cout << "Startpos hash: " << std::hex << startKey << std::endl;
    //std::cout << "Expected hash: 0xf8d626aaaf278509" << std::endl;

    //chessEngine.LoadPosition(fen);
    //uint64_t fenKey = chessEngine.ComputeFullHash();
    //std::cout << "FEN hash: " << std::hex << fenKey << std::endl;

    LoadPolyglot("res/komodo.bin");

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