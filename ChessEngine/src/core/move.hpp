#pragma once

#include "piece.hpp"
#include "square.hpp"
#include "bitboard.hpp"

#include <cstdint>
#include <string>

using Move = uint32_t;

inline Move EncodeMove(int start, int end, int promotion = static_cast<int>(Pieces::NONE), bool enPassant = false, bool castle = false)
{
    return (start & 0x3F) |
        ((end & 0x3F) << 6) |
        ((promotion & 0x7) << 12) |
        ((enPassant ? 1 : 0) << 15) |
        ((castle ? 1 : 0) << 16);
}

inline int  GetStart(Move m)       { return m         & 0x3F; }
inline int  GetEnd(Move m)         { return (m >> 6)  & 0x3F; }
inline int  GetPromotion(Move m)   { return (m >> 12) & 0x7; }
inline bool IsEnPassant(Move m)    { return (m >> 15) & 1; }
inline bool IsCastle(Move m)       { return (m >> 16) & 1; }

const char* MoveToUCI(Move m);
Move MoveFromUCI(const std::string& uci, const Square board[64]);
bool MoveIsNull(Move m);
bool MoveIsCapture(Move m, const BitboardBoard& board);