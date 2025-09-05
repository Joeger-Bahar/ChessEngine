#include "engine.hpp"
#include "graphics/graphicsEngine.hpp"

// TODO: Promotion GUI
// TODO: Optimization with bitboards
// TODO: Don't clear and refill queued renders every frame
// TODO: Castles aren't recorded as moves
int main()
{
	Engine chessEngine;
	while (1)
	{
		chessEngine.Update();
	}
	return 0;
}