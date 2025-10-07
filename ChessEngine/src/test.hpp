#pragma once

#include <map>
#include <vector>
#include <string>
#include <cstdint>

#include "core/engine.hpp"

struct StockfishPerftResult
{
    long long nodes;
    std::map<std::string, unsigned long long> moves; // UCI moves at the root
};

struct TmpFileGuard
{
    const std::string filename;
    TmpFileGuard(std::string fname) : filename(std::move(fname)) {}
    ~TmpFileGuard()
    {
        std::remove(filename.c_str());
    }
};

StockfishPerftResult StockfishPerft(const std::string& stockfishPath, const std::string& fen, int depth);
uint64_t Perft(Engine* engine, int depth);
void CompareMoveLists(const std::map<std::string, uint64_t>& myMap,
    const std::map<std::string, uint64_t>& sfMoves, Engine* engine);
void PerftDebug(Engine* engine, int depth, bool mismatch = false, std::vector<Move> path = {});