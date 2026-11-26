#include "TT.hpp"

// Unnamed namespace as only this file uses them
namespace
{
    constexpr std::memory_order RELAXED = std::memory_order_relaxed;

    uint64_t PackData1(int score, int eval)
    {
        return (uint64_t)(uint32_t)score | ((uint64_t)(uint32_t)eval << 32);
    }

    uint64_t PackData2(uint32_t move32, int depth, uint8_t flag, uint8_t age)
    {
        return (uint64_t)move32
            | ((uint64_t)(uint16_t)(int16_t)depth << 32)
            | ((uint64_t)flag << 48)
            | ((uint64_t)age << 56);
    }

    // Unpacks a slot into out. out.key is the key it holds: 0 if it's empty, and some
    // other position's key if another thread was writing it at the same time.
    void ReadSlot(const TTSlot& slot, TTEntry& out)
    {
        uint64_t check = slot.check.load(RELAXED);
        uint64_t d1 = slot.data1.load(RELAXED);
        uint64_t d2 = slot.data2.load(RELAXED);

        out.key = check ^ d1 ^ d2;
        out.score = (int32_t)(uint32_t)d1;
        out.eval = (int32_t)(uint32_t)(d1 >> 32);
        out.move32 = (uint32_t)d2;
        out.depth = (int16_t)(uint16_t)(d2 >> 32);
        out.flag = (uint8_t)(d2 >> 48);
        out.age = (uint8_t)(d2 >> 56);
    }

    void WriteSlot(TTSlot& slot, uint64_t key, uint64_t d1, uint64_t d2)
    {
        slot.data1.store(d1, RELAXED);
        slot.data2.store(d2, RELAXED);
        slot.check.store(key ^ d1 ^ d2, RELAXED);
    }
}

void TranspositionTable::Clear()
{
    for (size_t i = 0; i < buckets; ++i)
        for (TTSlot& slot : table[i].slot)
            WriteSlot(slot, 0, 0, 0);
}

void TranspositionTable::Resize(int megabytes)
{
    // Largest power of two that fits
    size_t bytes = (size_t)megabytes * 1024 * 1024;
    size_t count = 1;
    while (count * 2 * sizeof(TTBucket) <= bytes)
        count *= 2;

    if (count == buckets && table)
    {
        Clear();
        return;
    }
    table.reset(); // Free the old table
    table.reset(new TTBucket[count]); // Starts zeroed
    buckets = count;
}

void TranspositionTable::NewSearch()
{
    // increment age at each root search
    currentAge++; if (currentAge == 0) currentAge = 1;
}

Move TranspositionTable::GetBestMove(uint64_t key) const
{
    TTEntry e;
    if (Find(key, e) && e.move32 != 0)
        return (Move)e.move32;
    return Move(); // null move if none
}

bool TranspositionTable::Find(uint64_t key, TTEntry& out) const
{
    TTBucket& bucket = table[key & (buckets - 1)];
    for (const TTSlot& slot : bucket.slot)
    {
        ReadSlot(slot, out);
        if (out.key == key)
            return true;
    }
    return false;
}

TranspositionTable::TranspositionTable(int megabytes)
{
    Resize(megabytes);
}

bool TranspositionTable::ttProbe(uint64_t key, int depth, int alpha, int beta, int& outScore, uint32_t& outMove, int* outEval)
{
    TTEntry e;
    if (!Find(key, e))
        return false;

    outMove = e.move32;
    if (outEval) *outEval = e.eval;

    if (e.depth >= depth)
    {
        if (e.flag == TT_EXACT
            || (e.flag == TT_ALPHA && e.score <= alpha)
            || (e.flag == TT_BETA && e.score >= beta))
        {
            outScore = e.score;
            return true;
        }
    }

    return false;
}

// store an entry
void TranspositionTable::ttStore(uint64_t key, int depth, int score, uint32_t move32, uint8_t flag, int eval)
{
    TTBucket& bucket = table[key & (buckets - 1)];
    TTEntry e[2];
    ReadSlot(bucket.slot[0], e[0]);
    ReadSlot(bucket.slot[1], e[1]);

    // Which slot to use? The one alr holding this position or the less valuable one
    // New result always gets stored
    int target;
    if (e[0].key == key)
        target = 0;
    else if (e[1].key == key)
        target = 1;
    else
    {
        auto worth = [&](const TTEntry& x)
            {
                if (x.key == 0) return -1000;

                return x.depth - (x.age != currentAge ? 100 : 0);
            };
        target = worth(e[0]) <= worth(e[1]) ? 0 : 1;
    }

    if (e[target].key == key)
    {
        // Same position so keep the stored move and eval
        if (move32 == 0)
            move32 = e[target].move32;
        if (eval == TT_NO_EVAL)
            eval = e[target].eval;
    }

    WriteSlot(bucket.slot[target], key, PackData1(score, eval), PackData2(move32, depth, flag, currentAge));
}

bool TranspositionTable::ttLookup(uint64_t key, TTEntry& out) const
{
    return Find(key, out);
}

TranspositionTable& SharedTT()
{
    static TranspositionTable table(128);
    return table;
}
