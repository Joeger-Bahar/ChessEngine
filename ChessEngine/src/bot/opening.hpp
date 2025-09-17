#pragma once

#include <fstream>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <random>

#include "core/move.hpp"
#include "core/engine.hpp"

struct BookMove
{
    uint16_t move;   // Polyglot encoded
    uint16_t weight; // Frequency
};

//extern std::unordered_map<uint64_t, std::vector<BookMove>> book;

//void LoadPolyglot(const std::string& filename);
Move GetBookMove(Engine* engine, const std::string& filename);// , double randomness = 1.0);
Move PolyglotToMove(uint16_t pmove, Engine* engine);