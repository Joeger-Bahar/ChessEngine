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
        std::cout << "uciok" << std::endl;
    }
    else if (token == "isready")  std::cout << "readyok" << std::endl;
    else if (token == "position") HandlePosition(iss);
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
    }
    else if (token == "quit") exit(0);
}

void Uci::HandlePosition(std::istringstream& iss)
{
    std::string line;
    std::getline(iss, line);
    
    std::string token;
    std::istringstream lineSS(line);
    lineSS >> token;

    if (token == "startpos")
    {
        engine->LoadPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        lineSS >> token; // "moves"
    }
    else if (token == "fen")
    {
        std::string fen;
        std::string fen_part;
        for (int i = 0; i < 6; ++i)
        {
            lineSS >> fen_part;
            fen += fen_part + " ";
        }
        engine->LoadPosition(fen);
        lineSS >> token; // "moves"
    }

    if (token == "moves")
    {
        std::string moveString;
        while (lineSS >> moveString)
        {
            Move move = ParseMove(moveString);
            engine->MakeMove(move);
        }
    }
}

void Uci::HandleGo(std::istringstream& iss)
{
    int wtime = 300000; // Default 5 minutes
    int btime = 300000;

    std::string token;
    while (iss >> token)
    {
        if (token == "wtime")      iss >> wtime;
        else if (token == "btime") iss >> btime;
    }

    bot->SetColor(engine->GetCurrentPlayer());
    Move bestMove = bot->GetMoveUCI(wtime, btime);

    std::cout << "bestmove " << MoveToUCI(bestMove) << std::endl;
    std::cout.flush();
}

Move Uci::ParseMove(const std::string& moveString)
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
