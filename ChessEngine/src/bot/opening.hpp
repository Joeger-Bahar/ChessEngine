#pragma once

#include <cstdint>

#include "core/move.hpp"
#include "core/engine.hpp"

struct BookMove
{
    uint16_t move;   // Polyglot encoded
    uint16_t weight; // Frequency
};

Move GetBookMove(Engine* engine, const std::string& filename);
Move PolyglotToMove(uint16_t pmove, Engine* engine);