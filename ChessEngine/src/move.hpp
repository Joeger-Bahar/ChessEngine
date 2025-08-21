#pragma once

#include <cstdint>

#include "piece.hpp"

struct Move
{
	uint8_t fromX, fromY;
	uint8_t toX, toY;
};