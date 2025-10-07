#pragma once

#include <functional>
#include <vector>
#include <cstdint>

#include "core/square.hpp"
#include "core/piece.hpp"

#include <SDL/SDL.h>
#undef main

class GraphicsEngine
{
public:
	GraphicsEngine();
	~GraphicsEngine();
	void Render(const Square board[64]);
	void RenderBoard(const Square board[64]);
	void RenderPieces(const Square board[64]);
	void RenderBitboard(uint64_t bitboard); // Blue is 1, red is 0
	int GetInputs(); // Returns {row, col} of clicked square, or {-1, -1} if none
	template <typename F>
	void QueueRender(F&& func);
	void DrawSquareHighlight(int sq, SDL_Color color); // Highlight a square (for showing selected piece, valid moves, etc.)

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
