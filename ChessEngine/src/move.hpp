#pragma once

#include "piece.hpp"

struct Move
{
	uint8_t startCol, startRow;
	uint8_t endCol, endRow;
	Piece capturedPiece; // Piece that was captured, if any
	Piece promotion;     // Promotion piece, if any
};