#include "eval.hpp"

// White pawns (ranks from White's perspective)
int pawnPST_white[64] = {
	 0,  0,  0,  0,  0,  0,  0,  0,
	50, 50, 50, 50, 50, 50, 50, 50,
	10, 10, 20, 30, 30, 20, 10, 10,
	 5,  5, 10, 25, 25, 10,  5,  5,
	 0,  0,  0, 20, 20,  0,  0,  0,
	 5, -5,-10,  0,  0,-10, -5,  5,
	 5, 10, 10,-20,-20, 10, 10,  5,
	 0,  0,  0,  0,  0,  0,  0,  0
};

// Black pawns (mirrored)
int pawnPST_black[64] = {
	 0,  0,  0,  0,  0,  0,  0,  0,
	 5, 10, 10,-20,-20, 10, 10,  5,
	 5, -5,-10,  0,  0,-10, -5,  5,
	 0,  0,  0, 20, 20,  0,  0,  0,
	 5,  5, 10, 25, 25, 10,  5,  5,
	10, 10, 20, 30, 30, 20, 10, 10,
	50, 50, 50, 50, 50, 50, 50, 50,
	 0,  0,  0,  0,  0,  0,  0,  0
};

// White king, middle game (mirrored)
int kingPST_mg_white[64] = {
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -10,-20,-20,-20,-20,-20,-20,-10,
	20, 20,  0,  0,  0,  0, 20, 20,
	20, 30, 10,  0,  0, 10, 30, 20
};

// Black king, middle game
int kingPST_mg_black[64] = {
	20, 30, 10,  0,  0, 10, 30, 20,
	20, 20,  0,  0,  0,  0, 20, 20,
   -10,-20,-20,-20,-20,-20,-20,-10,
   -20,-30,-30,-40,-40,-30,-30,-20,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30,
   -30,-40,-40,-50,-50,-40,-40,-30
};

// White king, endgame (mirrored)
int kingPST_eg_white[64] = {
   -50,-40,-30,-20,-20,-30,-40,-50,
   -30,-20,-10,  0,  0,-10,-20,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-30,  0,  0,  0,  0,-30,-30,
   -50,-30,-30,-30,-30,-30,-30,-50
};

// Black king, endgame
int kingPST_eg_black[64] = {
   -50,-30,-30,-30,-30,-30,-30,-50,
   -30,-30,  0,  0,  0,  0,-30,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 30, 40, 40, 30,-10,-30,
   -30,-10, 20, 30, 30, 20,-10,-30,
   -30,-20,-10,  0,  0,-10,-20,-30,
   -50,-40,-30,-20,-20,-30,-40,-50
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

int Eval(const Square board[8][8])
{
	int score = 0;
	const int pieceValues[] = { 0, 100, 320, 330, 500, 900, 100000 };

	int whitePawns = 0, blackPawns = 0;
	int whiteKnights = 0, blackKnights = 0;
	int whiteBishops = 0, blackBishops = 0;
	int whiteRooks = 0, blackRooks = 0;
	int whiteQueens = 0, blackQueens = 0;

	for (int r = 0; r < 8; ++r)
	{
		for (int c = 0; c < 8; ++c)
		{
			Piece piece = board[r][c].GetPiece();
			if (piece.GetType() == Pieces::NONE) continue;

			// convert (r,c) into 0..63 index
			int sq = r * 8 + c;

			int value = pieceValues[static_cast<int>(piece.GetType()) + 1];

			if (piece.GetColor() == Color::WHITE)
			{
				score += value;

				switch (piece.GetType())
				{
				case Pieces::PAWN:
					score += pawnPST_white[sq];
					whitePawns++;
					break;
				case Pieces::KNIGHT:
					score += knightPST[sq];
					whiteKnights++;
					break;
				case Pieces::BISHOP:
					score += bishopPST[sq];
					whiteBishops++;
					break;
				case Pieces::ROOK:
					score += rookPST[sq];
					whiteRooks++;
					break;
				case Pieces::QUEEN:
					score += queenPST[sq];
					whiteQueens++;
					break;
				case Pieces::KING:
					if (!GameState::endgameStatus)
						score += kingPST_mg_white[sq];
					else
						score += kingPST_eg_white[sq];
					break;
				default:
					break;
				}
			}
			else // black piece
			{
				score -= value;

				switch (piece.GetType())
				{
				case Pieces::PAWN:
					score -= pawnPST_black[sq];
					blackPawns++;
					break;
				case Pieces::KNIGHT:
					score -= knightPST[sq];
					blackKnights++;
					break;
				case Pieces::BISHOP:
					score -= bishopPST[sq];
					blackBishops++;
					break;
				case Pieces::ROOK:
					score -= rookPST[sq];
					blackRooks++;
					break;
				case Pieces::QUEEN:
					score -= queenPST[sq];
					blackQueens++;
					break;
				case Pieces::KING:
					if (!GameState::endgameStatus)
						score -= kingPST_mg_black[sq];
					else
						score -= kingPST_eg_black[sq];
					break;
				default:
					break;
				}
			}
		}
	}

	// pawn capture penalties to knights
	int whitePawnsCaptured = 8 - whitePawns;
	int blackPawnsCaptured = 8 - blackPawns;

	score -= whiteKnights * (whitePawnsCaptured * 10);
	score += blackKnights * (blackPawnsCaptured * 10);

	return score;
}
