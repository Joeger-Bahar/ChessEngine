#pragma once

#include <vector>

#include "piece.hpp"
#include "move.hpp"
#include "bitboard.hpp"

class Movegen
{
public:
	static bool IsSquareAttacked(int sq, Color byColor, const BitboardBoard& board);

	// This gets moves for a piece, so it checks for checks as well
	static std::vector<uint8_t> GetValidMoves(int sq, const BitboardBoard& board);

	static std::vector<Move> GetAllLegalMoves(Color color, const BitboardBoard& board, class Engine* engine);
	// This is pseudo-legal moves, it does not check for checks for faster engine calculations
	static void GetAllMoves(std::vector<Move>& moves, Color color, const BitboardBoard& board, Engine* engine, bool onlyNoisy = false);
	static std::vector<Move> GetAllMoves(Color color, const BitboardBoard& board, Engine* engine, bool onlyCaptures = false);
	static Bitboard GetPseudoAttacks(Pieces piece, int sq, const Bitboard& allOcc, bool isWhite);

	static void InitPrecomputedAttacks();
	static void BuildMagicAttackTables();
	//static void GenerateAndInitMagics(bool dumpToHeader, const std::string& outPath);

	// TODO: Creating a definition in the cpp file breaks the ide lmao
	static const Bitboard(&GetPawnAttacks())[2][64];
	static const Bitboard(&GetKnightAttacks())[64];
	static const Bitboard(&GetKingAttacks())[64];
	static const Bitboard(&GetRookAttacks())[64][4096];
	static const Bitboard(&GetBishopAttacks())[64][512];

private:
	static Bitboard KingMoves(int sq, Color color, const BitboardBoard& board);
	static Bitboard PawnMoves(int sq, Color color, const BitboardBoard& board);
	static Bitboard KnightMoves(int sq, Color color, const BitboardBoard& board);
	static Bitboard SlidingMoves(int sq, Color color, const Pieces piece, const BitboardBoard& board, bool includeBlockers = false);

	// Direction and offset tables
	static const int knightOffsets[8];
	static const int kingOffsets[8];
	static const int bishopDirs[4];
	static const int rookDirs[4];
	static const int queenDirs[8];
};