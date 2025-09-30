#include "move.hpp"

#include <iostream>


Move::Move(int startSquare, int endSquare, int promotion, bool wasEnPassant, bool wasCastle)
	: startSquare(startSquare), endSquare(endSquare), promotion(promotion),
	wasEnPassant(wasEnPassant), wasCastle(wasCastle)
{}

const char* Move::ToUCIString() const
{
	static char buffer[6]; // Max: e2e4q + null

	buffer[0] = 'a' + (startSquare % 8); // file a-h
	buffer[1] = '8' - (startSquare / 8); // rank 1-8
	buffer[2] = 'a' + (endSquare % 8);   // file a-h
	buffer[3] = '8' - (endSquare / 8);   // rank 1-8

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

bool Move::IsNull() const
{
	return startSquare == 0 && endSquare == 0 &&
		   !wasEnPassant && !wasCastle;
}

bool Move::IsCapture(const Square board[64]) const
{
	bool isCapture = false;
	if (board[endSquare].GetPiece().GetType() != Pieces::NONE) isCapture = true;
	if (wasEnPassant) isCapture = true;
	if (wasCastle) isCapture = false;

	return isCapture;
}

Move Move::FromUCI(const std::string& uci, const Square board[64])
{
	if (uci.size() < 4)
		throw "Invalid UCI move: " + uci;

	Move m;

	// Convert file/rank into col/row
	int startCol = uci[0] - 'a';
	int startRow = '8' - uci[1];
	int endCol = uci[2] - 'a';
	int endRow = '8' - uci[3];

	m.startSquare = startRow * 8 + startCol;
	m.endSquare =	endRow	 * 8 + endCol;

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

	// TODO: Need to add en passant
	// King moved more than 1 space
	if (board[m.startSquare].GetPiece().GetType() == Pieces::KING && std::abs((m.startSquare % 8) - (m.endSquare % 8)) > 1)
		m.wasCastle = true;

	return m;
}