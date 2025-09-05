#pragma once

#include "piece.hpp"

struct Move
{
	uint8_t startCol, startRow;
	uint8_t endCol, endRow;
	Piece capturedPiece = Pieces::NONE; // Piece that was captured, if any
	Pieces promotion = Pieces::NONE;    // Promotion piece, if any
};