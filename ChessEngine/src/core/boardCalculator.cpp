#include "boardCalculator.hpp"

#include <iostream>

#include "gameState.hpp"

// Define offsets and directions
const int BoardCalculator::knightOffsets[8][2] = {
	{2, 1}, {1, 2}, {-1, 2}, {-2, 1},
	{-2, -1}, {-1, -2}, {1, -2}, {2, -1}
};
const int BoardCalculator::kingOffsets[8][2] = {
	{1, 0}, {1, 1}, {0, 1}, {-1, 1},
	{-1, 0}, {-1, -1}, {0, -1}, {1, -1}
};
const int BoardCalculator::bishopDirs[4][2] = { {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };
const int BoardCalculator::rookDirs[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
const int BoardCalculator::queenDirs[8][2] = { {1 ,1}, {1, -1}, {-1, 1}, {-1, -1},
											   {1, 0}, {-1, 0}, {0, 1}, {0, -1} };

bool BoardCalculator::IsSquareAttacked(int row, int col, Color byColor, const Square board[8][8])
{
	if (!InBounds(row, col) || byColor == Color::NONE)
		return false;

	// 1. Pawns
	int pawnDir = (byColor == Color::WHITE) ? 1 : -1;
	if (InBounds(row + pawnDir, col - 1)) {
		Piece p = board[row + pawnDir][col - 1].GetPiece();
		if (p.GetType() == Pieces::PAWN && p.GetColor() == byColor) return true;
	}
	if (InBounds(row + pawnDir, col + 1)) {
		Piece p = board[row + pawnDir][col + 1].GetPiece();
		if (p.GetType() == Pieces::PAWN && p.GetColor() == byColor) return true;
	}

	// 2. Knights
	for (int i = 0; i < 8; ++i) {
		int r = row + knightOffsets[i][0], c = col + knightOffsets[i][1];
		if (InBounds(r, c)) {
			Piece p = board[r][c].GetPiece();
			if (p.GetType() == Pieces::KNIGHT && p.GetColor() == byColor) return true;
		}
	}
	
	// 3. Sliding pieces (Rooks, Bishops, Queens)
	for (int i = 0; i < 8; ++i) {
		int dr = queenDirs[i][0], dc = queenDirs[i][1];
		int r = row + dr, c = col + dc;
		while (InBounds(r, c)) {
			Piece p = board[r][c].GetPiece();
			if (p.GetType() != Pieces::NONE) {
				if (p.GetColor() == byColor) {
					bool isDiagonal = (dr != 0 && dc != 0);
					if (p.GetType() == Pieces::QUEEN || (p.GetType() == Pieces::BISHOP && isDiagonal) || (p.GetType() == Pieces::ROOK && !isDiagonal))
						return true;
				}
				break; // Blocked
			}
			r += dr; c += dc;
		}
	}

	// 4. King
	for (int i = 0; i < 8; ++i) {
		int r = row + kingOffsets[i][0], c = col + kingOffsets[i][1];
		if (InBounds(r, c)) {
			Piece p = board[r][c].GetPiece();
			if (p.GetType() == Pieces::KING && p.GetColor() == byColor) return true;
		}
	}

	return false; // No attackers found
}


bool BoardCalculator::InBounds(int row, int col)
{
	return row >= 0 && row < 8 && col >= 0 && col < 8;
}

// Bitboard
std::vector<uint8_t> BoardCalculator::GetValidMoves(int row, int col, const Square board[8][8])
{
	Piece piece = board[row][col].GetPiece();
	if (piece.GetType() == Pieces::NONE) return {}; // No piece to move
	Color color = piece.GetColor();
	std::array<bool, 64> moves = { false };
	switch (piece.GetType())
	{
	case Pieces::PAWN:
		AddPawnMoves(row, col, color, moves, board);
		break;
	case Pieces::KNIGHT:
		AddKnightMoves(row, col, color, moves, board);
		break;
	case Pieces::BISHOP:
		AddSlidingMoves(row, col, color, moves, bishopDirs, 4, board);
		break;
	case Pieces::ROOK:
		AddSlidingMoves(row, col, color, moves, rookDirs, 4, board);
		break;
	case Pieces::QUEEN:
		AddSlidingMoves(row, col, color, moves, queenDirs, 8, board);
		break;
	case Pieces::KING:
		AddKingMoves(row, col, color, moves, board);
		break;
	default:
		return {};
	}

	// Now filter out moves that would put or leave the king in check
	std::vector<uint8_t> validMoves;
	Square tempBoard[8][8];
	memcpy(tempBoard, board, sizeof(Square) * 64);

	// Find king position
	int kingRow = -1, kingCol = -1;
	for (int r = 0; r < 8; r++)
	{
		for (int c = 0; c < 8; c++)
		{
			Piece p = tempBoard[r][c].GetPiece();
			if (p.GetType() == Pieces::KING && p.GetColor() == color)
			{
				kingRow = r; kingCol = c;
				break;
			}
		}
		if (kingRow != -1) break;
	}

	// Remove moves that put or leave king in check
	for (int idx = 0; idx < 64; ++idx)
	{
		if (!moves[idx]) continue;

		// Apply the move (row,col -> idx/8,idx%8)
		int destRow = idx / 8;
		int destCol = idx % 8;
		Piece movingPiece = tempBoard[row][col].GetPiece();
		Piece capturedPiece = tempBoard[destRow][destCol].GetPiece();
		tempBoard[destRow][destCol].SetPiece(movingPiece);
		tempBoard[row][col].SetPiece(Piece(Pieces::NONE, Color::NONE));

		// If the king moved, update its position for the check
		int currentKingRow = kingRow;
		int currentKingCol = kingCol;
		if (movingPiece.type == Pieces::KING)
		{
			currentKingRow = destRow;
			currentKingCol = destCol;
		}

		// Check if king is attacked
		Color enemy = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
		if (!IsSquareAttacked(currentKingRow, currentKingCol, enemy, tempBoard))
			validMoves.push_back(idx); // Legal move

		// Undo the move
		tempBoard[row][col].SetPiece(movingPiece);
		tempBoard[destRow][destCol].SetPiece(capturedPiece);
	}

	return validMoves;
}

std::vector<Move> BoardCalculator::GetAllMoves(Color color, const Square board[8][8])
{
	std::vector<Move> moves;
	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 8; ++j)
		{
			Piece piece = board[i][j].GetPiece();
			if (piece.GetType() == Pieces::NONE || piece.GetColor() != color) continue; // No piece or wrong color


			std::array<bool, 64> pieceMoves = { false };

			// Generate moves for the piece
			switch (piece.GetType())
			{
			case Pieces::PAWN:
			{
				AddPawnMoves(i, j, color, pieceMoves, board);

				// Promotion
				if ((color == Color::WHITE && i == 1) ||
				    (color == Color::BLACK && i == 6))
				{
					int endRow = (color == Color::WHITE) ? 0 : 7;

					for (int dc = -1; dc <= 1; ++dc)
					{
						int endCol = j + dc;
						if (endCol < 0 || endCol >= 8) continue;

						int idx = endRow * 8 + endCol;
						if (pieceMoves[idx])
						{
							for (Pieces promo : {Pieces::QUEEN, Pieces::ROOK, Pieces::BISHOP, Pieces::KNIGHT})
							{
								Move move;
								move.startRow = i;
								move.startCol = j;
								move.endRow = endRow;
								move.endCol = endCol;
								move.promotion = static_cast<int>(promo);
								moves.push_back(move);
							}
						}
					}
					continue; // Skip default pawn move gen
				}
				break;
			}
			case Pieces::KNIGHT:
				AddKnightMoves(i, j, color, pieceMoves, board);
				break;
			case Pieces::BISHOP:
				AddSlidingMoves(i, j, color, pieceMoves, bishopDirs, 4, board);
				break;
			case Pieces::ROOK:
				AddSlidingMoves(i, j, color, pieceMoves, rookDirs, 4, board);
				break;
			case Pieces::QUEEN:
				AddSlidingMoves(i, j, color, pieceMoves, queenDirs, 8, board);
				break;
			case Pieces::KING:
				AddKingMoves(i, j, color, pieceMoves, board);
				break;
			}

			// Convert pieceMoves to Move structs
			for (int idx = 0; idx < 64; ++idx)
			{
				if (pieceMoves[idx])
				{
					Move move;
					move.startRow = i;
					move.startCol = j;
					move.endRow = idx / 8;
					move.endCol = idx % 8;
					moves.push_back(move);
				}
			}

			//std::cout << "Found " << moves.size() << " moves for piece at " << i << "," << j << "\n";
		}
	}

	// Filter out moves that leave king in check
	// This is not strictly necessary for pseudo-legal moves, but can be useful
	// for certain engine algorithms that expect only legal moves.
	std::vector<Move> legalMoves;
	for (const Move& move : moves)
	{
		Square tempBoard[8][8];
		memcpy(tempBoard, board, sizeof(Square) * 64);
		// Apply the move
		Piece movingPiece = tempBoard[move.startRow][move.startCol].GetPiece();
		Piece capturedPiece = tempBoard[move.endRow][move.endCol].GetPiece();
		tempBoard[move.endRow][move.endCol].SetPiece(movingPiece);
		tempBoard[move.startRow][move.startCol].SetPiece(Piece(Pieces::NONE, Color::NONE));
		// Find king position
		int kingRow = -1, kingCol = -1;
		for (int r = 0; r < 8; r++)
		{
			for (int c = 0; c < 8; c++)
			{
				Piece p = tempBoard[r][c].GetPiece();
				if (p.GetType() == Pieces::KING && p.GetColor() == color)
				{
					kingRow = r; kingCol = c;
					break;
				}
			}
			if (kingRow != -1) break;
		}
		Color enemy = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
		if (!IsSquareAttacked(kingRow, kingCol, enemy, tempBoard))
			legalMoves.push_back(move);
		// No need to undo the move since we used a copy of the board
	}

	return legalMoves;
}

uint8_t BoardCalculator::FindPiece(Piece piece, const Square board[8][8])
{
	for (int r = 0; r < 8; r++)
	{
		for (int c = 0; c < 8; c++)
		{
			if (board[r][c].GetPiece() == piece)
				return r * 8 + c;
		}
	}
	return 64; // Not found
}

bool BoardCalculator::IsCastlingValid(bool kingside, const Square board[8][8])
{
	// Check if castling is allowed
	if ((GameState::currentPlayer == Color::WHITE && !GameState::whiteCastlingRights[kingside ? 1 : 0]) ||
		(GameState::currentPlayer == Color::BLACK && !GameState::blackCastlingRights[kingside ? 1 : 0]))
	{
		//invalidMove = true;
		return false;
	}

	int row = (GameState::currentPlayer == Color::WHITE) ? 7 : 0; // Row 7 for white, 0 for black
	int rookColumn = (kingside) ? 7 : 0;
	int kingColumn = 4; // Only used to avoid '4' as a pseudo magic number
	// +1 or -1, used for stepping to check missing pieces and to move rook and king to correct space
	int step = (kingside) ? 1 : -1; // -1 for queenside (down the columns), 1 for kingside

	// Squares between king and rook not empty
	// Only need to check 2 squares for kingside castling (f1/f8 and g1/g8)
	int squaresToCheck = kingside ? 2 : 3;
	for (int offset = 1; offset <= squaresToCheck; offset++)
	{
		if (!board[row][kingColumn + (step * offset)].IsEmpty())
		{
			//invalidMove = true;
			return false;
		}
	}

	return true;
}


void BoardCalculator::AddKingMoves(int row, int col, Color color, std::array<bool, 64>& moves, const Square board[8][8])
{
	for (int i = 0; i < 8; ++i)
	{
		int nr = row + kingOffsets[i][0];
		int nc = col + kingOffsets[i][1];
		if (0 <= nr && nr < 8 && 0 <= nc && nc < 8)
		{
			Piece target = board[nr][nc].GetPiece();
			if (target.GetType() == Pieces::NONE || target.GetColor() != color)
			{
				// Check for moving into check
				if (!IsSquareAttacked(nr, nc, (color == Color::WHITE) ? Color::BLACK : Color::WHITE, board))
					moves[nr * 8 + nc] = true;
			}
		}
	}

	// Castling
	if (IsCastlingValid(true, board)) // Kingside
	{
		int castlingCol = col + 2;
		moves[row * 8 + castlingCol] = true;
	}
	if (IsCastlingValid(false, board)) // Queenside
	{
		int castlingCol = col - 2;
		moves[row * 8 + castlingCol] = true;
	}

}

void BoardCalculator::AddPawnMoves(int row, int col, Color color, std::array<bool, 64>& moves, const Square board[8][8])
{
	int dir = (color == Color::WHITE) ? -1 : 1;  // White pawns move "up" (row decreases), black "down" (row increases)

	// One square forward
	int nr = row + dir;
	if (0 <= nr && nr < 8 && board[nr][col].GetPiece().GetType() == Pieces::NONE)
	{
		moves[nr * 8 + col] = true;
		// Two squares forward from starting position
		if ((color == Color::WHITE && row == 6) || (color == Color::BLACK && row == 1))
		{
			int nnr = row + 2 * dir;
			if (0 <= nnr && nnr < 8 && board[nnr][col].GetPiece().GetType() == Pieces::NONE)
			{
				//std::cout << "added pawn move\n";
				moves[nnr * 8 + col] = true;
			}
		}
	}

	// Captures
	for (int dc : {-1, 1})
	{
		int nc = col + dc;
		if (0 <= nr && nr < 8 && 0 <= nc && nc < 8)
		{
			Piece target = board[nr][nc].GetPiece();
			if (target.GetType() != Pieces::NONE && target.GetColor() != color)
			{
				moves[nr * 8 + nc] = true;
			}
		}
	}

	// En passant
	if (GameState::enPassantTarget[0] == nr && abs(GameState::enPassantTarget[1] - col) == 1)
	{
		int epCol = GameState::enPassantTarget[1];
		moves[nr * 8 + epCol] = true;
	}
}

void BoardCalculator::AddKnightMoves(int row, int col, Color color, std::array<bool, 64>& moves, const Square board[8][8])
{
	for (int i = 0; i < 8; ++i) {
		int nr = row + knightOffsets[i][0], nc = col + knightOffsets[i][1];
		if (0 <= nr && nr < 8 && 0 <= nc && nc < 8) {
			Piece target = board[nr][nc].GetPiece();
			if (target.GetType() == Pieces::NONE || target.GetColor() != color) {
				moves[nr * 8 + nc] = true;
			}
		}
	}
}

void BoardCalculator::AddSlidingMoves(int row, int col, Color color, std::array<bool, 64>& moves, const int dirs[][2], int numDirs, const Square board[8][8])
{
	for (int i = 0; i < numDirs; ++i) {
		int dr = dirs[i][0], dc = dirs[i][1];
		int nr = row + dr, nc = col + dc;
		while (0 <= nr && nr < 8 && 0 <= nc && nc < 8) {
			Piece target = board[nr][nc].GetPiece();
			if (target.GetType() == Pieces::NONE) {
				moves[nr * 8 + nc] = true;
			}
			else {
				if (target.GetColor() != color) {
					moves[nr * 8 + nc] = true;
				}
				break; // Blocked
			}
			nr += dr; nc += dc;
		}
	}
}

//void BoardCalculator::AddKnightAttacks(int row, int col, std::array<bool, 64>& attacked)
//{
//	for (int i = 0; i < 8; ++i) {
//		int nr = row + knightOffsets[i][0], nc = col + knightOffsets[i][1];
//		if (0 <= nr && nr < 8 && 0 <= nc && nc < 8)
//			attacked[nr * 8 + nc] = true;
//	}
//}
//
//void BoardCalculator::AddSlidingAttacks(int row, int col, std::array<bool, 64>& attacked, const int dirs[][2], int numDirs,
//	const Square board[8][8])
//{
//	for (int i = 0; i < numDirs; ++i) {
//		int dr = dirs[i][0], dc = dirs[i][1];
//		int nr = row + dr, nc = col + dc;
//		while (0 <= nr && nr < 8 && 0 <= nc && nc < 8) {
//			attacked[nr * 8 + nc] = true;
//			if (board[nr][nc].GetPiece() != Pieces::NONE) break; // Blocked
//			nr += dr; nc += dc;
//		}
//	}
//}
//
//void BoardCalculator::AddPawnAttacks(int row, int col, Color color, std::array<bool, 64>& attacked)
//{
//	int dir = (color == Color::WHITE) ? -1 : 1;  // White pawns move "up" (row decreases), black "down" (row increases)
//
//	// Two attack directions: left diagonal and right diagonal
//	for (int dc : {-1, 1})
//	{
//		int nr = row + dir;
//		int nc = col + dc;
//
//		if (0 <= nr && nr < 8 && 0 <= nc && nc < 8)
//			attacked[nr * 8 + nc] = true;
//	}
//}

//void BoardCalculator::AddKingAttacks(int row, int col, std::array<bool, 64>& attacked)
//{
//	for (int i = 0; i < 8; ++i)
//	{
//		int nr = row + kingOffsets[i][0];
//		int nc = col + kingOffsets[i][1];
//
//		if (0 <= nr && nr < 8 && 0 <= nc && nc < 8)
//			attacked[nr * 8 + nc] = true;
//	}
//}

//std::vector<uint8_t> BoardCalculator::GetAttackedSquares(Color color, const Square board[8][8])
//{
//	// Attacked can be used as a bitboard for attacked squares
//	std::array<bool, 64> attacked = { false };
//
//	for (int i = 0; i < 8; ++i)
//	{
//		for (int j = 0; j < 8; ++j)
//		{
//			Piece piece = board[i][j].GetPiece();
//			if (piece == Pieces::NONE || piece.GetColor() != color) continue;
//
//			switch (piece)
//			{
//			case Pieces::PAWN:
//				AddPawnAttacks(i, j, color, attacked);
//				break;
//			case Pieces::KNIGHT:
//				AddKnightAttacks(i, j, attacked);
//				break;
//			case Pieces::BISHOP:
//				AddSlidingAttacks(i, j, attacked, bishopDirs, 4, board);
//				break;
//			case Pieces::ROOK:
//				AddSlidingAttacks(i, j, attacked, rookDirs, 4, board);
//				break;
//			case Pieces::QUEEN:
//				AddSlidingAttacks(i, j, attacked, queenDirs, 8, board);
//				break;
//			case Pieces::KING:
//				AddKingAttacks(i, j, attacked);
//				break;
//			}
//		}
//	}
//
//	std::vector<uint8_t> attackedSquares;
//	for (int idx = 0; idx < 64; ++idx)
//	{
//		if (attacked[idx])
//		{
//			int row = idx / 8;
//			int col = idx % 8;
//			Piece targetPiece = board[row][col].GetPiece();
//			if (targetPiece == Pieces::NONE || targetPiece.GetColor() != color)
//				attackedSquares.push_back(idx);
//		}
//	}
//
//	return attackedSquares;
//}