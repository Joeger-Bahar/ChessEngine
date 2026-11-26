#pragma once

#include "core/engine.hpp"
#include "bot/bot.hpp"
#include <string>
#include <sstream>

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
    Move ParseMove(const std::string& moveString);

    Engine* engine;
    Bot* bot;
    int hashMB = 128; // UCI "Hash" option for TT
    int threads = 1;
};