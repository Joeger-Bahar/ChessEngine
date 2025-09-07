#include "core/engine.hpp"
#include "graphics/graphicsEngine.hpp"

// TODO: Promotion GUI
// TODO: Optimization with bitboards
// TODO: Don't clear and refill queued renders every frame

int main()
{
	Engine chessEngine;
	while (1)
	{
		chessEngine.Update();
	}
	return 0;
}