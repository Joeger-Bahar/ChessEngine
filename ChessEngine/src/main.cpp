#include "engine.hpp"
#include "graphics/graphicsEngine.hpp"

// TODO: Promotion GUI
// TODO: Optimization with bitboards
// TODO: Don't clear and refill queued renders every frame
// TODO: Castles aren't recorded as moves

// Can't undo move but set state
// Can't set moves instead of undoing because you need the rest of the board because you aren't only doing the changes (git mentality)
int main()
{
	Engine chessEngine;
	while (1)
	{
		chessEngine.Update();
	}
	return 0;
}