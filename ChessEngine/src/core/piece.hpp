#pragma once

#include <cstdint>

enum class Color : uint8_t {
	WHITE,
	BLACK,
	NONE
};

enum class Pieces : uint8_t {
	NONE,
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
};

struct Piece
{
	Piece(Pieces t = Pieces::NONE, Color c = Color::NONE);
	// Comparison operators
	friend bool operator==(const Piece& a, const Piece& b)
	{
		return a.type == b.type && a.color == b.color;
	}

	Color GetColor() const;
	Pieces GetType() const;
	const char* ToString() const;

	Pieces type;
	Color color;
};