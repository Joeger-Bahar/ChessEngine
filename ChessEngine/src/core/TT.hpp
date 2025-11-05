#pragma once

#include <vector>
#include <cstdint>

#include "move.hpp"

enum TTFlag : uint8_t { TT_UNKNOWN = 0, TT_EXACT = 1, TT_ALPHA = 2, TT_BETA = 3 };

struct TTEntry
{
    uint64_t key;     // full zobrist key
    int32_t score;
    int16_t depth;    // ply or depth
    uint32_t move32;  // encoded move (or index to Move pool); 0 = no move
    uint8_t flag;     // TTFlag
    uint8_t age;      // for simple replacement scheme
};

struct TranspositionTable
{
    std::vector<TTEntry> table;
    size_t entries; // number of entries (power of two)
    uint8_t currentAge = 0;

    TranspositionTable(int megabytes = 128);

    bool ttProbe(uint64_t key, int depth, int alpha, int beta, int& outScore, uint32_t& outMove);
    void ttStore(uint64_t key, int depth, int score, uint32_t move32, uint8_t flag);

    inline size_t IndexFor(uint64_t key) const;

    void Clear();

    void NewSearch();

    Move GetBestMove(uint64_t key) const;
};
