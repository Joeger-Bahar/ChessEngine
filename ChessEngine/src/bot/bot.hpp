#pragma once

#include "core/piece.hpp"
#include "core/boardCalculator.hpp"
#include "core/engine.hpp"
#include "graphics/graphicsEngine.hpp"

class Bot
{
public:
	Bot(Engine* engine, Color color);
	Move GetMove();
	const Color GetColor() const { return botColor; }

private:
	Engine* engine;
	Color botColor;
};
