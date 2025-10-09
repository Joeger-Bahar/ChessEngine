#pragma once

#include <random>
#include <cstdint>
#include <vector>
#include <array>

#include "square.hpp"
#include "gameState.hpp"
#include "constants.hpp"

struct Zobrist
{
    uint64_t piece[12][64];   // Piece[typeIndex 0..11][sq 0..63]
    uint64_t sideToMove;
    uint64_t castling[4];     // Order: WK, WQ, BK, BQ (choose your mapping)
    uint64_t enPassantFile[8]; // File 0..7

    Zobrist();

    void init();
};