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

Color Piece::GetColor() const
{
	return color; // Returns the color of the piece
}

// TODO: Check for piece in the way of a move
bool Piece::ValidMove(int startRow, int startCol, int endRow, int endCol, Square board[8][8])
{
	switch (type)
	{
	case Pieces::PAWN: // TODO: Need to check if theres a piece before diag move
		// Pawns move forward 1 square, or 2 squares from starting position
		if (color == Color::WHITE)
		{
			if ((startCol == endCol && endRow == startRow - 1) ||					  // Move forward 1
			    (startCol == endCol && startRow == 6 && endRow == 4) ||				  // Move forward 2 from starting position
			    (abs(endCol - startCol) == 1 && endRow == startRow - 1)) return true; // Capture diagonally
		}
		else // Black
		{
			if ((startCol == endCol && endRow == startRow + 1) ||					  // Move forward 1
				(startCol == endCol && startRow == 1 && endRow == 3) ||				  // Move forward 2 from starting position
				(abs(endCol - startCol) == 1 && endRow == startRow + 1)) return true; // Capture diagonally
		}
		break;
	case Pieces::KNIGHT:
		// If row changes by 2 (abs) and column changes by 1 (abs), or the other way around
		if (abs(endRow - startRow) == 2 && abs(endCol - startCol) == 1) return true;
		if (abs(endCol - startCol) == 2 && abs(endRow - startRow) == 1) return true;
		break;
	case Pieces::BISHOP:
		if (abs(endCol - startCol) == abs(endRow - startRow)) return true; // Change in up and across is the same (diagonally)
		break;
	case Pieces::ROOK:
		if (startCol == endCol || startRow == endRow) return true; // Only move up or across
		break;
	case Pieces::QUEEN: // Copy of rook and bishop
		if (abs(endCol - startCol) == abs(endRow - startRow)) return true;
		if (startCol == endCol || startRow == endRow) return true;
		break;
	case Pieces::KING:
		if (abs(endCol - startCol) <= 1 && abs(endRow - startRow) <= 1) return true;
		break;

	default:
		return false;
	}
	return false;
}
