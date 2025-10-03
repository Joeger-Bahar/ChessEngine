#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

#include "core/engine.hpp"

struct StockfishPerftResult
{
    long long nodes;
    std::map<std::string, unsigned long long> moves; // UCI moves at the root
};

struct TmpFileGuard
{
    const std::string filename;
    TmpFileGuard(std::string fname) : filename(std::move(fname)) {}
    ~TmpFileGuard()
    {
        std::remove(filename.c_str());
    }
};

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
    auto moves = BoardCalculator::GetAllMoves(GameState::currentPlayer, engine->GetBoard(), engine);
    // Remove checks
    moves.erase(
        std::remove_if(moves.begin(), moves.end(),
            [&](Move move)
            {
                // Remove moves that cause checks
                engine->MakeMove(move);
                bool inCheck = engine->InCheck(Opponent(GameState::currentPlayer));
                engine->UndoMove();
                return inCheck;
            }),
        moves.end()
    );

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
    // Convert myMoves to UCI strings and compute perft counts
    //std::map<std::string, uint64_t> myMap;
    //for (const auto& move : myMoves)
    //{
    //    myMap[move.ToUCIString()] = 0; // Initialize with 0, will fill with actual counts if available
    //}

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
        std::cout << engine->GetFEN() << '\n';
}


void PerftDebug(Engine* engine, int depth, bool mismatch = false, std::vector<Move> path = {})
{
    //std::cout << "Depth: " << depth << "\n";
    if (depth == 0) return;

    auto moves = BoardCalculator::GetAllMoves(GameState::currentPlayer, engine->GetBoard(), engine);

    // Remove checks
    moves.erase(
        std::remove_if(moves.begin(), moves.end(),
            [&](Move move)
            {
                // Remove moves that cause checks
                engine->MakeMove(move);
                bool inCheck = engine->InCheck(Opponent(GameState::currentPlayer));
                //std::cout << "Move left player in check: " << move.ToUCIString() << '\n';
                engine->UndoMove();
                return inCheck;
            }),
        moves.end()
    );
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
    //std::vector<Move> newPath = path;
    //newPath.push_back(move);

    //if (myCount != sfCount.nodes)
    //{
    //    std::cout << "Mismatch at depth " << depth << " My count: " << myCount << " " << sfCount.nodes << " depth: " << depth <<  "!\n";
    //    std::cout << "Move path: ";
    //    for (auto& m : newPath) std::cout << m.ToUCIString() << " ";
    //    std::cout << "\n";
    //    std::cout << "FEN at mismatch: " << engine->GetFEN() << "\n";
    //    std::cout << "My count=" << myCount << " Stockfish count=" << sfCount.nodes;

    //    int numMovesHere = Perft(engine, 1);
    //    StockfishPerftResult sfMovesHere = StockfishPerft("C:/Users/Joeger/Downloads/stockfish-windows-x86-64-avx2/stockfish/stockfish.exe",
    //        engine->GetFEN(), 1);
    //    std::cout << "\nMoves for here: " << numMovesHere << " vs stockfish: " << sfMovesHere.nodes << "\n";

    //    auto hereMoves = BoardCalculator::GetAllLegalMoves(GameState::currentPlayer, engine->GetBoard(), engine);
    //    // Remove checks
    //    //hereMoves.erase(
    //    //    std::remove_if(hereMoves.begin(), hereMoves.end(),
    //    //        [&](Move move)
    //    //        {
    //    //            // Remove moves that cause checks
    //    //            engine->MakeMove(move);
    //    //            bool inCheck = engine->InCheck(Opponent(GameState::currentPlayer));
    //    //            engine->UndoMove();
    //    //            return inCheck;
    //    //        }),
    //    //    hereMoves.end()
    //    //);

    //    CompareMoveLists(hereMoves, sfMovesHere.moves);

    //    std::cout << "\n\n";

    //    // Recurse into this subtree to narrow down the bug
    //    PerftDebug(engine, depth - 1, true, newPath);
    //}
    //else
    //{
    //    if (mismatch && depth != 1)
    //    {
    //        PerftDebug(engine, depth - 1, true, newPath);
    //    }
    //}
}