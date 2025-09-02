#pragma once

#include "../square.hpp"
#include "../piece.hpp"

#include <SDL/SDL.h>
#undef main

class GraphicsEngine
{
public:
	GraphicsEngine();
	~GraphicsEngine();
	void Render(Square board[8][8], const std::vector<std::pair<int, int>>& highlights = {});
	std::pair<int, int> GetClick(); // Returns {row, col} of clicked square, or {-1, -1} if none
	void DrawSquareHighlight(int row, int col); // Highlight a square (for showing selected piece, valid moves, etc.)

private:
	void Initialize();
	void Shutdown();

	SDL_Texture* pieceTextures[12];
	SDL_Window* window;
	SDL_Renderer* renderer;
};