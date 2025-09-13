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
    uint16_t move;   // polyglot encoded
    uint16_t weight; // frequency
};

extern std::unordered_map<uint64_t, std::vector<BookMove>> book;

void LoadPolyglot(const std::string& filename);
Move GetBookMove(Engine* engine);
Move PolyglotToMove(uint16_t pmove);