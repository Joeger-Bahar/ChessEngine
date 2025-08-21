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
	Pieces type;
	Color color;
	Piece(Pieces t = Pieces::NONE, Color c = Color::WHITE) : type(t), color(c) {}
	// Overloader to return type when used in an expression
	operator Pieces() const { return type; }
	void Render() const;
};