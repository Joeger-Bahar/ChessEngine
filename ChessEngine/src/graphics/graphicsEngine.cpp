#include "graphicsEngine.hpp"

#include <iostream>

#include <SDL/SDL_image.h>

GraphicsEngine::GraphicsEngine()
{
	Initialize();
}

GraphicsEngine::~GraphicsEngine()
{
	Shutdown();
}

void GraphicsEngine::Render(Square board[8][8], const std::vector<std::pair<int, int>>& highlights)
{
	// Clear screen
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);

	// Draw chess board
	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 8; ++j)
		{
			// Alternate colors
			if ((i + j) % 2 == 0)
				SDL_SetRenderDrawColor(renderer, 240, 217, 181, 255); // Light squares
			else
				SDL_SetRenderDrawColor(renderer, 181, 136, 99, 255); // Dark squares
			SDL_Rect rect = { j * 75, i * 75, 75, 75 }; // Each square is 75x75 pixels
			SDL_RenderFillRect(renderer, &rect);
		}
	}

	// Draw highlights
	for (const auto& [row, col] : highlights)
	{
		DrawSquareHighlight(row, col);
	}

	// Draw pieces
	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 8; ++j)
		{
			Piece piece = board[i][j].GetPiece();
			if (piece == Pieces::NONE) continue; // No piece to draw

			int pieceIndex = -1;
			switch (piece) {
			case Pieces::KING:   pieceIndex = (piece.GetColor() == Color::WHITE) ? 0 : 6; break;
			case Pieces::QUEEN:  pieceIndex = (piece.GetColor() == Color::WHITE) ? 1 : 7; break;
			case Pieces::BISHOP: pieceIndex = (piece.GetColor() == Color::WHITE) ? 2 : 8; break;
			case Pieces::KNIGHT: pieceIndex = (piece.GetColor() == Color::WHITE) ? 3 : 9; break;
			case Pieces::ROOK:   pieceIndex = (piece.GetColor() == Color::WHITE) ? 4 : 10; break;
			case Pieces::PAWN:   pieceIndex = (piece.GetColor() == Color::WHITE) ? 5 : 11; break;
			default: throw "Piece not in list";
			}

			if (pieceIndex != -1)
			{
				SDL_Rect destRect = { j * 75, i * 75, 75, 75 };
				SDL_RenderCopy(renderer, pieceTextures[pieceIndex], NULL, &destRect);
			}
		}
	}
	// Present the rendered frame
	SDL_RenderPresent(renderer);
}

std::pair<int, int> GraphicsEngine::GetClick()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			exit(0); // Exit the program if the window is closed
		}
		else if (event.type == SDL_MOUSEBUTTONDOWN) {
			int x = event.button.x;
			int y = event.button.y;
			int col = x / 75; // Each square is 75 pixels wide
			int row = y / 75; // Each square is 75 pixels tall
			if (col >= 0 && col < 8 && row >= 0 && row < 8) {
				return { row, col };
			}
		}
		else if (event.type == SDL_KEYDOWN) { // Left arrow for undo
			if (event.key.keysym.sym == SDLK_LEFT) {
				return { -2, -2 }; // Special code for undo
			}
		}
	}
	return { -1, -1 }; // No click detected
}

void GraphicsEngine::DrawSquareHighlight(int row, int col)
{
	// Draw an outline around the square
	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green outline
	SDL_Rect rect = { col * 75, row * 75, 75, 75 };
	SDL_RenderFillRect(renderer, &rect);
}

void GraphicsEngine::Initialize()
{
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
		exit(1);
	}

	// Create window
	window = SDL_CreateWindow("Chess Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 600, SDL_WINDOW_SHOWN);
	if (window == nullptr) {
		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}

	// Create renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == nullptr) {
		SDL_DestroyWindow(window);
		std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(1);
	}

	// Load pieces from single piece map (white on top, king, queen, bishops, knights, rooks, pawns left to right, then black)
	SDL_Surface* pieceSurfaces[12];
	SDL_Surface* pieceMap = IMG_Load("res/pieces.png");

	if (!pieceMap) {
		std::cerr << "Failed to load piece map: " << SDL_GetError() << std::endl;
		return;
	}
	// Divide surface into 6 across (each piece takes up equal space), and 2 down (white and black)
	for (int i = 0; i < 12; ++i) {
		int x = (i % 6) * (pieceMap->w / 6);
		int y = (i / 6) * (pieceMap->h / 2);
		SDL_Rect srcRect = { x, y, pieceMap->w / 6, pieceMap->h / 2 };
		pieceSurfaces[i] = SDL_CreateRGBSurface(0, srcRect.w, srcRect.h, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		SDL_BlitSurface(pieceMap, &srcRect, pieceSurfaces[i], NULL);
		pieceTextures[i] = SDL_CreateTextureFromSurface(renderer, pieceSurfaces[i]);
	}

	SDL_FreeSurface(pieceMap);
}

void GraphicsEngine::Shutdown()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
