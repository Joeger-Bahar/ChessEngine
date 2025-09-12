#include "move.hpp"

Move::Move()
	: startCol(0), startRow(0), endCol(0), endRow(0), promotion(6), wasEnPassant(false), wasCastle(false)
{
}

const char* Move::ToString() const
{
	static char buffer[6]; // Max length for move notation (e.g., e2e4) + null terminator
	if (wasCastle)
	{
		if (endCol == 6) // Kingside
			return "O-O";
		else // Queenside
			return "O-O-O";
	}
	buffer[0] = 'a' + startCol; // Column a-h
	buffer[1] = '8' - startRow; // Row 1-8
	buffer[2] = 'a' + endCol;   // Column a-h
	buffer[3] = '8' - endRow;   // Row 1-8
	if (promotion != 6) // If there is a promotion
	{
		switch (promotion)
		{
		case static_cast<int>(Pieces::QUEEN):
			buffer[4] = 'q';
			break;
		case static_cast<int>(Pieces::ROOK):
			buffer[4] = 'r';
			break;
		case static_cast<int>(Pieces::BISHOP):
			buffer[4] = 'b';
			break;
		case static_cast<int>(Pieces::KNIGHT):
			buffer[4] = 'n';
			break;
		default:
			buffer[4] = '\0'; // No promotion
			break;
		}
		buffer[5] = '\0'; // Null terminator
	}
	else
	{
		buffer[4] = '\0'; // Null terminator
	}
	return buffer;
}

const char* Move::ToUCIString() const
{
	static char buffer[6]; // Max: e2e4q + null

	buffer[0] = 'a' + startCol; // file a-h
	buffer[1] = '8' - startRow; // rank 1-8
	buffer[2] = 'a' + endCol;   // file a-h
	buffer[3] = '8' - endRow;   // rank 1-8

	if (promotion != 6) // If promotion piece
	{
		switch (promotion)
		{
		case static_cast<int>(Pieces::QUEEN):
			buffer[4] = 'q'; break;
		case static_cast<int>(Pieces::ROOK):
			buffer[4] = 'r'; break;
		case static_cast<int>(Pieces::BISHOP):
			buffer[4] = 'b'; break;
		case static_cast<int>(Pieces::KNIGHT):
			buffer[4] = 'n'; break;
		default:
			buffer[4] = '\0'; // no promotion
			break;
		}
		buffer[5] = '\0'; // null terminator
	}
	else
	{
		buffer[4] = '\0'; // null terminator
	}

	return buffer;
}

bool Move::IsNull() const
{
	return startCol == 0 && startRow == 0 &&
		endCol == 0 && endRow == 0 &&
		!wasEnPassant && !wasCastle;
}
