#include "move.hpp"

Move::Move()
	: startCol(0), startRow(0), endCol(0), endRow(0), promotion(6), wasEnPassant(false), wasCastle(false)
{
}
