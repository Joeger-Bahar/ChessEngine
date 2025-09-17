#include "TT.hpp"

void TranspositionTable::Clear()
{
    for (auto& e : table) e.key = 0;
}

void TranspositionTable::NewSearch()
{
    // increment age at each root search
    currentAge++; if (currentAge == 0) currentAge = 1;
} 

TranspositionTable::TranspositionTable(int megabytes)
{
    size_t bytes = megabytes * 1024ull * 1024ull;
    entries = bytes / sizeof(TTEntry);
    unsigned long index;
    // round down to power of 2 for fast masking     what the fuck
    if (_BitScanReverse64(&index, entries))
    {
        size_t p2 = 1ull << index;
        entries = p2;
    }
    table.resize(entries);
    
    Clear();
}

bool TranspositionTable::ttProbe(uint64_t key, int depth, int alpha, int beta, int& outScore, uint32_t& outMove)
{
    size_t idx = IndexFor(key);
    TTEntry& e = table[idx];
    if (e.key == key)
    {
        outMove = e.move32;
        if (e.depth >= depth)
        {
            if (e.flag == TT_EXACT)
            {
                outScore = e.score;
                return true;
            }
            else if (e.flag == TT_ALPHA && e.score <= alpha)
            {
                outScore = e.score;
                return true;
            }
            else if (e.flag == TT_BETA && e.score >= beta)
            {
                outScore = e.score;
                return true;
            }
        }
    }
    return false;
}

// store an entry
void TranspositionTable::ttStore(uint64_t key, int depth, int score, uint32_t move32, uint8_t flag)
{
    size_t idx = IndexFor(key);
    TTEntry& e = table[idx];

    // replacement scheme: keep deeper searches and/or newer ages
    bool replace = false;
    if (e.key == 0) replace = true; // empty
    else if (e.key == key) replace = true; // update same key
    else if (depth > e.depth) replace = true; // deeper preferred
    else if (e.age != currentAge) replace = true; // newer age
    // you can refine: always replace if depth >= e.depth-2 etc

    if (replace)
    {
        e.key = key;
        e.depth = depth;
        e.score = score;
        e.move32 = move32;
        e.flag = flag;
        e.age = currentAge;
    }
}

inline size_t TranspositionTable::IndexFor(uint64_t key) const
{
    return (size_t)(key & (entries - 1));
}
