#include "piece.hpp"
#include "square.hpp"

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
bool Piece::ValidMove(int startRow, int startCol, int& endRow, int& endCol, Square board[8][8])
{
	// Need to check if theres pieces in the way for bishop, rook, and queen
	// Return false if conditions aren't met, and break if they are, so only bishop rook and queen get to the end of the function
	// Then have the in the way code in 1 place so it isn't repeated
	switch (type)
	{
	case Pieces::PAWN: // Complete
		// Line 1: Pawn moves forward 1
		// Line 2: Pawn moves forward 2 from starting pos
		// Line 3: Ensures line 1 and 2 only work if square is empty
		// Line 4: Move diagonally to capture a piece
		// Line 5: Ensures line 4 only works if square is occupied
		if (color == Color::WHITE)
		{
			if ((((startCol == endCol && endRow == startRow - 1) ||
				(startCol == endCol && startRow == 6 && endRow == 4))
					&& board[endRow][endCol].IsEmpty()) ||
				((abs(endCol - startCol) == 1 && endRow == startRow - 1)
					&& !board[endRow][endCol].IsEmpty())) return true;
		}
		else // Black
		{
			if ((((startCol == endCol && endRow == startRow + 1) ||
				(startCol == endCol && startRow == 1 && endRow == 3))
					&& board[endRow][endCol].IsEmpty()) ||
				((abs(endCol - startCol) == 1 && endRow == startRow + 1)
					&& !board[endRow][endCol].IsEmpty())) return true;
		}
		break;
	case Pieces::KNIGHT:
		// If row changes by 2 (abs) and column changes by 1 (abs), or the other way around
		if (abs(endRow - startRow) == 2 && abs(endCol - startCol) == 1) return true;
		if (abs(endCol - startCol) == 2 && abs(endRow - startRow) == 1) return true;
		break;
	case Pieces::BISHOP:
		if (abs(endCol - startCol) == abs(endRow - startRow)) goto check_pieces_in_way; // Change in up and across is the same (diagonally)
		break;
	case Pieces::ROOK:
		if (startCol == endCol || startRow == endRow) goto check_pieces_in_way; // Only move up or across
		break;
	case Pieces::QUEEN: // Copy of rook and bishop
		if ((abs(endCol - startCol) == abs(endRow - startRow)) ||
		   (startCol == endCol || startRow == endRow)) goto check_pieces_in_way;
		break;
	case Pieces::KING:
		if (abs(endCol - startCol) <= 1 && abs(endRow - startRow) <= 1) return true;
		break;

	default:
		return false;
	}

	return false;

	// Uses goto. Best way I could think of
	check_pieces_in_way:
	for (int i = 1; i < 8; ++i) // 7 loops because chess board has 8 squares
	{
		int rowChange = (endRow - startRow) / std::max(1, abs(endRow - startRow)); // Normalize row difference to 1 or -1 (up or down)
		int colChange = (endCol - startCol) / std::max(1, abs(endCol - startCol)); // Normalize column difference to 1 or -1 (left or right)
		int currentRow = startRow + i * rowChange; // Starts at startRow and moves towards endRow
		int currentCol = startCol + i * colChange; // Starts at startCol and moves towards endCol

		if (currentRow == endRow && currentCol == endCol) return true; // Reached the end square
		if (!board[currentRow][currentCol].IsEmpty()) // Piece in the way
		{
			// Set endRow and endCol to the current square if a piece of the opponent is found
			// Or the previous square if a piece of the same color is found
			if (board[currentRow][currentCol].GetPiece().GetColor() != color)
			{
				endRow = currentRow;
				endCol = currentCol;
			}
			else
			{
				endRow = currentRow - rowChange; // Set to the previous square
				endCol = currentCol - colChange; // Set to the previous square

				// Check if the previous square is the start square (wasted move)
				if (endRow == startRow && endCol == startCol)
					return false;
			}

			return true;
		}
	}
}
