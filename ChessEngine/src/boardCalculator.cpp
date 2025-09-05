#include "boardCalculator.hpp"

#include <algorithm>
#include <iostream>

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
	int pawnDir = (byColor == Color::WHITE) ? 1 : -1; // White pawns attack up (-1), black down (+1)
	int pawnRows[2] = { row + pawnDir, row + pawnDir };
	int pawnCols[2] = { col - 1, col + 1 };
	for (int i = 0; i < 2; i++)
	{
		if (InBounds(pawnRows[i], pawnCols[i]))
		{
			Piece p = board[pawnRows[i]][pawnCols[i]].GetPiece();
			if (p.type == Pieces::PAWN && p.color == byColor)
				return true;
		}
	}

	// 2. Knights
	for (int i = 0; i < 8; ++i)
	{
		int r = row + knightOffsets[i][0], c = col + knightOffsets[i][1];
		if (InBounds(r, c))
		{
			Piece p = board[r][c].GetPiece();
			if (p.type == Pieces::KNIGHT && p.color == byColor)
				return true;
		}
	}

	// 3. Bishops / Queens (diagonals)
	for (int i = 0; i < 4; ++i)
	{
		int dr = bishopDirs[i][0], dc = bishopDirs[i][1];
		int r = row + dr, c = col + dc;
		while (InBounds(r, c))
		{
			Piece p = board[r][c].GetPiece();
			if (p.type != Pieces::NONE)
			{
				if (p.color == byColor &&
					(p.type == Pieces::BISHOP || p.type == Pieces::QUEEN))
					return true;
				break; // Blocked
			}
			r += dr; c += dc;
		}
	}

	// 4. Rooks / Queens (straights)
	for (int i = 0; i < 4; ++i)
	{
		int dr = rookDirs[i][0], dc = rookDirs[i][1];
		int r = row + dr, c = col + dc;
		while (InBounds(r, c))
		{
			Piece p = board[r][c].GetPiece();
			if (p.type != Pieces::NONE)
			{
				if (p.color == byColor &&
					(p.type == Pieces::ROOK || p.type == Pieces::QUEEN))
					return true;
				break; // Blocked
			}
			r += dr; c += dc;
		}
	}

	// 5. King
	for (int i = 0; i < 8; ++i)
	{
		int r = row + kingOffsets[i][0], c = col + kingOffsets[i][1];
		if (InBounds(r, c))
		{
			Piece p = board[r][c].GetPiece();
			if (p.type == Pieces::KING && p.color == byColor)
				return true;
		}
	}

	return false; // No attackers found
}


bool BoardCalculator::InBounds(int row, int col)
{
	return row >= 0 && row < 8 && col >= 0 && col < 8;
}

std::vector<uint8_t> BoardCalculator::GetAttackedSquares(Color color, const Square board[8][8])
{
	// Attacked can be used as a bitboard for attacked squares
	std::array<bool, 64> attacked = { false };

	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 8; ++j)
		{
			Piece piece = board[i][j].GetPiece();
			if (piece == Pieces::NONE || piece.GetColor() != color) continue;

			switch (piece)
			{
			case Pieces::PAWN:
				AddPawnAttacks(i, j, color, attacked);
				break;
			case Pieces::KNIGHT:
				AddKnightAttacks(i, j, attacked);
				break;
			case Pieces::BISHOP:
				AddSlidingAttacks(i, j, attacked, bishopDirs, 4, board);
				break;
			case Pieces::ROOK:
				AddSlidingAttacks(i, j, attacked, rookDirs, 4, board);
				break;
			case Pieces::QUEEN:
				AddSlidingAttacks(i, j, attacked, queenDirs, 8, board);
				break;
			case Pieces::KING:
				AddKingAttacks(i, j, attacked);
				break;
			}
		}
	}

	std::vector<uint8_t> attackedSquares;
	for (int idx = 0; idx < 64; ++idx)
	{
		if (attacked[idx])
		{
			int row = idx / 8;
			int col = idx % 8;
			Piece targetPiece = board[row][col].GetPiece();
			if (targetPiece == Pieces::NONE || targetPiece.GetColor() != color)
				attackedSquares.push_back(idx);
		}
	}

	return attackedSquares;
}

// Bitboard
// TODO: Add castling and en passant
std::vector<uint8_t> BoardCalculator::GetValidMoves(int row, int col, const Square board[8][8])
{
	Piece piece = board[row][col].GetPiece();
	if (piece == Pieces::NONE) return {}; // No piece to move
	Color color = piece.GetColor();
	std::array<bool, 64> moves = { false };
	switch (piece)
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
			if (p.type == Pieces::KING && p.color == color)
			{
				kingRow = r; kingCol = c;
				break;
			}
		}
		if (kingRow != -1) break;
	}

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

std::vector<uint8_t> BoardCalculator::GetAllMoves(Color color, const Square board[8][8])
{
	std::array<bool, 64> allMoves = { false };
	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 8; ++j)
		{
			Piece piece = board[i][j].GetPiece();
			if (piece == Pieces::NONE || piece.GetColor() != color) continue;
			switch (piece)
			{
			case Pieces::PAWN:
				AddPawnMoves(i, j, color, allMoves, board);
				break;
			case Pieces::KNIGHT:
				AddKnightMoves(i, j, color, allMoves, board);
				break;
			case Pieces::BISHOP:
				AddSlidingMoves(i, j, color, allMoves, bishopDirs, 4, board);
				break;
			case Pieces::ROOK:
				AddSlidingMoves(i, j, color, allMoves, rookDirs, 4, board);
				break;
			case Pieces::QUEEN:
				AddSlidingMoves(i, j, color, allMoves, queenDirs, 8, board);
				break;
			case Pieces::KING:
				AddKingMoves(i, j, color, allMoves, board);
				break;
			}
		}
	}

	std::vector<uint8_t> validMoves;
	for (int idx = 0; idx < 64; ++idx)
	{
		if (allMoves[idx])
			validMoves.push_back(idx);
	}
	return validMoves;
}

void BoardCalculator::AddKnightAttacks(int row, int col, std::array<bool, 64>& attacked)
{
	for (int i = 0; i < 8; ++i) {
		int nr = row + knightOffsets[i][0], nc = col + knightOffsets[i][1];
		if (0 <= nr && nr < 8 && 0 <= nc && nc < 8)
			attacked[nr * 8 + nc] = true;
	}
}

void BoardCalculator::AddSlidingAttacks(int row, int col, std::array<bool, 64>& attacked, const int dirs[][2], int numDirs,
	const Square board[8][8])
{
	for (int i = 0; i < numDirs; ++i) {
		int dr = dirs[i][0], dc = dirs[i][1];
		int nr = row + dr, nc = col + dc;
		while (0 <= nr && nr < 8 && 0 <= nc && nc < 8) {
			attacked[nr * 8 + nc] = true;
			if (board[nr][nc].GetPiece() != Pieces::NONE) break; // Blocked
			nr += dr; nc += dc;
		}
	}
}

void BoardCalculator::AddPawnAttacks(int row, int col, Color color, std::array<bool, 64>& attacked)
{
	int dir = (color == Color::WHITE) ? -1 : 1;  // White pawns move "up" (row decreases), black "down" (row increases)

	// Two attack directions: left diagonal and right diagonal
	for (int dc : {-1, 1})
	{
		int nr = row + dir;
		int nc = col + dc;

		if (0 <= nr && nr < 8 && 0 <= nc && nc < 8)
			attacked[nr * 8 + nc] = true;
	}
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
			if (target == Pieces::NONE || target.GetColor() != color)
			{
				// Check for moving into check
				if (!IsSquareAttacked(nr, nc, (color == Color::WHITE) ? Color::BLACK : Color::WHITE, board))
					moves[nr * 8 + nc] = true;
			}
		}
	}
}

void BoardCalculator::AddPawnMoves(int row, int col, Color color, std::array<bool, 64>& moves, const Square board[8][8])
{
	int dir = (color == Color::WHITE) ? -1 : 1;  // White pawns move "up" (row decreases), black "down" (row increases)

	// One square forward
	int nr = row + dir;
	if (0 <= nr && nr < 8 && board[nr][col].GetPiece() == Pieces::NONE)
	{
		moves[nr * 8 + col] = true;
		// Two squares forward from starting position
		if ((color == Color::WHITE && row == 6) || (color == Color::BLACK && row == 1))
		{
			int nnr = row + 2 * dir;
			if (0 <= nnr && nnr < 8 && board[nnr][col].GetPiece() == Pieces::NONE)
			{
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
			if (target != Pieces::NONE && target.GetColor() != color)
			{
				moves[nr * 8 + nc] = true;
			}
		}
	}

	// Note: En passant and promotion are not handled here
}

void BoardCalculator::AddKnightMoves(int row, int col, Color color, std::array<bool, 64>& moves, const Square board[8][8])
{
	for (int i = 0; i < 8; ++i) {
		int nr = row + knightOffsets[i][0], nc = col + knightOffsets[i][1];
		if (0 <= nr && nr < 8 && 0 <= nc && nc < 8) {
			Piece target = board[nr][nc].GetPiece();
			if (target == Pieces::NONE || target.GetColor() != color) {
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
			if (target == Pieces::NONE) {
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

void BoardCalculator::AddKingAttacks(int row, int col, std::array<bool, 64>& attacked)
{
	for (int i = 0; i < 8; ++i)
	{
		int nr = row + kingOffsets[i][0];
		int nc = col + kingOffsets[i][1];

		if (0 <= nr && nr < 8 && 0 <= nc && nc < 8)
			attacked[nr * 8 + nc] = true;
	}
}
