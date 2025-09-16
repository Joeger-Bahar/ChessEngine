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

Uci::Uci(Engine* engine, Bot* bot) : engine(engine), bot(bot)
{
}

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
    else if (token == "isready")
    {
        std::cout << "readyok" << std::endl;
    }
    else if (token == "position")
    {
        HandlePosition(iss);
    }
    else if (token == "go")
    {
        HandleGo(iss);
    }
    else if (token == "ucinewgame")
    {
        // Should probably clean this up
        delete engine;
        engine = new Engine();

        GameState::currentPlayer = Color::WHITE;
        GameState::checkStatus = 0;
        GameState::enPassantTarget[0] = -1;
        GameState::enPassantTarget[1] = -1;
        GameState::halfmoves = 0;
        GameState::endgameStatus = false;
        GameState::checkmate = false;
        GameState::draw = false;
        GameState::invalidMove = false;
        GameState::whiteCastlingRights[0] = true;
        GameState::whiteCastlingRights[1] = true;
        GameState::blackCastlingRights[0] = true;
        GameState::blackCastlingRights[1] = true;

        bot->Clear();
    }
    else if (token == "quit")
    {
        exit(0);
    }
}

void Uci::HandlePosition(std::istringstream& iss)
{
    std::string line;
    std::getline(iss, line);
    
    std::string token;
    std::istringstream line_ss(line);
    line_ss >> token;

    if (token == "startpos")
    {
        engine->LoadPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        line_ss >> token; // "moves"
    }
    else if (token == "fen")
    {
        std::string fen;
        std::string fen_part;
        for(int i=0; i<6; ++i)
        {
            line_ss >> fen_part;
            fen += fen_part + " ";
        }
        engine->LoadPosition(fen);
        line_ss >> token; // "moves"
    }

    if (token == "moves")
    {
        std::string moveString;
        while (line_ss >> moveString)
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
        if (token == "wtime")
        {
            iss >> wtime;
        } else if (token == "btime")
        {
            iss >> btime;
        }
    }

    bot->SetColor(engine->GetCurrentPlayer());
    Move bestMove = bot->GetMoveUCI(wtime, btime);
    std::cout << "bestmove " << bestMove.ToUCIString() << std::endl;
    std::cout.flush();
}

Move Uci::ParseMove(const std::string& moveString)
{
    Move move;
    move.startCol = moveString[0] - 'a';
    move.startRow = 8 - (moveString[1] - '0');
    move.endCol = moveString[2] - 'a';
    move.endRow = 8 - (moveString[3] - '0');

    if (moveString.length() == 5)
    {
        switch (moveString[4])
        {
            case 'q': move.promotion = static_cast<int>(Pieces::QUEEN); break;
            case 'r': move.promotion = static_cast<int>(Pieces::ROOK); break;
            case 'b': move.promotion = static_cast<int>(Pieces::BISHOP); break;
            case 'n': move.promotion = static_cast<int>(Pieces::KNIGHT); break;
            default: move.promotion = 6; // None
        }
    }
    else
    {
        move.promotion = 6; // None
    }

    Piece movingPiece = engine->GetBoard()[move.startRow][move.startCol].GetPiece();
    if (movingPiece.GetType() == Pieces::KING && abs(move.startCol - move.endCol) == 2)
    {
        move.wasCastle = true;
    }
    else
    {
        move.wasCastle = false;
    }

    if (movingPiece.GetType() == Pieces::PAWN &&
        move.startCol != move.endCol &&
        engine->GetBoard()[move.endRow][move.endCol].IsEmpty())
    {
        move.wasEnPassant = true;
    }
    else
    {
        move.wasEnPassant = false;
    }

    return move;
}
