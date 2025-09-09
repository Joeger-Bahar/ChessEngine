#include "zobrist.hpp"

enum PieceTypes : int {
    EMPTY = 0,
    W_PAWN = 1, W_KNIGHT = 2, W_BISHOP = 3, W_ROOK = 4, W_QUEEN = 5, W_KING = 6,
    B_PAWN = 7, B_KNIGHT = 8, B_BISHOP = 9, B_ROOK = 10, B_QUEEN = 11, B_KING = 12
};
inline bool is_white_piece(int p) { return p >= W_PAWN && p <= W_KING; }
inline bool is_black_piece(int p) { return p >= B_PAWN && p <= B_KING; }

Zobrist::Zobrist()
{
	init();
}

void Zobrist::init()
{
    std::mt19937_64 rng(0xC0FFEE); // fixed seed for reproducibility; change to random_device if desired
    std::uniform_int_distribution<uint64_t> dist;
    for (int p = 0; p < 12; ++p)
        for (int s = 0; s < 64; ++s)
            piece[p][s] = dist(rng);

    sideToMove = dist(rng);
    for (int i = 0; i < 4; ++i) castling[i] = dist(rng);
    for (int f = 0; f < 8; ++f) enPassantFile[f] = dist(rng);
}