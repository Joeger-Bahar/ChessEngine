#include "eval.hpp"

// White pawns (ranks from White's perspective)
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

//// Black pawns (mirrored)
//int pawnPST_black[64] = {
//	 0,  0,  0,  0,  0,  0,  0,  0,
//	 5, 10, 10,-20,-20, 10, 10,  5,
//	 5, -5,-10,  0,  0,-10, -5,  5,
//	 0,  0,  0, 20, 20,  0,  0,  0,
//	 5,  5, 10, 25, 25, 10,  5,  5,
//	10, 10, 20, 30, 30, 20, 10, 10,
//	50, 50, 50, 50, 50, 50, 50, 50,
//	 0,  0,  0,  0,  0,  0,  0,  0
//};

// White king, middle game (mirrored)
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

//// Black king, middle game
//int kingPST_mg_black[64] = {
//	20, 30, 10,  0,  0, 10, 30, 20,
//	20, 20,  0,  0,  0,  0, 20, 20,
//   -10,-20,-20,-20,-20,-20,-20,-10,
//   -20,-30,-30,-40,-40,-30,-30,-20,
//   -30,-40,-40,-50,-50,-40,-40,-30,
//   -30,-40,-40,-50,-50,-40,-40,-30,
//   -30,-40,-40,-50,-50,-40,-40,-30,
//   -30,-40,-40,-50,-50,-40,-40,-30
//};

// White king, endgame (mirrored)
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

//// Black king, endgame
//int kingPST_eg_black[64] = {
//   -50,-30,-30,-30,-30,-30,-30,-50,
//   -30,-30,  0,  0,  0,  0,-30,-30,
//   -30,-10, 20, 30, 30, 20,-10,-30,
//   -30,-10, 30, 40, 40, 30,-10,-30,
//   -30,-10, 30, 40, 40, 30,-10,-30,
//   -30,-10, 20, 30, 30, 20,-10,-30,
//   -30,-20,-10,  0,  0,-10,-20,-30,
//   -50,-40,-30,-20,-20,-30,-40,-50
//};

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

int Eval(Color player, const Square board[64])
{
	int score = 0;
	const int pieceValues[] = { 0, 100, 320, 330, 500, 900, 100000 };

	int whitePawns = 0, blackPawns = 0;
	int whiteKnights = 0, blackKnights = 0;
	int whiteBishops = 0, blackBishops = 0;
	int whiteRooks = 0, blackRooks = 0;
	int whiteQueens = 0, blackQueens = 0;

	for (int sq = 0; sq < 64; ++sq)
	{
		Piece piece = board[sq].GetPiece();
		if (piece.GetType() == Pieces::NONE) continue;

		int value = pieceValues[static_cast<int>(piece.GetType()) + 1];
		bool isWhite = IsWhite(piece.GetColor());

		if (isWhite)
			score += value;
		else
			score -= value;


		switch (piece.GetType())
		{
		case Pieces::PAWN:
			if (isWhite)
			{
				score += pawnPST[sq];
				whitePawns++;
			}
			else
			{
				score -= pawnPST[Mirror(sq)];
				blackPawns++;
			}
			break;
		case Pieces::KNIGHT:
			if (isWhite)
			{
				score += knightPST[sq];
				whiteKnights++;
			}
			else
			{
				score -= knightPST[Mirror(sq)];
				blackKnights++;
			}
			break;
		case Pieces::BISHOP:
			if (isWhite)
			{
				score += bishopPST[sq];
				whiteBishops++;
			}
			else
			{
				score -= bishopPST[Mirror(sq)];
				blackBishops++;
			}
			break;
		case Pieces::ROOK:
			if (isWhite)
			{
				score += rookPST[sq];
				whiteRooks++;
			}
			else
			{
				score -= rookPST[Mirror(sq)];
				blackRooks++;
			}
			break;
		case Pieces::QUEEN:
			if (isWhite)
			{
				score += queenPST[sq];
				whiteQueens++;
			}
			else
			{
				score -= queenPST[Mirror(sq)];
				blackQueens++;
			}
			break;
		case Pieces::KING:
			if (!GameState::endgame)
			{
				if (isWhite) score += kingPST_mg[sq];
				else		 score -= kingPST_mg[Mirror(sq)];
			}
			else
			{
				if (isWhite) score += kingPST_eg[sq];
				else		 score -= kingPST_eg[Mirror(sq)];
			}
			break;
		default:
			break;
		}
		//else // Black piece
		//{
		//	score -= value;

		//	switch (piece.GetType())
		//	{
		//	case Pieces::PAWN:
		//		score -= pawnPST_black[sq];
		//		blackPawns++;
		//		break;
		//	case Pieces::KNIGHT:
		//		score -= knightPST[Mirror(sq)];
		//		blackKnights++;
		//		break;
		//	case Pieces::BISHOP:
		//		score -= bishopPST[Mirror(sq)];
		//		blackBishops++;
		//		break;
		//	case Pieces::ROOK:
		//		score -= rookPST[Mirror(sq)];
		//		blackRooks++;
		//		break;
		//	case Pieces::QUEEN:
		//		score -= queenPST[Mirror(sq)];
		//		blackQueens++;
		//		break;
		//	case Pieces::KING:
		//		if (!GameState::endgame)
		//			score -= kingPST_mg_black[sq];
		//		else
		//			score -= kingPST_eg_black[sq];
		//		break;
		//	default:
		//		break;
		//	}
		//}
	}

	// Pawn capture penalties to knights
	//int whitePawnsCaptured = 8 - whitePawns;
	//int blackPawnsCaptured = 8 - blackPawns;

	//score -= whiteKnights * (whitePawnsCaptured * 10);
	//score += blackKnights * (blackPawnsCaptured * 10);

	if (player == Color::BLACK) score *= -1;

	return score;
}
