#pragma once

#include <random>
#include <cstdint>
#include <vector>

#include "square.hpp"
#include "gameState.hpp"

struct Zobrist {
    uint64_t piece[12][64];   // piece[typeIndex 0..11][sq 0..63]
    uint64_t sideToMove;
    uint64_t castling[4];     // order: WK, WQ, BK, BQ (choose your mapping)
    uint64_t enPassantFile[8]; // file 0..7

    Zobrist();

    void init();
};