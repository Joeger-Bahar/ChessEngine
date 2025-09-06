#pragma once

#include <functional>
#include <vector>

#include "../square.hpp"
#include "../piece.hpp"

#include <SDL/SDL.h>
#undef main

class GraphicsEngine
{
public:
	GraphicsEngine();
	~GraphicsEngine();
	void Render(Square board[8][8]);
	void RenderBoard(Square board[8][8]);
	void RenderPieces(Square board[8][8]);
	std::pair<int, int> GetInputs(); // Returns {row, col} of clicked square, or {-1, -1} if none
	template <typename F>
	void QueueRender(F&& func);
	void DrawSquareHighlight(int row, int col, SDL_Color color); // Highlight a square (for showing selected piece, valid moves, etc.)

private:
	void Initialize();
	void Shutdown();

	std::vector<std::function<void()>> queuedRenders;

	SDL_Texture* pieceTextures[12];
	SDL_Window* window;
	SDL_Renderer* renderer;
};

template<typename F>
inline void GraphicsEngine::QueueRender(F&& func)
{
	queuedRenders.push_back(std::forward<F>(func));
}
