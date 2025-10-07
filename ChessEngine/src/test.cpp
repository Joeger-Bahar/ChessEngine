#include "test.hpp"

#include <iostream>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>

#include "core/engine.hpp"

StockfishPerftResult StockfishPerft(const std::string& stockfishPath, const std::string& fen, int depth)
{
    const char* tmpFilename = "stockfish_uci_commands.txt";
    TmpFileGuard fileGuard(tmpFilename); // Ensures file is deleted when function exits

    // --- 1. Write the UCI commands to a temporary file ---
    { // Use a scope to ensure the file is closed before we use it
        std::ofstream cmdFile(tmpFilename);
        if (!cmdFile) {
            throw std::runtime_error("Failed to create temporary command file.");
        }
        cmdFile << "position fen " << fen << std::endl;
        cmdFile << "go perft " << std::to_string(depth) << std::endl;
        cmdFile << "quit" << std::endl; // Tell Stockfish to exit cleanly
    }

    // --- 2. Build the shell command to run Stockfish with input redirection ---
    // The "<" tells the shell to feed our file into the program's standard input
    std::string command = "\"" + stockfishPath + "\" < " + tmpFilename;

    // --- 3. Execute the command and open a pipe to read its output ---
    std::string result;
    std::array<char, 256> buffer;

    // The unique_ptr ensures _pclose is called automatically, even if an error occurs
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
    if (!pipe)
    {
        throw std::runtime_error("Failed to start Stockfish process with _popen.");
    }

    // --- 4. Read the entire output from the pipe ---
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }

    StockfishPerftResult out;
    out.nodes = 0;

    std::istringstream iss(result);
    std::string line;
    while (std::getline(iss, line))
    {
        // Trim whitespace
        if (line.empty()) continue;

        // Look for final nodes line: "Nodes searched: N"
        const std::string searchString = "Nodes searched: ";
        size_t pos = line.find(searchString);
        if (pos != std::string::npos)
        {
            std::string numberStr = line.substr(pos + searchString.length());
            out.nodes = std::stoll(numberStr);
            return out;
        }

        // Look for move lines: e.g. "e2e4: 20"
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            if (colonPos > 6) continue; // If debugging statement
            //std::cout << line << " for depth " << depth << "\n";
            std::string move = line.substr(0, colonPos);
            std::string number = line.substr(colonPos + 2, line.size()); // Get the number at that move
            // remove whitespace if any
            move.erase(remove_if(move.begin(), move.end(), ::isspace), move.end());
            if (!move.empty() && !number.empty())
            {
                uint64_t count = std::stoull(number);
                out.moves[move] = count;
            }
            continue;
        }

    }

    return out;
}

uint64_t Perft(Engine* engine, int depth)
{
    if (depth == 0)
    {
        return 1;
    }

    uint64_t nodes = 0;
    auto moves = BoardCalculator::GetAllLegalMoves(GameState::currentPlayer, engine->GetBitboardBoard(), engine);

    //std::cout << "My engine has " << moves.size() << " moves for depth " << depth << "\n";

    for (auto& move : moves)
    {
        engine->MakeMove(move);
        nodes += Perft(engine, depth - 1);
        engine->UndoMove();
    }

    return nodes;
}

void CompareMoveLists(const std::map<std::string, uint64_t>& myMap,
    const std::map<std::string, uint64_t>& sfMoves, Engine* engine)
{
    // Check for moves missing in either side or mismatched counts
    bool mismatchFound = false;
    for (const auto& [move, sfCount] : sfMoves)
    {
        auto it = myMap.find(move);
        if (it == myMap.end())
        {
            std::cout << "Missing move in my engine: " << move << " (Stockfish count: " << sfCount << ")\n";
            mismatchFound = true;
        }
        else
        {
            if (it->second != sfCount)
            {
                std::cout << "Move count mismatch for " << move
                    << ": My count=" << it->second
                    << " Stockfish count=" << sfCount << "\n";
                mismatchFound = true;
            }
        }
    }

    // Moves that exist in my engine but not in Stockfish
    for (const auto& [move, myCount] : myMap)
    {
        if (sfMoves.find(move) == sfMoves.end())
        {
            std::cout << "Extra move in my engine: " << move << " (My count: " << myCount << ")\n";
            mismatchFound = true;
        }
    }

    if (!mismatchFound)
    {
        std::cout << "All moves and counts match Stockfish!\n";
    }
    else
    {
        std::cout << engine->GetFEN() << '\n';
    }
}


void PerftDebug(Engine* engine, int depth, bool mismatch, std::vector<Move> path)
{
    //std::cout << "Depth: " << depth << "\n";
    if (depth == 0) return;

    auto moves = BoardCalculator::GetAllLegalMoves(GameState::currentPlayer, engine->GetBitboardBoard(), engine);

    std::map<std::string, uint64_t> myMoveCounts;
    for (auto& move : moves)
    {
        //std::cout << "Move: " << move.ToUCIString() << '\n';
        engine->MakeMove(move);

        uint64_t myCount = Perft(engine, depth - 2);

        engine->UndoMove();

        if (depth == 1)
            myCount = 0;

        myMoveCounts[MoveToUCI(move)] = myCount;
    }

    StockfishPerftResult sfCount = StockfishPerft("C:/Users/Joeger/Downloads/stockfish-windows-x86-64-avx2/stockfish/stockfish.exe",
        engine->GetFEN(), depth - 1);

    CompareMoveLists(myMoveCounts, sfCount.moves, engine);
}