#include "piece.hpp"

#include <iostream>

// TODO: Use a dictionary for piece rendering

Piece::Piece(Pieces t, Color c)
	: type(t), color(c)
{}

Piece::operator Pieces() const
{
	return type; // Allows Piece to be used as Pieces enum
}

const std::string Piece::Render()
{
	std::string output;
	switch (type)
	{
	case Pieces::PAWN:
		output.append((color == Color::WHITE ? "P" : "p"));
		break;
	case Pieces::KNIGHT:
		output.append((color == Color::WHITE ? "N" : "n"));
		break;
	case Pieces::BISHOP:
		output.append((color == Color::WHITE ? "B" : "b"));
		break;
	case Pieces::ROOK:
		output.append((color == Color::WHITE ? "R" : "r"));
		break;
	case Pieces::QUEEN:
		output.append((color == Color::WHITE ? "Q" : "q"));
		break;
	case Pieces::KING:
		output.append((color == Color::WHITE ? "K" : "k"));
		break;
	default:
		output.append(" ");
		break;
	}
	output.append(" | ");

	return output;
}

Color Piece::GetColor() const
{
	return color; // Returns the color of the piece
}
