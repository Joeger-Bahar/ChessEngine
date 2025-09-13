#include "opening.hpp"

#include <iostream>
#include <cstdint>
#include <string>

std::unordered_map<uint64_t, std::vector<BookMove>> book;

void LoadPolyglot(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file) return;

    while (true)
    {
        uint64_t key;
        uint16_t move, weight, learn;
        file.read(reinterpret_cast<char*>(&key), 8);
        if (!file) break;

        file.read(reinterpret_cast<char*>(&move), 2);
        file.read(reinterpret_cast<char*>(&weight), 2);
        file.read(reinterpret_cast<char*>(&learn), 2);
        file.ignore(2); // rest of learn

        // Polyglot files are big-endian, swap bytes
        auto swap64 = [](uint64_t x)
            {
                return ((x & 0xFF00000000000000ULL) >> 56) |
                    ((x & 0x00FF000000000000ULL) >> 40) |
                    ((x & 0x0000FF0000000000ULL) >> 24) |
                    ((x & 0x000000FF00000000ULL) >> 8) |
                    ((x & 0x00000000FF000000ULL) << 8) |
                    ((x & 0x0000000000FF0000ULL) << 24) |
                    ((x & 0x000000000000FF00ULL) << 40) |
                    ((x & 0x00000000000000FFULL) << 56);
            };

        auto swap16 = [](uint16_t x) { return (x >> 8) | (x << 8); };

        key = swap64(key);
        move = swap16(move);
        weight = swap16(weight);

        book[key].push_back({ move, weight });
    }
}

Move GetBookMove(Engine* engine)
{
    uint64_t key = engine->ComputeFullHash();
    auto it = book.find(key);
    if (it == book.end())
    {
        //std::cout << "Didn't find string\n";
        return Move();
    }

    // Weighted random choice
    int total = 0;
    for (auto& bm : it->second) total += bm.weight;

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, total - 1);
    int r = dist(rng);

    for (auto& bestMove : it->second)
    {
        if (r < bestMove.weight)
        {
            return PolyglotToMove(bestMove.move); // Decode into your Move struct
        }
        r -= bestMove.weight;
    }
    //std::cout << "Didn't find a best move\n";
    return Move();
}

Move PolyglotToMove(uint16_t pmove)
{
    int to = pmove & 0x3F;
    int from = (pmove >> 6) & 0x3F;
    int promo = (pmove >> 12) & 0x7;

    // Polyglot: 0=a1 … 63=h8
    int fromFile = from % 8;
    int fromRank = from / 8;
    int toFile = to % 8;
    int toRank = to / 8;

    // Flip rank to match engine's (0=top=a8)
    int fromRow = 7 - fromRank;
    int fromCol = fromFile;
    int toRow = 7 - toRank;
    int toCol = toFile;

    Pieces promotion = Pieces::NONE;
    if (promo == 1) promotion = Pieces::KNIGHT;
    else if (promo == 2) promotion = Pieces::BISHOP;
    else if (promo == 3) promotion = Pieces::ROOK;
    else if (promo == 4) promotion = Pieces::QUEEN;

    return Move(fromCol, fromRow, toCol, toRow, static_cast<int>(promotion));
}

