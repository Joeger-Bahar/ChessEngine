#include "eval.hpp"

#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <bit>

#include "movegen.hpp"
#include "boardCalculator.hpp"
#include "constants.hpp"
#include "engine.hpp"

#include <intrin.h>
#  define __builtin_popcount __popcnt

// Pawns
int pawnPST[64] = {
	 0,  0,  0,  0,  0,  0,  0,  0,
	50, 50, 50, 50, 50, 50, 50, 50,
	10, 10, 20, 30, 30, 20, 10, 10,
	 5,  5, 10, 25, 25, 10,  5,  5,
	 0,  0,  0, 20, 20,  0,  0,  0,
	 5, -5,-10,  0,  0,-10, -5,  5,
	 5, 10, 10,-20,-20, 10, 10,  5,
	 0,  0,  0,  0,  0,  0,  0,  0
};

// King, middle game
int kingPST_mg[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
	20, 20,  0,  0,  0,  0, 20, 20,
	20, 30, 10,  0,  0, 10, 30, 20
};

// King, endgame
int kingPST_eg[64] = {
   -50,-40,-30,-20,-20,-30,-40,-50,
   -30,-20,-10,  0,  0,-10,-20,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-30,  0,  0,  0,  0,-30,-30,
   -50,-30,-30,-30,-30,-30,-30,-50
};

// Knight PST
int knightPST[64] = {
   -50,-40,-30,-30,-30,-30,-40,-50,
   -40,-20,  0,  0,  0,  0,-20,-40,
   -30,  0, 10, 15, 15, 10,  0,-30,
   -30,  5, 15, 20, 20, 15,  5,-30,
   -30,  0, 15, 20, 20, 15,  0,-30,
   -30,  5, 10, 15, 15, 10,  5,-30,
   -40,-20,  0,  5,  5,  0,-20,-40,
   -50,-40,-30,-30,-30,-30,-40,-50
};

// Bishop PST
int bishopPST[64] = {
   -20,-10,-10,-10,-10,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5, 10, 10,  5,  0,-10,
   -10,  5,  5, 10, 10,  5,  5,-10,
   -10,  0, 10, 10, 10, 10,  0,-10,
   -10, 10, 10, 10, 10, 10, 10,-10,
   -10,  5,  0,  0,  0,  0,  5,-10,
   -20,-10,-10,-10,-10,-10,-10,-20
};

// Rook PST
int rookPST[64] = {
	 0,  0,  0,  5,  5,  0,  0,  0,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	 5, 10, 10, 10, 10, 10, 10,  5,
	 0,  0,  0,  0,  0,  0,  0,  0
};

// Queen PST
int queenPST[64] = {
   -20,-10,-10, -5, -5,-10,-10,-20,
   -10,  0,  0,  0,  0,  0,  0,-10,
   -10,  0,  5,  5,  5,  5,  0,-10,
	-5,  0,  5,  5,  5,  5,  0, -5,
	 0,  0,  5,  5,  5,  5,  0, -5,
   -10,  5,  5,  5,  5,  5,  0,-10,
   -10,  0,  5,  0,  0,  0,  0,-10,
   -20,-10,-10, -5, -5,-10,-10,-20
};

int Mirror(int sq)
{
	int row = ToRow(sq);
	int col = ToCol(sq);
	int mirroredRow = 7 - row;
	return ToIndex(mirroredRow, col);
}

const Bitboard FILE_MASK[8] = {
	0x0101010101010101ULL, // File A
	0x0202020202020202ULL, // File B
	0x0404040404040404ULL, // File C
	0x0808080808080808ULL, // File D
	0x1010101010101010ULL, // File E
	0x2020202020202020ULL, // File F
	0x4040404040404040ULL, // File G
	0x8080808080808080ULL  // File H
};

// Rank 1 = bottom (a1–h1), Rank 8 = top (a8–h8)
// Bit 0 = a8, Bit 63 = h1
const Bitboard RANK_MASK[8] = {
	0xFF00000000000000ULL, // Rank 8
	0x00FF000000000000ULL, // Rank 7
	0x0000FF0000000000ULL, // Rank 6
	0x000000FF00000000ULL, // Rank 5
	0x00000000FF000000ULL, // Rank 4
	0x0000000000FF0000ULL, // Rank 3
	0x000000000000FF00ULL, // Rank 2
	0x00000000000000FFULL  // Rank 1
};

int Eval(Color player, const Engine* engine)
{
	const BitboardBoard& board = engine->GetBitboardBoard();

	int kingSafetyScore = 0;
	int pieceActivityScore = 0;
	int materialScore = 0;

	const int pieceValues[] = { 0, 100, 320, 330, 500, 900, 100000 };

	Bitboard whiteOcc = board.allPieces[0];
	Bitboard blackOcc = board.allPieces[1];
	Bitboard allOcc = whiteOcc | blackOcc;

	// Pawn structure masks
	const int doubledPenalty = 15;
	const int isolatedPenalty = 20;
	const int passedBonus = 20;

	// Game phase from remaining material (24 = all pieces, 0 = bare kings), used to blend the king
	// tables. GameState::endgame can't be used: it is never updated in UCI mode and never reset.
	const int phaseWeights[6] = { 0, 1, 1, 2, 4, 0 };
	int phase = 0;
	for (int c = 0; c < 2; ++c)
		for (int t = 0; t < 6; ++t)
			phase += std::popcount(board.pieceBitboards[c][t]) * phaseWeights[t];
	phase = std::min(phase, 24);

	// TODO: Make this more cache friendly, like sq on the inside loop
	for (int sq = 0; sq < 64; ++sq)
	{
		for (int color = 0; color < 2; ++color)
		{
			for (int t = 0; t < 6; ++t)
			{
				if (IsSet(board.pieceBitboards[color][t], sq))
				{
					Pieces type = static_cast<Pieces>(t + 1);
					bool isWhite = (color == 0);
					int value = pieceValues[t + 1];
					int mirroredSq = Mirror(sq);

					if (isWhite)
						materialScore += value;
					else
						materialScore -= value;

					switch (type)
					{
					case Pieces::PAWN:
					{
						if (isWhite) pieceActivityScore += pawnPST[sq];
						else         pieceActivityScore -= pawnPST[mirroredSq];

						int col = ToCol(sq);
						int row = ToRow(sq);
						Bitboard fileMask = FILE_MASK[col];
						Bitboard leftMask = (col > 0) ? FILE_MASK[col - 1] : 0ULL;
						Bitboard rightMask = (col < 7) ? FILE_MASK[col + 1] : 0ULL;

						Bitboard sameFile = board.pieceBitboards[color][(int)Pieces::PAWN - 1] & fileMask;
						Bitboard adjFiles = board.pieceBitboards[color][(int)Pieces::PAWN - 1] & (leftMask | rightMask);
						Bitboard enemyPawns = board.pieceBitboards[1 - color][(int)Pieces::PAWN - 1];

						// Doubled pawns
						if (std::popcount(sameFile) > 1) // Must count all 64 bits: __popcnt only sees ranks 8-5
						{
							pieceActivityScore += isWhite ? -doubledPenalty : doubledPenalty;
						}

						// Isolated pawns
						if (adjFiles == 0ULL)
						{
							pieceActivityScore += isWhite ? -isolatedPenalty : isolatedPenalty;
						}

						// Passed pawns
						Bitboard blockingPawns = enemyPawns & (fileMask | leftMask | rightMask);
						Bitboard inFrontEnemyPawns = 0ULL;
						// Every square ahead on this file and both neighbouring files (it used to be this file only)
						if (isWhite)
						{
							for (int r = row - 1; r >= 0; --r)
								inFrontEnemyPawns |= (0xFFULL << (8 * r));
						}
						else
						{
							for (int r = row + 1; r < 8; ++r)
								inFrontEnemyPawns |= (0xFFULL << (8 * r));
						}
						if ((blockingPawns & inFrontEnemyPawns) == 0ULL)
						{
							pieceActivityScore += isWhite ? passedBonus : -(passedBonus);
						}

						break;
					}
					case Pieces::KNIGHT:
					{
						if (isWhite) pieceActivityScore += knightPST[sq];
						else         pieceActivityScore -= knightPST[Mirror(sq)];
						break;
					}
					case Pieces::BISHOP:
					{
						if (isWhite) pieceActivityScore += bishopPST[sq];
						else         pieceActivityScore -= bishopPST[Mirror(sq)];

						// +30 if both colors of bishop are present
						Bitboard bishops = board.pieceBitboards[color][(int)Pieces::BISHOP - 1];
						Bitboard lightSquares = 0x55AA55AA55AA55AAULL; // Light square mask
						Bitboard darkSquares = 0xAA55AA55AA55AA55ULL;  // Dark square mask
						if ((bishops & lightSquares) && (bishops & darkSquares))
						{
							pieceActivityScore += isWhite ? 30 : -30;
						}
						break;
					}
					case Pieces::ROOK:
					{
						if (isWhite) pieceActivityScore += rookPST[sq];
						else         pieceActivityScore -= rookPST[Mirror(sq)];

						// +25 if on open file, +15 if on semi-open file
						int col = ToCol(sq);
						Bitboard fileMask = FILE_MASK[col];
						Bitboard friendlyPawns = board.pieceBitboards[color][(int)Pieces::PAWN - 1];
						Bitboard enemyPawns = board.pieceBitboards[1 - color][(int)Pieces::PAWN - 1];
						if ((friendlyPawns & fileMask) == 0ULL && (enemyPawns & fileMask) == 0ULL)
						{
							pieceActivityScore += isWhite ? 25 : -25;
						}
						else if ((friendlyPawns & fileMask) == 0ULL)
						{
							pieceActivityScore += isWhite ? 15 : -15;
						}

						// +25 if on same file as other rook
						Bitboard rooks = board.pieceBitboards[color][(int)Pieces::ROOK - 1];
						if (std::popcount(rooks & fileMask) > 1)
						{
							pieceActivityScore += isWhite ? 25 : -25;
						}

						// +30 if on 7th rank
						int row = ToRow(sq);
						if ((isWhite && row == 1) || (!isWhite && row == 6))
						{
							pieceActivityScore += isWhite ? 30 : -30;
						}
						break;
					}
					case Pieces::QUEEN:
					{
						if (isWhite) pieceActivityScore += queenPST[sq];
						else         pieceActivityScore -= queenPST[Mirror(sq)];
						break;
					}
					case Pieces::KING:
					{
						int kingSq = isWhite ? sq : Mirror(sq);
						int kingScore = (kingPST_mg[kingSq] * phase + kingPST_eg[kingSq] * (24 - phase)) / 24;
						pieceActivityScore += isWhite ? kingScore : -kingScore;
						break;
					}
					}
				}
			}
		}
	}

	float score = ((float)materialScore * engine->materialWeight) + ((float)pieceActivityScore * engine->pieceActivityWeight);

	if (player == Color::BLACK) score *= -1;

	return (int)score;
}
