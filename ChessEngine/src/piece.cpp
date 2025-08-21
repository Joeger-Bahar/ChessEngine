#include "piece.hpp"

#include <iostream>

void Piece::Render() const
{
	switch (type)
	{
	case Pieces::PAWN:
		std::cout << (color == Color::WHITE ? "P" : "p");
		break;
	case Pieces::KNIGHT:
		std::cout << (color == Color::WHITE ? "N" : "n");
		break;
	case Pieces::BISHOP:
		std::cout << (color == Color::WHITE ? "B" : "b");
		break;
	case Pieces::ROOK:
		std::cout << (color == Color::WHITE ? "R" : "r");
		break;
	case Pieces::QUEEN:
		std::cout << (color == Color::WHITE ? "Q" : "q");
		break;
	case Pieces::KING:
		std::cout << (color == Color::WHITE ? "K" : "k");
		break;
	default:
		std::cout << " ";
		break;
	}
	std::cout << " | ";
}
