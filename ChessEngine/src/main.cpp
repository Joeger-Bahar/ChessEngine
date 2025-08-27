#include <iostream>
#include "engine.hpp"

// TODO: List of attacked squares
// TODO: Checks and checkmate
// TODO: En passant
// TODO: GUI
// TODO: Promotion
int main()
{
	Engine chessEngine;
	while (1)
	{
		chessEngine.RunTurn();	// Does everything for 1 turn
	}
	return 0;
}