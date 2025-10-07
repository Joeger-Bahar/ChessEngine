#pragma once

#include <cstdint>

#include "piece.hpp"

using Bitboard = uint64_t;

constexpr Bitboard EMPTY_BITBOARD = 0ULL;
constexpr Bitboard FULL_BITBOARD = ~EMPTY_BITBOARD;

inline bool IsSet(const Bitboard& b, int square)  { return b & (1ULL << square); }
inline void Set  (Bitboard& b, int square) { b |=  (1ULL << square); }
inline void Clear(Bitboard& b, int square) { b &= ~(1ULL << square); }

struct BitboardBoard
{
    Bitboard pieceBitboards[2][6];
    Bitboard allPieces[2];
    Bitboard occupied;

	inline void Add(const Piece& p, int sq)
	{
		Bitboard mask = 1ULL << sq;
		int c = (p.GetColor() == Color::WHITE ? 0 : 1);
		int t = static_cast<int>(p.GetType()) - 1; // PAWN starts at 1

		pieceBitboards[c][t] |= mask;
		allPieces[c] |= mask;
		occupied |= mask;
	}

	inline void Remove(const Piece& p, int sq)
	{
		Bitboard mask = 1ULL << sq;
		int c = (p.GetColor() == Color::WHITE ? 0 : 1);
		int t = static_cast<int>(p.GetType()) - 1;

		pieceBitboards[c][t] &= ~mask;
		allPieces[c] &= ~mask;
		occupied &= ~mask;
	}
};