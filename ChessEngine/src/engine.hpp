#pragma once

#include "piece.hpp"
#include "square.hpp"
#include "move.hpp"

class Engine
{
public:
	Engine();
	~Engine();

	void Render() const;
	bool Move(Square& from, Square& to);

private:
	Square board[8][8];
};