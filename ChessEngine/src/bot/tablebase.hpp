#pragma once

#include <string>

#include "core/engine.hpp"

class Tablebase
{
public:
	Tablebase(const std::string& path);
	~Tablebase();

	// Returns true if prober initialized and files present
	bool Initialized() const;

	// If the position can be probed (<= 7 pieces)
	bool Probeable(const BitboardBoard& board) const;

	// Returns +1 for win for stm, 0 draw, -1 loss, -99 on error
	int ProbeWDL(Engine* engine) const;

	// Returns distance to zeroing (>= 1) or -99 on unavailable
	int ProbeDTZ(Engine* engine) const;

	// Get's best move
	Move GetMove(Engine* engine) const;

private:
	// Uses Fathom
	// https://github.com/jdart1/Fathom.git
	bool ok;
};