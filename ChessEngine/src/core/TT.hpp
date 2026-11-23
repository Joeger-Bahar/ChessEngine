#pragma once

#include <vector>
#include <cstdint>

#include "move.hpp"

enum TTFlag : uint8_t { TT_UNKNOWN = 0, TT_EXACT = 1, TT_ALPHA = 2, TT_BETA = 3 };

constexpr int TT_NO_EVAL = -99999;

struct TTEntry
{
    uint64_t key;     // full zobrist key
    int32_t score;
    int32_t eval;     // Static eval
    uint32_t move32;  // encoded move (or index to Move pool); 0 = no move
    int16_t depth;    // ply or depth
    uint8_t flag;     // TTFlag
    uint8_t age;      // for simple replacement scheme
};

struct TranspositionTable
{
    std::vector<TTEntry> table;
    size_t entries; // number of entries (power of two)
    uint8_t currentAge = 0;

    TranspositionTable(int megabytes = 128);

    bool ttProbe(uint64_t key, int depth, int alpha, int beta, int& outScore, uint32_t& outMove, int* outEval = nullptr);
    void ttStore(uint64_t key, int depth, int score, uint32_t move32, uint8_t flag, int eval = TT_NO_EVAL);

    // Copies out the entry for key, false is there isn't one
    bool ttLookup(uint64_t key, TTEntry& out) const;

    inline size_t IndexFor(uint64_t key) const;

    void Clear();

    void NewSearch();

    Move GetBestMove(uint64_t key) const;
};
