#pragma once

#include "engine.hpp"
#include "gamestate.hpp"
#include "square.hpp"

extern int pawnPST_white[64];
extern int pawnPST_black[64];
extern int kingPST_mg_white[64];
extern int kingPST_mg_black[64];
extern int kingPST_eg_white[64];
extern int kingPST_eg_black[64];
extern int knightPST[64];
extern int bishopPST[64];
extern int rookPST[64];
extern int queenPST[64];

int Eval(Color player, const Engine* engine);