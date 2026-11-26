#include "uci.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <random>
#include <thread>
#include <cstdlib>
#include <ctime>

Uci::Uci(Engine* engine, Bot* bot)
    : engine(engine), bot(bot)
{}

void Uci::Loop()
{
    std::string line;
    while (std::getline(std::cin, line)) {
        //uciLog << "<<< " << line << std::endl; // log incoming
        //uciLog.flush(); // make sure it writes immediately
        HandleCommand(line);
    }
}

void Uci::HandleCommand(const std::string& line) 
{
    std::istringstream iss(line);
    std::string token;
    iss >> token;

    if (token == "uci")
    {
        std::cout << "id name ChessEngine" << std::endl;
        std::cout << "id author Joeger" << std::endl;
        std::cout << "option name Hash type spin default 128 min 1 max 4096" << std::endl;
        std::cout << "option name Threads type spin default 1 min 1 max 1024" << std::endl;
        std::cout << "uciok" << std::endl;
    }
    else if (token == "isready")  std::cout << "readyok" << std::endl;
    else if (token == "position") HandlePosition(iss);
    else if (token == "setoption") HandleSetOption(iss);
    else if (token == "go")       HandleGo(iss);
    else if (token == "ucinewgame")
    {
        // Should probably clean this up
        GameState::currentPlayer = Color::WHITE;
        GameState::checkStatus = 0;
        GameState::enPassantTarget = 0;
        GameState::halfmoves = 0;
        GameState::endgame = false;
        GameState::checkmate = false;
        GameState::draw = false;
        GameState::invalidMove = false;
        GameState::whiteCastlingRights[0] = true;
        GameState::whiteCastlingRights[1] = true;
        GameState::blackCastlingRights[0] = true;
        GameState::blackCastlingRights[1] = true;

        delete engine;
        engine = new Engine();

        delete bot;
        bot = new Bot(engine, Color::WHITE);
        bot->SetHashSize(hashMB);
        helperBots.clear(); // New game, new history for bots
    }
    else if (token == "quit") exit(0);
}

void Uci::HandlePosition(std::istringstream& iss)
{
    std::getline(iss, positionLine);
    SetPosition(engine, positionLine);
}
    
void Uci::SetPosition(Engine* engine, const std::string& positionLine)
{
    std::string token;
    std::istringstream lineSS(positionLine);
    lineSS >> token;

    if (token == "startpos")
    {
        engine->LoadPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        lineSS >> token; // "moves"
    }
    else if (token == "fen")
    {
        std::string fen;
        std::string fenPart;
        for (int i = 0; i < 6; ++i)
        {
            lineSS >> fenPart;
            fen += fenPart + " ";
        }

        engine->LoadPosition(fen);
        lineSS >> token; // "moves"
    }

    if (token == "moves")
    {
        std::string moveString;
        while (lineSS >> moveString)
        {
            Move move = ParseMove(engine, moveString);
            engine->MakeMove(move);
        }
    }
}

void Uci::HandleGo(std::istringstream& iss)
{
    int wtime = -1, btime = -1;        // Remaining time in ms
    int winc = 0, binc = 0;            // Increment per move in ms
    int movestogo = -1;                // Moves to next time control
    int movetime = -1;                 // Fixed time per move
    int depth = -1, nodes = -1;
    bool infinite = false;

    std::string token;
    while (iss >> token)
    {
        if (token == "wtime")      iss >> wtime;
        else if (token == "btime") iss >> btime;
        else if (token == "winc")  iss >> winc;
        else if (token == "binc")  iss >> binc;
        else if (token == "movestogo") iss >> movestogo;
        else if (token == "movetime")  iss >> movetime;
        else if (token == "depth")     iss >> depth;
        else if (token == "nodes")     iss >> nodes;
        else if (token == "infinite")  infinite = true;
    }

    // Determine our color
    Color us = engine->GetCurrentPlayer();
    int myTime = (us == Color::WHITE ? wtime : btime);
    int myInc = (us == Color::WHITE ? winc : binc);

    int timeForMove = 0; // Hard limit, search stops here during a serach
    int softTime = -1; // Soft limit, don't start new search

    if (movetime > 0)
    {
        // --- Fixed time per move ---
        timeForMove = movetime;
    }
    else if (infinite)
    {
        // --- Search until stop command ---
        timeForMove = -1; // Let search run indefinitely
    }
    else if (myTime > 0)
    {
        // --- Regular timed games (sudden death, increment, or moves in X time) ---

        int movesLeft = movestogo > 0 ? movestogo : 30; // Estimate if unknown

        // Time we should spend on this move, on average
        int target = myTime / movesLeft + myInc * 3 / 4;
        
        // Don't start a new depth at 60% of the target time
        softTime = target * 60 / 100;

        // Never use more than 1/4 of the clock
        timeForMove = target * 3;
        if (timeForMove > myTime / 4)
            timeForMove = myTime / 4;

        // Keep a little time for GUI communication
        int overhead = myTime / 20;
        if (overhead > 30)
            overhead = 30;
        timeForMove -= overhead;
        if (timeForMove < 5)
            timeForMove = 5;
        if (softTime > timeForMove)
            softTime = timeForMove;
    }
    else
    {
        // No valid timing info (e.g. analysis mode)
        timeForMove = 1000; // default 1 second
    }

    bot->SetColor(us);
    bot->SetSearchStartCallback([this, us]() { StartHelpers(us); });
    Move bestMove = bot->GetMoveUCI(timeForMove, softTime);
    StopHelpers();

    std::cout << "bestmove " << MoveToUCI(bestMove) << std::endl;
    std::cout.flush();
}

void Uci::HandleSetOption(std::istringstream& is)
{
    std::string word, name, value;
    is >> word; // "name";
    while (is >> word && word != "value")
        name += (name.empty() ? "" : " ") + word;
    is >> value;

    if (name == "Hash" && !value.empty())
    {
        int mb = std::stoi(value);
        if (mb < 1) mb = 1;
        if (mb > 4096) mb = 4096;
        hashMB = mb;
        bot->SetHashSize(hashMB);
    }

    if (name == "Threads" && !value.empty())
    {
        threads = std::stoi(value);
        if (threads < 1) threads = 1;
        bot->SetThreadCount(threads);
        helperBots.clear(); // Main again at next search
    }
}



Move Uci::ParseMove(Engine* engine, const std::string& moveString)
{
    int startCol = moveString[0] - 'a';
    int startRow = '8' - moveString[1];
    int endCol = moveString[2] - 'a';
    int endRow = '8' - moveString[3];

    int startSquare = ToIndex(startRow, startCol);
    int endSquare = ToIndex(endRow, endCol);
    int promotion = static_cast<int>(Pieces::NONE);
    bool isCastle = false;
    bool isEnPassant = false;

    if (moveString.length() == 5)
    {
        switch (moveString[4])
        {
            case 'q': promotion = static_cast<int>(Pieces::QUEEN);  break;
            case 'r': promotion = static_cast<int>(Pieces::ROOK);   break;
            case 'b': promotion = static_cast<int>(Pieces::BISHOP); break;
            case 'n': promotion = static_cast<int>(Pieces::KNIGHT); break;
            default: promotion = static_cast<int>(Pieces::NONE); // None
        }
    }
    else
        promotion = static_cast<int>(Pieces::NONE); // None

    Piece movingPiece = engine->GetBoard()[startSquare].GetPiece();
    if (movingPiece.GetType() == Pieces::KING && abs((startSquare % 8) - (endSquare % 8)) == 2)
    {
        isCastle = true;
    }
    else
    {
        isCastle = false;
    }

    if (movingPiece.GetType() == Pieces::PAWN &&
        (startSquare % 8) != (endSquare % 8) &&
        engine->GetBoard()[endSquare].IsEmpty())
    {
        isEnPassant = true;
    }
    else
    {
        isEnPassant = false;
    }

    return EncodeMove(startSquare, endSquare, promotion, isEnPassant, isCastle);
}

// Lazy SMP
// All threads share the same TT and search the same position, so what one finds
// they all can use
// Only main thread's move is played
void Uci::StartHelpers(Color us)
{
    if (threads <= 1)
        return;

    while ((int)helperBots.size() < threads - 1)
    {
        helperBots.push_back(std::make_unique<Bot>(engine, us)); // Get's its own engine
        helperBots.back()->MakeHelper((int)helperBots.size());
    }

    Bot::ResetHelperState();
    for (int i = 0; i < threads - 1; ++i)
    {
        Bot* helper = helperBots[i].get();
        helperThreads.emplace_back([this, helper, us]()
        {
            auto helperEngine = std::make_unique<Engine>();
            SetPosition(helperEngine.get(), positionLine);

            helper->SetEngine(helperEngine.get());
            helper->SetColor(us);
            helper->HelperSearch();
            helper->SetEngine(nullptr);
        });
    }
}

void Uci::StopHelpers()
{
    Bot::StopHelpers();
    for (std::thread& t : helperThreads)
        t.join();
    helperThreads.clear();
}
