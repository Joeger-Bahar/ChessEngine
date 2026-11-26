#pragma once

#include "core/engine.hpp"
#include "bot/bot.hpp"
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

class Uci
{
public:
    Uci(Engine* engine, Bot* bot);
    void Loop();

private:
    void HandleCommand(const std::string& line);
    void HandlePosition(std::istringstream& is);
    void HandleGo(std::istringstream& is);
    void HandleSetOption(std::istringstream& is);
    
    // Sets up a position from text after "position"
    static void SetPosition(Engine* engine, const std::string& positionLine);
    static Move ParseMove(Engine* engine, const std::string& moveString);

    // Lazy SMP helper threads
    void StartHelpers(Color us);
    void StopHelpers();


    Engine* engine;
    Bot* bot;
    int hashMB = 128; // UCI "Hash" option for TT
    int threads = 1;  // UCI "Threads" option

    // For Lazy SMP
    std::string positionLine = "startpos"; // Last "position" command
    std::vector<std::unique_ptr<Bot>> helperBots; // Kept between moves for history tables
    std::vector<std::thread> helperThreads;
};