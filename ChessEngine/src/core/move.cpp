#include "move.hpp"

#include <iostream>

#include "boardCalculator.hpp"

const char* MoveToUCI(Move m)
{
	static char buffer[6]; // Max: e2e4q + null

	int start = GetStart(m);
	int end = GetEnd(m);
	int promo = GetPromotion(m);

	buffer[0] = 'a' + (start % 8); // file a-h
	buffer[1] = '8' - (start / 8); // rank 1-8
	buffer[2] = 'a' + (end % 8);   // file a-h
	buffer[3] = '8' - (end / 8);   // rank 1-8
	
	if (promo != static_cast<int>(Pieces::NONE)) // If promotion piece
	{
		switch (promo)
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

bool MoveIsNull(Move m)
{
	return GetStart(m) == 0 && GetEnd(m) == 0 &&
		!IsEnPassant(m) && !IsCastle(m);
}

bool MoveIsCapture(Move m, const BitboardBoard& board)
{
	int endSq = GetEnd(m);

	if (IsCastle(m)) return false;
	if (!BoardCalculator::IsEmptyAt(endSq, board)) return true;
	return IsEnPassant(m);
}

Move MoveFromUCI(const std::string& uci, const Square board[64])
{
	if (uci.size() < 4)
		throw "Invalid UCI move: " + uci;

	// Convert file/rank into col/row
	int startCol = uci[0] - 'a';
	int startRow = '8' - uci[1];
	int endCol = uci[2] - 'a';
	int endRow = '8' - uci[3];

	int startSquare = startRow * 8 + startCol;
	int endSquare   = endRow * 8 + endCol;

	int promotion = static_cast<int>(Pieces::NONE); // or however you store it
	if (uci.size() == 5)
	{
		char promoChar = uci[4];
		switch (promoChar)
		{
		case 'q': promotion = (int)Pieces::QUEEN; break;
		case 'r': promotion = (int)Pieces::ROOK;  break;
		case 'b': promotion = (int)Pieces::BISHOP; break;
		case 'n': promotion = (int)Pieces::KNIGHT; break;
		default: break; // Invalid char, ignore
		}
	}

	// King moved more than 1 space
	bool castle = false;
	if (board[startSquare].GetPiece().GetType() == Pieces::KING && std::abs((startSquare % 8) - (endSquare % 8)) > 1)
		castle = true;

	return EncodeMove(startSquare, endSquare, promotion, false, castle);
}