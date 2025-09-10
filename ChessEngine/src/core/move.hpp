#pragma once

#include "piece.hpp"

#include <cstdint>

struct Move
{
	Move();
	uint8_t startCol, startRow;
	uint8_t endCol, endRow;
	//Piece capturedPiece; // Piece that was captured, if any
	uint8_t promotion;     // Promotion piece, if any
	bool wasEnPassant, wasCastle;

	const char* ToString() const;
	bool operator==(const Move& right);

};