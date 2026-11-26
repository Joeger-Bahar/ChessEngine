#pragma once

#include <cstdint>
#include <xmmintrin.h>
#include <atomic>
#include <memory>

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

// One entry as 3 64 bit words.
// Check holds key ^ data1 ^ data2, so if there is a race condition, the words
// don't fit together and the key test fails. Returns a miss instead of garbage data
// Lockles hasing. ReLaxed atomics compile to plain loads and stores on x64
struct TTSlot
{
    std::atomic<uint64_t> check{ 0 };
    std::atomic<uint64_t> data1{ 0 }; // Score (low 32 bits) | static eval (high 32 bits)
    std::atomic<uint64_t> data2{ 0 }; // move (32) | depth (16) | flag (8) | age (8)
};

// Two entries per index. alignas(64) puts each bucket in a 64 bytes cache line
// so both entries costs a single memory load
struct alignas(64) TTBucket
{
    TTSlot slot[2];
};

struct TranspositionTable
{
    TranspositionTable(int megabytes = 128);

    bool ttProbe(uint64_t key, int depth, int alpha, int beta, int& outScore, uint32_t& outMove, int* outEval = nullptr);
    void ttStore(uint64_t key, int depth, int score, uint32_t move32, uint8_t flag, int eval = TT_NO_EVAL);

    // Copies out the entry for key, false is there isn't one
    bool ttLookup(uint64_t key, TTEntry& out) const;

    // Start loading this positions bucket into the cache now
    void Prefetch(uint64_t key) const { _mm_prefetch((const char*)&table[key & (buckets - 1)], _MM_HINT_T0); }

    void Clear();

    void Resize(int megabytes); // Reallocates and clears the table

    void NewSearch();

    Move GetBestMove(uint64_t key) const;

private:
    // The bucket holding this position
    bool Find(uint64_t key, TTEntry& out) const;

    std::unique_ptr<TTBucket[]> table;
    size_t buckets = 0; // Number of buckets. Is a power of 2
    uint8_t currentAge = 0;
};

// Singleton table for all the threads
TranspositionTable& SharedTT();
