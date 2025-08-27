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

//bool Piece::ValidMove(Move& move, Square board[8][8])
//{
//	// Need to check if theres pieces in the way for bishop, rook, and queen
//	// Return false if conditions aren't met, and break if they are, so only bishop rook and queen get to the end of the function
//	// Then have the in the way code in 1 place so it isn't repeated
//	switch (type)
//	{
//	case Pieces::PAWN: // Complete
//		// Line 1: Pawn moves forward 1
//		// Line 2: Pawn moves forward 2 from starting pos
//		// Line 3: Ensures line 1 and 2 only work if square is empty
//		// Line 4: Move diagonally to capture a piece
//		// Line 5: Ensures line 4 only works if square is occupied
//		if (color == Color::WHITE)
//		{
//			if ((((move.startCol == move.endCol && move.endRow == move.startRow - 1) ||
//				(move.startCol == move.endCol && move.startRow == 6 && move.endRow == 4))
//					&& board[move.endRow][move.endCol].IsEmpty()) ||
//				((abs(move.endCol - move.startCol) == 1 && move.endRow == move.startRow - 1)
//					&& !board[move.endRow][move.endCol].IsEmpty())) return true;
//		}
//		else // Black
//		{
//			if ((((move.startCol == move.endCol && move.endRow == move.startRow + 1) ||
//				(move.startCol == move.endCol && move.startRow == 1 && move.endRow == 3))
//					&& board[move.endRow][move.endCol].IsEmpty()) ||
//				((abs(move.endCol - move.startCol) == 1 && move.endRow == move.startRow + 1)
//					&& !board[move.endRow][move.endCol].IsEmpty())) return true;
//		}
//		break;
//	case Pieces::KNIGHT:
//		// If row changes by 2 (abs) and column changes by 1 (abs), or the other way around
//		if (abs(move.endRow - move.startRow) == 2 && abs(move.endCol - move.startCol) == 1) return true;
//		if (abs(move.endCol - move.startCol) == 2 && abs(move.endRow - move.startRow) == 1) return true;
//		break;
//	case Pieces::BISHOP:
//		if (abs(move.endCol - move.startCol) == abs(move.endRow - move.startRow)) goto check_pieces_in_way; // Change in up and across is the same (diagonally)
//		break;
//	case Pieces::ROOK:
//		if (move.startCol == move.endCol || move.startRow == move.endRow) goto check_pieces_in_way; // Only move up or across
//		break;
//	case Pieces::QUEEN: // Copy of rook and bishop
//		if ((abs(move.endCol - move.startCol) == abs(move.endRow - move.startRow)) ||
//		   (move.startCol == move.endCol || move.startRow == move.endRow)) goto check_pieces_in_way;
//		break;
//	case Pieces::KING:
//		if (abs(move.endCol - move.startCol) <= 1 && abs(move.endRow - move.startRow) <= 1) return true;
//		break;
//
//	default:
//		return false;
//	}
//
//	return false;
//
//	// Uses goto. Best way I could think of
//	check_pieces_in_way:
//	for (int i = 1; i < 8; ++i) // 7 loops because chess board has 8 squares
//	{
//		int rowChange = (move.endRow - move.startRow) / std::max(1, abs(move.endRow - move.startRow)); // Normalize row difference to 1 or -1 (up or down)
//		int colChange = (move.endCol - move.startCol) / std::max(1, abs(move.endCol - move.startCol)); // Normalize column difference to 1 or -1 (left or right)
//		int currentRow = move.startRow + i * rowChange; // Starts at move.startRow and moves towards move.endRow
//		int currentCol = move.startCol + i * colChange; // Starts at move.startCol and moves towards move.endCol
//
//		if (currentRow == move.endRow && currentCol == move.endCol) return true; // Reached the end square
//		if (!board[currentRow][currentCol].IsEmpty()) // Piece in the way
//		{
//			// Set move.endRow and move.endCol to the current square if a piece of the opponent is found
//			// Or the previous square if a piece of the same color is found
//			if (board[currentRow][currentCol].GetPiece().GetColor() != color)
//			{
//				move.endRow = currentRow;
//				move.endCol = currentCol;
//			}
//			else
//			{
//				move.endRow = currentRow - rowChange; // Set to the previous square
//				move.endCol = currentCol - colChange; // Set to the previous square
//
//				// Check if the previous square is the start square (wasted move)
//				if (move.endRow == move.startRow && move.endCol == move.startCol)
//					return false;
//			}
//
//			return true;
//		}
//	}
//}
