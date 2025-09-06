#include "piece.hpp"

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
	case Pieces::KING:   return (color == Color::WHITE) ? "K" : "k";
	case Pieces::QUEEN:  return (color == Color::WHITE) ? "Q" : "q";
	case Pieces::ROOK:   return (color == Color::WHITE) ? "R" : "r";
	case Pieces::BISHOP: return (color == Color::WHITE) ? "B" : "b";
	case Pieces::KNIGHT: return (color == Color::WHITE) ? "N" : "n";
	case Pieces::PAWN:   return (color == Color::WHITE) ? "P" : "p";
	default:             return ".";
	}
}
