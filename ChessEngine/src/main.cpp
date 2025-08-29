#include "engine.hpp"
#include "graphics/graphicsEngine.hpp"


// TODO: List of attacked squares (untested)
// TODO: Checks and checkmate
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