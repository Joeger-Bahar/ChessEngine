#include "square.hpp"

Square::Square(Piece p)
	: piece(p)
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

bool Square::IsEmpty() const
{
	return piece.GetType() == Pieces::NONE;
}
