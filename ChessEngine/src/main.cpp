#include "engine.hpp"
#include "graphics/graphicsEngine.hpp"

// TODO: Checkmate
// TODO: Promotion
// TODO: Optimization with bitboards
// TODO: Optimization with not checking check twice (boardCalculator.cpp and engine.cpp)
// TODO: Pawns don't trigger checks so you can capture king with pawn
int main()
{
	Engine chessEngine;
	while (1)
	{
		chessEngine.Update();
	}
	return 0;
}