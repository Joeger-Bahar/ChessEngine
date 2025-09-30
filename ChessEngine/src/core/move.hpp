#pragma once

#include "piece.hpp"
#include "square.hpp"

#include <cstdint>
#include <string>

struct Move
{
	Move(int startSquare = 0, int endSquare = 0, int promotion = static_cast<int>(Pieces::NONE), bool wasEnPassant = false, bool wasCastle = false);
	uint8_t startSquare;
	uint8_t endSquare;
	uint8_t promotion;     // Promotion piece, if any
    bool wasEnPassant : 1;
    bool wasCastle : 1;

    const char* ToUCIString() const;
    static Move FromUCI(const std::string& uci, const Square board[64]);
	bool IsNull() const;
    bool IsCapture(const Square board[64]) const;
    bool operator==(const Move& other) const
    {
        return startSquare == other.startSquare &&
               endSquare == other.endSquare &&
               promotion == other.promotion &&
               wasEnPassant == other.wasEnPassant &&
               wasCastle == other.wasCastle;
    }
};