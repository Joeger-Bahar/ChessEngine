#include "zobrist.hpp"

#include <iostream>

Zobrist::Zobrist()
{
	init();
}

void Zobrist::init()
{
    for (int p = 0; p < 12; ++p)
        for (int s = 0; s < 64; ++s)
            piece[p][s] = Random64[(p * 64) + s];

    castling[0] = Random64[768]; // WK
    castling[1] = Random64[769]; // WQ
    castling[2] = Random64[770]; // BK
    castling[3] = Random64[771]; // BQ

    for (int f = 0; f < 8; ++f)
        enPassantFile[f] = Random64[772 + f];

    sideToMove = Random64[780];
}