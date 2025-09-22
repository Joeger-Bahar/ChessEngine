#include "piece.hpp"
#include "boardCalculator.hpp"

Piece::Piece(Pieces t, Color c)
	: type(t), color(c)
{}

Color Piece::GetColor() const
{
	return color; // Returns the color of the piece
}

Pieces Piece::GetType() const
{
	return type;
}

const char* Piece::ToString() const
{
	switch (type)
	{
	case Pieces::KING:   return IsWhite(color) ? "K" : "k";
	case Pieces::QUEEN:  return IsWhite(color) ? "Q" : "q";
	case Pieces::ROOK:   return IsWhite(color) ? "R" : "r";
	case Pieces::BISHOP: return IsWhite(color) ? "B" : "b";
	case Pieces::KNIGHT: return IsWhite(color) ? "N" : "n";
	case Pieces::PAWN:   return IsWhite(color) ? "P" : "p";
	default:             return ".";
	}
}
