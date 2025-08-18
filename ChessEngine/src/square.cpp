#include "square.hpp"
#include "piece.hpp"

enum Color {
	WHITE,
	BLACK
};

struct Square
{
	Color color;
	Piece piece;
};