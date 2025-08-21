#include <iostream>
#include "engine.hpp"

int main()
{
	Engine chessEngine;
	while (1)
	{
		chessEngine.RunTurn();	// Does everything for 1 turn
	}
	return 0;
}