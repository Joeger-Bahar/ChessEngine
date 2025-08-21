#include "square.hpp"

Square::Square(Color c, Piece p)
	: color(c), piece(p)
{
}

Piece Square::GetPiece() const
{
	return piece;
}
void Square::SetPiece(Piece p)
{
	piece = p;
}

Color Square::GetColor() const
{
	return color;
}
void Square::SetColor(Color c)
{
	color = c;
}

bool Square::IsEmpty() const
{
	return piece.type == Pieces::NONE;
}
