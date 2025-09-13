#pragma once

#include "piece.hpp"

#include <cstdint>

struct Move
{
	Move();
	int startCol, startRow;
	int endCol, endRow;
	int promotion;     // Promotion piece, if any
	bool wasEnPassant, wasCastle;

	const char* ToString() const;
    const char* ToUCIString() const;
	bool IsNull() const;
    bool operator==(const Move& other) const
    {
        return startCol == other.startCol &&
            startRow == other.startRow &&
            endCol == other.endCol &&
            endRow == other.endRow &&
            promotion == other.promotion &&
            wasEnPassant == other.wasEnPassant &&
            wasCastle == other.wasCastle;
    }
};