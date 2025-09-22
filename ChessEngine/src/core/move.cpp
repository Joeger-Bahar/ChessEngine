#include "move.hpp"

#include <iostream>


Move::Move(int startCol, int startRow, int endCol, int endRow, int promotion, bool wasEnPassant, bool wasCastle)
	: startCol(startCol), startRow(startRow), endCol(endCol), endRow(endRow), promotion(promotion),
	wasEnPassant(wasEnPassant), wasCastle(wasCastle)
{}

const char* Move::ToString() const
{
	static char buffer[6]; // Max length for move notation (e.g., e2e4q) + null terminator
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

bool Move::IsCapture(const Square board[8][8]) const
{
	bool isCapture = false;
	if (board[endRow][endCol].GetPiece().GetType() != Pieces::NONE) isCapture = true;
	if (wasEnPassant) isCapture = true;
	if (promotion == 6) isCapture = false;
	if (wasCastle) isCapture = false;

	return isCapture;
}

Move Move::FromUCI(const std::string& uci, const Square board[8][8])
{
	if (uci.size() < 4)
		throw "Invalid UCI move: " + uci;

	Move m;

	// Convert file/rank into col/row
	m.startCol = uci[0] - 'a';
	m.startRow = '8' - uci[1];
	m.endCol = uci[2] - 'a';
	m.endRow = '8' - uci[3];

	m.promotion = static_cast<int>(Pieces::NONE); // or however you store it
	if (uci.size() == 5)
	{
		char promoChar = uci[4];
		switch (promoChar)
		{
		case 'q': m.promotion = (int)Pieces::QUEEN; break;
		case 'r': m.promotion = (int)Pieces::ROOK;  break;
		case 'b': m.promotion = (int)Pieces::BISHOP; break;
		case 'n': m.promotion = (int)Pieces::KNIGHT; break;
		default: break; // Invalid char, ignore
		}
	}

	// King moved more than 1 space
	if (board[m.startRow][m.startCol].GetPiece().GetType() == Pieces::KING && std::abs(m.startCol - m.endCol) > 1) 
		m.wasCastle = true;

	return m;
}