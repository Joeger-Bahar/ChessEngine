#include <iostream>
#include "engine.hpp"

int main()
{
	Engine chessEngine;
	while (1)
	{
		do
		{
			chessEngine.Render();				// Clears screen as well
			chessEngine.StoreMove();			// Stores internally
		} while (!chessEngine.ProcessMove());	// Uses stored move. Returns true if move was valid

		chessEngine.ChangePlayers();
	}
	return 0;
}