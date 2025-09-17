#include "opening.hpp"

#include <iostream>
#include <cstdint>
#include <string>

//std::unordered_map<uint64_t, std::vector<BookMove>> book;

struct PolyglotEntry
{
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint16_t learn;
};

uint64_t Swap64(uint64_t x)
{
    return  ((x & 0xFF00000000000000ULL) >> 56) |
            ((x & 0x00FF000000000000ULL) >> 40) |
            ((x & 0x0000FF0000000000ULL) >> 24) |
            ((x & 0x000000FF00000000ULL) >> 8)  |
            ((x & 0x00000000FF000000ULL) << 8)  |
            ((x & 0x0000000000FF0000ULL) << 24) |
            ((x & 0x000000000000FF00ULL) << 40) |
            ((x & 0x00000000000000FFULL) << 56);
}
uint16_t Swap16(uint16_t x) { return (x >> 8) | (x << 8); }

std::vector<BookMove> LookupPolyglot(const std::string& filename, uint64_t searchKey)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) return {};

    std::streamsize size = file.tellg();
    size_t count = size / 16;
    size_t lo = 0, hi = count - 1;

    // Binary search for first matching key
    while (lo <= hi)
    {
        size_t mid = (lo + hi) / 2;
        file.seekg(mid * 16);
        uint64_t rawKey;
        file.read(reinterpret_cast<char*>(&rawKey), 8);
        uint64_t key = Swap64(rawKey);

        if (key < searchKey) lo = mid + 1;
        else if (key > searchKey) hi = mid - 1;
        else
        {
            // Found one, now collect all consecutive entries with same key
            std::vector<BookMove> moves;

            // Scan backwards
            size_t idx = mid;
            while (idx > 0)
            {
                file.seekg((idx - 1) * 16);
                uint64_t rk; file.read(reinterpret_cast<char*>(&rk), 8);
                if (Swap64(rk) != searchKey) break;
                idx--;
            }

            // Scan forwards
            for (; idx < count; idx++)
            {
                file.seekg(idx * 16);
                PolyglotEntry e;
                file.read(reinterpret_cast<char*>(&e.key), 8);
                file.read(reinterpret_cast<char*>(&e.move), 2);
                file.read(reinterpret_cast<char*>(&e.weight), 2);
                file.read(reinterpret_cast<char*>(&e.learn), 2);
                file.ignore(2);

                if (Swap64(e.key) != searchKey) break;
                moves.push_back({ Swap16(e.move), Swap16(e.weight) });
            }
            return moves;
        }
    }
    return {};
}

//void LoadPolyglot(const std::string& filename)
//{
//    std::ifstream file(filename, std::ios::binary);
//    if (!file) return;
//
//    while (true)
//    {
//        uint64_t key;
//        uint16_t move, weight, learn;
//        file.read(reinterpret_cast<char*>(&key), 8);
//        if (!file) break;
//
//        file.read(reinterpret_cast<char*>(&move), 2);
//        file.read(reinterpret_cast<char*>(&weight), 2);
//        file.read(reinterpret_cast<char*>(&learn), 2);
//        file.ignore(2); // Rest of learn
//
//        // Polyglot files are big-endian, swap bytes
//        key = Swap64(key);
//        move = Swap16(move);
//        weight = Swap16(weight);
//
//        book[key].push_back({ move, weight });
//    }
//}

//Move GetBookMove(Engine* engine)
//{
//    uint64_t key = engine->ComputeFullHash();
//    auto it = book.find(key);
//    if (it == book.end())
//    {
//        //std::cout << "Didn't find string\n";
//        return Move();
//    }
//
//    // Weighted random choice
//    int total = 0;
//    for (auto& bm : it->second) total += bm.weight;
//
//    std::mt19937 rng(std::random_device{}());
//    std::uniform_int_distribution<int> dist(0, total - 1);
//    int r = dist(rng);
//
//    for (auto& bestMove : it->second)
//    {
//        if (r < bestMove.weight)
//        {
//            return PolyglotToMove(bestMove.move, engine); // Decode into your Move struct
//        }
//        r -= bestMove.weight;
//    }
//    //std::cout << "Didn't find a best move\n";
//    return Move();
//}
Move GetBookMove(Engine* engine, const std::string& filename)
{
    uint64_t key = engine->ComputeFullHash();
    auto entries = LookupPolyglot(filename, key);
    if (entries.empty()) return Move();

    int total = 0;
    for (auto& bm : entries) total += bm.weight;

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, total - 1);
    int r = dist(rng);

    for (auto& bm : entries) {
        if (r < bm.weight)
            return PolyglotToMove(bm.move, engine);
        r -= bm.weight;
    }
    return Move();
}

Move PolyglotToMove(uint16_t pmove, Engine* engine)
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

    bool wasCastle = false;
    bool wasEnPassant = false;

    // Detect castling (engine rows: white = row 7, black = row 0)
    if (engine->GetBoard()[fromRow][fromCol].GetPiece().GetType() == Pieces::KING)
    {
        if (std::abs(fromCol - toCol) > 1) // King moved more than 1 space
        {
            wasCastle = true;
            // Convert polyglot into engine expected movement for castling
            if (toCol == 7) toCol = 6;
            if (toCol == 0 || toCol == 1) toCol = 2;
        }
    }

    // Detect en passant
    if (engine->GetBoard()[fromRow][fromCol].GetPiece().GetType() == Pieces::PAWN)
    {
        if (toRow == GameState::enPassantTarget[0] && toCol == GameState::enPassantTarget[1])
        {
            wasEnPassant = true;
        }
    }

    return Move(fromCol, fromRow, toCol, toRow, static_cast<int>(promotion), wasEnPassant, wasCastle);
}

