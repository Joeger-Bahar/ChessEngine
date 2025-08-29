#pragma once

#include <string>
#include <vector>
#include <array>

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
	const std::string Render(); // Returns the string representation of the piece

	Color GetColor() const;

	static void AddKnightAttacks(int row, int col, std::array<bool, 64>& attacked);
	static void AddSlidingAttacks(int row, int col, std::array<bool, 64>& attacked, const std::vector<std::pair<int, int>>& dirs);
	static void AddPawnAttacks(int row, int col, Color color, std::array<bool, 64>& attacked);

	Pieces type;
	Color color;
};