#include "piece.hpp"

Piece::Piece(Pieces t, Color c)
	: type(t), color(c)
{}

Piece::operator Pieces() const
{
	return type; // Allows Piece to be used as Pieces enum
}

Color Piece::GetColor() const
{
	return color; // Returns the color of the piece
}
