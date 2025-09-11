#pragma once

#include "piece.hpp"

#include <cstdint>

struct Move
{
	Move();
	uint8_t startCol, startRow;
	uint8_t endCol, endRow;
	uint8_t promotion;     // Promotion piece, if any
	bool wasEnPassant, wasCastle;

	const char* ToString() const;
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