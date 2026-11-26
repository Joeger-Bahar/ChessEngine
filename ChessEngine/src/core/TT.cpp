#include "TT.hpp"

void TranspositionTable::Clear()
{
    for (auto& e : table) e.key = 0;
}

void TranspositionTable::Resize(int megabytes)
{
    // Largest power of two that fits
    size_t bytes = (size_t)megabytes * 1024 * 1024;
    size_t count = 1;
    while (count * 2 * sizeof(TTEntry) <= bytes)
        count *= 2;

    if (count == entries && table.size() == count)
    {
        Clear();
        return;
    }
    entries = count;
    table.assign(entries, TTEntry{});
}

void TranspositionTable::NewSearch()
{
    // increment age at each root search
    currentAge++; if (currentAge == 0) currentAge = 1;
}

Move TranspositionTable::GetBestMove(uint64_t key) const
{
    size_t idx = IndexFor(key);
    const TTEntry& e = table[idx];
    if (e.key == key && e.move32 != 0)
        return static_cast<Move>(e.move32);
    return Move(); // null move if none
}

TranspositionTable::TranspositionTable(int megabytes)
{
    Resize(megabytes);
}

bool TranspositionTable::ttProbe(uint64_t key, int depth, int alpha, int beta, int& outScore, uint32_t& outMove, int* outEval)
{
    size_t idx = IndexFor(key);
    TTEntry& e = table[idx];
    if (e.key == key)
    {
        outMove = e.move32;
        if (outEval) *outEval = e.eval;
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
void TranspositionTable::ttStore(uint64_t key, int depth, int score, uint32_t move32, uint8_t flag, int eval)
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
        // A fail-low search has no best move: keep the one already stored for this position
        if (move32 == 0 && e.key == key)
            move32 = e.move32;

        if (eval == TT_NO_EVAL && e.key == key)
            eval = e.eval;

        e.key = key;
        e.depth = depth;
        e.score = score;
        e.move32 = move32;
        e.flag = flag;
        e.age = currentAge;
        e.eval = eval;

    }
}

bool TranspositionTable::ttLookup(uint64_t key, TTEntry& out) const
{
    const TTEntry& e = table[IndexFor(key)];
    if (e.key != key) return false;
    out = e;
    return true;
}

inline size_t TranspositionTable::IndexFor(uint64_t key) const
{
    return (size_t)(key & (entries - 1));
}
