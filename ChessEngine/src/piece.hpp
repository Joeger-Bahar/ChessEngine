#pragma once

enum class Color {
	WHITE,
	BLACK,
	NONE
};

enum class Pieces {
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
	NONE
};

struct Piece {
	Piece(Pieces t = Pieces::NONE, Color c = Color::NONE);
	// Overloader to return piece type when used in an expression
	operator Pieces() const;

	Color GetColor() const;

	Pieces type;
	Color color;
};