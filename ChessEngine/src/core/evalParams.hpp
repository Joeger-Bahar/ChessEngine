#pragma once
// All the evaluation weights
// S(middlegame, endgame), in centipawns.

// Pawn, knight, bishop, rook, queen
constexpr S PieceValue[5] = {
	S(100, 100), S(320, 320), S(330, 330), S(500, 500), S(900, 900),
};

// Early and endgame. Raw values so they can be pasted/tweaked easily.
constexpr int RawPSTMid[6][64] =
{
	// Pawn
	{
		  0,   0,   0,   0,   0,   0,   0,   0,
		 50,  50,  50,  50,  50,  50,  50,  50,
		 10,  10,  20,  30,  30,  20,  10,  10,
		  5,   5,  10,  25,  25,  10,   5,   5,
		  0,   0,   0,  20,  20,   0,   0,   0,
		  5,  -5, -10,   0,   0, -10,  -5,   5,
		  5,  10,  10, -20, -20,  10,  10,   5,
		  0,   0,   0,   0,   0,   0,   0,   0,
	},
	// Knight
	{
		-50, -40, -30, -30, -30, -30, -40, -50,
		-40, -20,   0,   0,   0,   0, -20, -40,
		-30,   0,  10,  15,  15,  10,   0, -30,
		-30,   5,  15,  20,  20,  15,   5, -30,
		-30,   0,  15,  20,  20,  15,   0, -30,
		-30,   5,  10,  15,  15,  10,   5, -30,
		-40, -20,   0,   5,   5,   0, -20, -40,
		-50, -40, -30, -30, -30, -30, -40, -50,
	},
	// Bishop
	{
		-20, -10, -10, -10, -10, -10, -10, -20,
		-10,   0,   0,   0,   0,   0,   0, -10,
		-10,   0,   5,  10,  10,   5,   0, -10,
		-10,   5,   5,  10,  10,   5,   5, -10,
		-10,   0,  10,  10,  10,  10,   0, -10,
		-10,  10,  10,  10,  10,  10,  10, -10,
		-10,   5,   0,   0,   0,   0,   5, -10,
		-20, -10, -10, -10, -10, -10, -10, -20,
	},
	// Rook
	{
		  0,   0,   0,   0,   0,   0,   0,   0,
		  5,  10,  10,  10,  10,  10,  10,   5,
		 -5,   0,   0,   0,   0,   0,   0,  -5,
		 -5,   0,   0,   0,   0,   0,   0,  -5,
		 -5,   0,   0,   0,   0,   0,   0,  -5,
		 -5,   0,   0,   0,   0,   0,   0,  -5,
		 -5,   0,   0,   0,   0,   0,   0,  -5,
		  0,   0,   0,   5,   5,   0,   0,   0,
	},
	// Queen
	{
		-20, -10, -10,  -5,  -5, -10, -10, -20,
		-10,   0,   0,   0,   0,   0,   0, -10,
		-10,   0,   5,   5,   5,   5,   0, -10,
		 -5,   0,   5,   5,   5,   5,   0,  -5,
		  0,   0,   5,   5,   5,   5,   0,  -5,
		-10,   5,   5,   5,   5,   5,   0, -10,
		-10,   0,   5,   0,   0,   0,   0, -10,
		-20, -10, -10,  -5,  -5, -10, -10, -20,
	},
	// King
	{
		-30, -40, -40, -50, -50, -40, -40, -30,
		-30, -40, -40, -50, -50, -40, -40, -30,
		-30, -40, -40, -50, -50, -40, -40, -30,
		-30, -40, -40, -50, -50, -40, -40, -30,
		-20, -30, -30, -40, -40, -30, -30, -20,
		-10, -20, -20, -20, -20, -20, -20, -10,
		 20,  20,   0,   0,   0,   0,  20,  20,
		 20,  30,  10,   0,   0,  10,  30,  20,
	},
};
constexpr int RawPSTEnd[6][64] =
{
	// Pawn
	{
		  0,   0,   0,   0,   0,   0,   0,   0,
		 50,  50,  50,  50,  50,  50,  50,  50,
		 30,  30,  30,  30,  30,  30,  30,  30,
		 15,  15,  15,  15,  15,  15,  15,  15,
		  5,   5,   5,   5,   5,   5,   5,   5,
		  0,   0,   0,   0,   0,   0,   0,   0,
		  0,   0,   0,   0,   0,   0,   0,   0,
		  0,   0,   0,   0,   0,   0,   0,   0,
	},
	// Knight
	{
		-50, -40, -30, -30, -30, -30, -40, -50,
		-40, -20,   0,   0,   0,   0, -20, -40,
		-30,   0,  10,  15,  15,  10,   0, -30,
		-30,   5,  15,  20,  20,  15,   5, -30,
		-30,   0,  15,  20,  20,  15,   0, -30,
		-30,   5,  10,  15,  15,  10,   5, -30,
		-40, -20,   0,   5,   5,   0, -20, -40,
		-50, -40, -30, -30, -30, -30, -40, -50,
	},
	// Bishop
	{
		-20, -10, -10, -10, -10, -10, -10, -20,
		-10,   0,   0,   0,   0,   0,   0, -10,
		-10,   0,   5,  10,  10,   5,   0, -10,
		-10,   5,   5,  10,  10,   5,   5, -10,
		-10,   0,  10,  10,  10,  10,   0, -10,
		-10,  10,  10,  10,  10,  10,  10, -10,
		-10,   5,   0,   0,   0,   0,   5, -10,
		-20, -10, -10, -10, -10, -10, -10, -20,
	},
	// Rook
	{
		  0,   0,   0,   0,   0,   0,   0,   0,
		 10,  10,  10,  10,  10,  10,  10,  10,
		  0,   0,   0,   0,   0,   0,   0,   0,
		  0,   0,   0,   0,   0,   0,   0,   0,
		  0,   0,   0,   0,   0,   0,   0,   0,
		  0,   0,   0,   0,   0,   0,   0,   0,
		  0,   0,   0,   0,   0,   0,   0,   0,
		  0,   0,   0,   0,   0,   0,   0,   0,
	},
	// Queen
	{
		-20, -10, -10,  -5,  -5, -10, -10, -20,
		-10,   0,   0,   0,   0,   0,   0, -10,
		-10,   0,   5,   5,   5,   5,   0, -10,
		 -5,   0,   5,   5,   5,   5,   0,  -5,
		  0,   0,   5,   5,   5,   5,   0,  -5,
		-10,   5,   5,   5,   5,   5,   0, -10,
		-10,   0,   5,   0,   0,   0,   0, -10,
		-20, -10, -10,  -5,  -5, -10, -10, -20,
	},
	// King
	{
		-50, -40, -30, -20, -20, -30, -40, -50,
		-30, -20, -10,   0,   0, -10, -20, -30,
		-30, -10,  20,  30,  30,  20, -10, -30,
		-30, -10,  30,  40,  40,  30, -10, -30,
		-30, -10,  30,  40,  40,  30, -10, -30,
		-30, -10,  20,  30,  30,  20, -10, -30,
		-30, -30,   0,   0,   0,   0, -30, -30,
		-50, -30, -30, -30, -30, -30, -30, -50,
	},
};

// Pawn, knight, bishop, rook, queen, king. a8 first, White's side
// Fill in PST using the hardcoded values
constexpr std::array<std::array<S, 64>, 6> MakePST()
{
	std::array<std::array<S, 64>, 6> pst{};
	for (int piece = 0; piece < 6; ++piece)
		for (int sq = 0; sq < 64; ++sq)
			pst[piece][sq] = S(RawPSTMid[piece][sq], RawPSTEnd[piece][sq]);
	return pst;
}
constexpr auto PST = MakePST();

// By number of useful squares: 4 per square, 0 at 4 squares
constexpr S MobilityKnight[9] = {
	S(-16, -16), S(-12, -12), S(-8, -8), S(-4, -4), S(0, 0), S(4, 4), S(8, 8), S(12, 12),
	S(16, 16),
};

// 5 per square, 0 at 6 squares
constexpr S MobilityBishop[14] = {
	S(-30, -30), S(-25, -25), S(-20, -20), S(-15, -15), S(-10, -10), S(-5, -5), S(0, 0), S(5, 5),
	S(10, 10), S(15, 15), S(20, 20), S(25, 25), S(30, 30), S(35, 35),
};

// 2 per square (4 in the endgame), 0 at 7 squares
constexpr S MobilityRook[15] = {
	S(-14, -28), S(-12, -24), S(-10, -20), S(-8, -16), S(-6, -12), S(-4, -8), S(-2, -4), S(0, 0),
	S(2, 4), S(4, 8), S(6, 12), S(8, 16), S(10, 20), S(12, 24), S(14, 28),
};

// 1 per square (2 in the endgame), 0 at 13 squares
constexpr S MobilityQueen[28] = {
	S(-13, -26), S(-12, -24), S(-11, -22), S(-10, -20), S(-9, -18), S(-8, -16), S(-7, -14), S(-6, -12),
	S(-5, -10), S(-4, -8), S(-3, -6), S(-2, -4), S(-1, -2), S(0, 0), S(1, 2), S(2, 4),
	S(3, 6), S(4, 8), S(5, 10), S(6, 12), S(7, 14), S(8, 16), S(9, 18), S(10, 20),
	S(11, 22), S(12, 24), S(13, 26), S(14, 28),
};

// Two bishops, more in the endgame
constexpr S BishopPair = S(30, 50);

// Per knight per own pawn: knights like closed positions
constexpr S KnightPawns = S(2, 2);

// Per rook per own pawn: rooks like open positions
constexpr S RookPawns = S(-2, -2);

// No pawn of ours on a neighbouring file
constexpr S Isolated = S(-10, -15);

// Another of our pawns in front of it
constexpr S Doubled = S(-10, -20);

// Can't be supported and can't safely advance
constexpr S Backward = S(-8, -10);

// Isolated or backward on a half-open file
constexpr S WeakUnopposed = S(-5, -10);

// By rank (index = rank - 1 from our side): stuck far up the board, it can't support the attack
constexpr S BlockedPawn[8] = {
	S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(-5, -5), S(-5, -5), S(0, 0), S(0, 0),
};

// A pawn of ours beside it, by rank
constexpr S Phalanx[8] = {
	S(0, 0), S(2, 1), S(4, 3), S(8, 6), S(15, 12), S(25, 20), S(40, 30), S(0, 0),
};

// Defended by a pawn of ours, by rank
constexpr S Supported[8] = {
	S(0, 0), S(0, 0), S(5, 3), S(7, 5), S(10, 8), S(15, 12), S(25, 20), S(0, 0),
};

// Passed pawn, by rank
constexpr S PassedRank[8] = {
	S(0, 0), S(5, 10), S(5, 15), S(10, 25), S(20, 45), S(40, 80), S(70, 130), S(0, 0),
};

// Defended by a pawn, no enemy pawn can chase it
constexpr S KnightOutpost = S(25, 15);

constexpr S BishopOutpost = S(15, 10);

// A knight that can jump to an outpost
constexpr S ReachableOutpost = S(10, 5);

// A pawn right in front shields it
constexpr S MinorBehindPawn = S(8, 2);

// Per own pawn on the bishop's colour
constexpr S BishopPawns = S(-2, -4);

// Sees both center squares of a long diagonal
constexpr S BishopLongDiagonal = S(15, 0);

// Per square away from our king
constexpr S KnightKingDistance = S(-3, -2);

constexpr S BishopKingDistance = S(-2, -1);

// No pawns on the file
constexpr S RookOpenFile = S(25, 10);

// No pawn of ours on the file
constexpr S RookSemiOpenFile = S(12, 6);

constexpr S RookOnQueenFile = S(5, 5);

// Shut in by its own king (twice when the king can't castle)
constexpr S TrappedRook = S(-25, -5);

// For the side to move (unused)
constexpr S Tempo = S(20, 10);
