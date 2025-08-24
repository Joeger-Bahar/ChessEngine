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

struct Square;

struct Piece {
	Piece(Pieces t = Pieces::NONE, Color c = Color::NONE);
	// Overloader to return type when used in an expression
	operator Pieces() const;
	void Render() const;
	// End numbers are references to change if pieces are in the way
	bool ValidMove(int startRow, int startCol, int& endRow, int& endCol, Square board[8][8]);

	Color GetColor() const;

	Pieces type;
	Color color;
};