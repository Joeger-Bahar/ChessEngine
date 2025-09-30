#include "boardCalculator.hpp"

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

#include "gameState.hpp"
#include "engine.hpp"

// Define offsets and directions
const int BoardCalculator::knightOffsets[8] = {
	-17, -15, -10, -6,
	  6,  10,  15, 17
};
const int BoardCalculator::kingOffsets[8] = {
	-9, -8, -7,
	-1,      1,
	 7,  8,  9
};
const int BoardCalculator::bishopDirs[4] = {
	-9,   -7,
//	    b
	7,	   9
};
const int BoardCalculator::rookDirs[4] = {
		-8,
	 -1,   1,
		 8
};
const int BoardCalculator::queenDirs[8] = {
	-9, -8, -7,
	-1,      1,
	 7,  8,  9
};

bool BoardCalculator::IsSquareAttacked(int sq, Color byColor, const Square board[64])
{
	if (!InBounds(sq) || byColor == Color::NONE)
		return false;

	int row = ToRow(sq);
	int col = ToCol(sq);

	// 1. Pawns
	int pawnDir = IsWhite(byColor) ? 1 : -1;
	// Up 1 row and across +1/-1 cols
	int pawnLeft = sq + pawnDir * 8 - 1;
	int pawnRight = sq + pawnDir * 8 + 1;

	if (col > 0 && InBounds(pawnLeft))
	{
		Piece p = board[pawnLeft].GetPiece();
		if (p.GetType() == Pieces::PAWN && p.GetColor() == byColor) return true;
	}
	if (col < 7 && InBounds(pawnRight))
	{
		Piece p = board[pawnRight].GetPiece();
		if (p.GetType() == Pieces::PAWN && p.GetColor() == byColor) return true;
	}

	// 2. Knights
	for (int i = 0; i < 8; ++i)
	{
		int target = sq + knightOffsets[i];
		if (!InBounds(target)) continue;
		if (std::abs(ToCol(target) - col) > 2) continue; // Prevent wraparoudn

		Piece p = board[target].GetPiece();
		if (p.GetType() == Pieces::KNIGHT && p.GetColor() == byColor) return true;
	}

	// 3. Straight sliding pieces (Rooks, Queens)
	for (int d = 0; d < 4; ++d)
	{
		int dir = rookDirs[d];
		int target = sq;

		while (true)
		{
			int prevCol = ToCol(target);
			target += dir;
			if (!InBounds(target)) break;

			// Prevent wrap across left/right
			if (std::abs(ToCol(target) - prevCol) > 1) break;

			Piece p = board[target].GetPiece();
			if (p.GetType() == Pieces::NONE) continue;

			if (p.GetColor() == byColor)
				if (p.GetType() == Pieces::ROOK || (p.GetType() == Pieces::QUEEN))
					return true;
			break; // Blocked
		}
	}

	// 4. Diagonal sliding pieces (Bishops, Queens)
	for (int d = 0; d < 4; ++d)
	{
		int dir = bishopDirs[d];
		int target = sq;

		while (true)
		{
			int prevCol = ToCol(target);
			target += dir;
			if (!InBounds(target)) break;

			// Prevent wrap across left/right
			if (std::abs(ToCol(target) - prevCol) > 1) break;

			Piece p = board[target].GetPiece();
			if (p.GetType() == Pieces::NONE) continue;

			if (p.GetColor() == byColor)
				if (p.GetType() == Pieces::BISHOP || p.GetType() == Pieces::QUEEN)
					return true;
			break; // Blocked
		}
	}

	// 4. King
	for (int i = 0; i < 8; ++i)
	{
		int target = sq + kingOffsets[i];
		if (!InBounds(target)) continue;
		if (std::abs(ToCol(target) - col) > 1) continue; // Prevent wraparound

		Piece p = board[target].GetPiece();
		if (p.GetType() == Pieces::KING && p.GetColor() == byColor) return true;
	}

	return false; // No attackers found
}

// Bitboard
std::vector<uint8_t> BoardCalculator::GetValidMoves(int sq, const Square board[64])
{
	Piece piece = board[sq].GetPiece();
	if (piece.GetType() == Pieces::NONE) return {}; // No piece to move

	Color color = piece.GetColor();
	std::array<bool, 64> moves = { false };

	switch (piece.GetType())
	{
	case Pieces::PAWN:
		AddPawnMoves(sq, color, moves, board);
		break;
	case Pieces::KNIGHT:
		AddKnightMoves(sq, color, moves, board);
		break;
	case Pieces::BISHOP:
		AddSlidingMoves(sq, color, moves, bishopDirs, 4, board);
		break;
	case Pieces::ROOK:
		AddSlidingMoves(sq, color, moves, rookDirs, 4, board);
		break;
	case Pieces::QUEEN:
		AddSlidingMoves(sq, color, moves, queenDirs, 8, board);
		break;
	case Pieces::KING:
		AddKingMoves(sq, color, moves, board);
		break;
	default:
		return {};
	}

	// Now filter out moves that would put or leave the king in check
	std::vector<uint8_t> validMoves;
	Square tempBoard[64];
	memcpy(tempBoard, board, sizeof(Square) * 64);

	// Find king position
	int kingSq = FindPiece(Piece(Pieces::KING, color), board);
	
	// Remove moves that put or leave king in check
	for (int endSq = 0; endSq < 64; ++endSq)
	{
		if (!moves[endSq]) continue;
		
		// Apply the move
		Piece movingPiece = tempBoard[sq].GetPiece();
		Piece capturedPiece = tempBoard[endSq].GetPiece();
		tempBoard[endSq].SetPiece(movingPiece);
		tempBoard[sq].SetPiece(Piece(Pieces::NONE, Color::NONE));

		// En passant
		if (movingPiece.type == Pieces::PAWN &&
			ToCol(endSq) != ToCol(sq) && capturedPiece.GetType() == Pieces::NONE)
		{
			// Remove the pawn that was passed
			int epRow = ToRow(sq);  // The pawn is on the same row as the moving pawn started
			tempBoard[ToIndex(epRow, ToCol(endSq))].SetPiece(Piece(Pieces::NONE, Color::NONE));
		}

		// If the king moved, update its position for the check
		if (movingPiece.type == Pieces::KING)
		{
			kingSq = endSq;
		}

		// Check if king is attacked
		Color enemy = Opponent(color);
		if (!IsSquareAttacked(kingSq, enemy, tempBoard))
			validMoves.push_back(endSq); // Legal move

		// Undo the move
		tempBoard[sq].SetPiece(movingPiece);
		tempBoard[endSq].SetPiece(capturedPiece);
	}

	return validMoves;
}

std::vector<Move> BoardCalculator::GetAllLegalMoves(Color color, const Square board[64], Engine* engine)
{
	std::vector<Move> moves;
	GetAllMoves(moves, color, board, engine);
	moves.erase(
		std::remove_if(moves.begin(), moves.end(),
			[&](Move move)
			{
				// Remove moves that cause checks
				engine->MakeMove(move);
				bool inCheck = engine->InCheck(Opponent(GameState::currentPlayer));
				engine->UndoMove();
				return inCheck;
			}),
		moves.end()
	);

	return moves;
}

void BoardCalculator::GetAllMoves(std::vector<Move>& moves, Color color, const Square board[64], Engine* engine,
	bool onlyNoisy)
{
	moves.clear();
	for (int sq = 0; sq < 64; ++sq)
	{
		Piece piece = board[sq].GetPiece();
		if (piece.GetType() == Pieces::NONE || piece.GetColor() != color) continue; // No piece or wrong color

		std::array<bool, 64> pieceMoves = { false };

		// Generate moves for the piece
		switch (piece.GetType())
		{
		case Pieces::PAWN:
		{
			AddPawnMoves(sq, color, pieceMoves, board);

			// Promotion
			if ((color == Color::WHITE && ToRow(sq) == 1) ||
				(color == Color::BLACK && ToRow(sq) == 6))
			{
				for (int dc = -1; dc <= 1; ++dc)
				{
					int endRow = IsWhite(color) ? 0 : 7;
					int endCol = ToCol(sq) + dc;
					if (endCol < 0 || endCol >= 8) continue;

					int endSq = ToIndex(endRow, endCol);
					if (pieceMoves[endSq])
					{
						for (Pieces promo : {Pieces::QUEEN, Pieces::ROOK, Pieces::BISHOP, Pieces::KNIGHT})
						{
							Move move;
							move.startSquare = sq;
							move.endSquare = endSq;
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
			AddKnightMoves(sq, color, pieceMoves, board);
			break;
		case Pieces::BISHOP:
			AddSlidingMoves(sq, color, pieceMoves, bishopDirs, 4, board);
			break;
		case Pieces::ROOK:
			AddSlidingMoves(sq, color, pieceMoves, rookDirs, 4, board);
			break;
		case Pieces::QUEEN:
			AddSlidingMoves(sq, color, pieceMoves, queenDirs, 8, board);
			break;
		case Pieces::KING:
			AddKingMoves(sq, color, pieceMoves, board);
			break;
		}

		// Convert pieceMoves to Move structs
		for (int endSq = 0; endSq < 64; ++endSq)
		{
			if (pieceMoves[endSq])
			{
				Move move;
				move.startSquare = sq;
				move.endSquare = endSq;

				const Piece movingPiece = board[move.startSquare].GetPiece();
				const Piece targetPiece = board[move.endSquare].GetPiece();

				// Castle
				if (movingPiece.GetType() == Pieces::KING && abs(move.startSquare - move.endSquare) == 2)
					move.wasCastle = true;

				// En passant
				if (movingPiece.GetType() == Pieces::PAWN && ToCol(move.startSquare) != ToCol(move.endSquare) &&
					targetPiece.GetType() == Pieces::NONE)
					move.wasEnPassant = true;

				// Capture
				if (onlyNoisy)
				{
					// Capture
					if (move.IsCapture(board)) moves.push_back(move);

					// Promotions are added earlier

					// Checks
					engine->MakeMove(move);
					// Make move switches player so this is opponent
					if (engine->InCheck(GameState::currentPlayer)) moves.push_back(move);
					engine->UndoMove();
				}
				else
					moves.push_back(move);
			}
		}
	}
}

std::vector<Move> BoardCalculator::GetAllMoves(Color color, const Square board[64], Engine* engine, bool onlyCaptures)
{
	std::vector<Move> moves;
	GetAllMoves(moves, color, board, engine, onlyCaptures);
	return moves;
}

uint8_t BoardCalculator::FindPiece(Piece piece, const Square board[64])
{
	for (int sq = 0; sq < 64; sq++)
	{
		if (board[sq].GetPiece() == piece)
			return sq;
	}
	return 64; // Not found
}

bool BoardCalculator::IsCastlingValid(bool kingside, const Square board[64])
{
	Color player = GameState::currentPlayer;
	Color enemy = Opponent(player);
	int row = IsWhite(player) ? 7 : 0;

	// 1. Check if castling rights exist
	if ((player == Color::WHITE && !GameState::whiteCastlingRights[kingside ? 1 : 0]) ||
		(player == Color::BLACK && !GameState::blackCastlingRights[kingside ? 1 : 0]))
		return false;

	// 2. King is in check
	if (IsSquareAttacked(ToIndex(row, 4), enemy, board))
		return false;

	int kingColumn = 4;
	int step = kingside ? 1 : -1;

	// 3. Squares between king and rook are not empty
	int squaresToMove = kingside ? 2 : 3;
	for (int i = 1; i <= squaresToMove; ++i)
	{
		if (!board[ToIndex(row, kingColumn + (step * i))].IsEmpty())
		{
			return false;
		}
	}

	// 4. Squares king moves through are attacked
	int squaresToMoveThrough = 2;
	for (int i = 1; i <= squaresToMoveThrough; ++i)
	{
		if (IsSquareAttacked(ToIndex(row, kingColumn + (step * i)), enemy, board))
			return false;
	}

	return true;
}


void BoardCalculator::AddKingMoves(int sq, Color color, std::array<bool, 64>& moves, const Square board[64])
{
	int row = ToRow(sq);
	int col = ToCol(sq);

	for (int i = 0; i < 8; ++i)
	{
		int target = sq + kingOffsets[i];

		if (!InBounds(target)) continue;
		if (std::abs(ToCol(target) - col) > 1) continue; // Prevent wraparound
		
		Piece targetPiece = board[target].GetPiece();
		if (targetPiece.GetType() == Pieces::NONE || targetPiece.GetColor() != color)
		{
			// Check for moving into check
			if (!IsSquareAttacked(target, Opponent(color), board))
				moves[target] = true;
		}
	}

	// Castling
	if (IsCastlingValid(true, board)) // Kingside
		moves[ToIndex(row, col + 2)] = true;
	if (IsCastlingValid(false, board)) // Queenside
		moves[ToIndex(row, col - 2)] = true;
}

void BoardCalculator::AddPawnMoves(int sq, Color color, std::array<bool, 64>& moves, const Square board[64])
{
	int row = ToRow(sq);
	int col = ToCol(sq);
	int dir = IsWhite(color) ? -1 : 1;  // White pawns move "up" (row decreases), black "down" (row increases)

	// One square forward
	int oneForward = ToIndex(row + dir, col);

	if (InBounds(oneForward) && board[oneForward].GetPiece().GetType() == Pieces::NONE)
	{
		moves[oneForward] = true;

		// Two squares forward from starting position
		if ((IsWhite(color) && row == 6) || (IsBlack(color) && row == 1))
		{
			int twoForward = ToIndex(row + 2 * dir, col);
			if (InBounds(twoForward) && board[twoForward].GetPiece().GetType() == Pieces::NONE)
				moves[twoForward] = true;
		}
	}

	// Captures
	for (int dc : {-1, 1})
	{
		int nc = col + dc;
		if (nc < 0 || nc > 7) continue;
		int target = ToIndex(row + dir, nc);
		if (!InBounds(target)) continue;

		Piece targetPieces = board[target].GetPiece();
		if (targetPieces.GetType() != Pieces::NONE && targetPieces.GetColor() != color)
			moves[target] = true;
	}

	// En passant
	if (GameState::enPassantTarget != -1)
	{
		int epSq = GameState::enPassantTarget;
		if (ToRow(epSq) == row + dir && std::abs(ToCol(epSq) - col) == 1)
			moves[epSq] = true;
	}
}

void BoardCalculator::AddKnightMoves(int sq, Color color, std::array<bool, 64>& moves, const Square board[64])
{
	int col = ToCol(sq);

	for (int i = 0; i < 8; ++i)
	{
		int target = sq + knightOffsets[i];
		if (!InBounds(target)) continue;
		if (std::abs(ToCol(target) - col) > 2) continue; // Prevent wraparound

		Piece targetPiece = board[target].GetPiece();
		if (targetPiece.GetType() == Pieces::NONE || targetPiece.GetColor() != color)
			moves[target] = true;
	}
}

void BoardCalculator::AddSlidingMoves(int sq, Color color, std::array<bool, 64>& moves, const int dirs[], int numDirs, const Square board[64])
{
	for (int i = 0; i < numDirs; ++i)
	{
		int dir = dirs[i];
		int target = sq;

		while (true)
		{
			int prevCol = ToCol(target);
			target += dir;
			if (!InBounds(target)) break;
			if (std::abs(ToCol(target) - prevCol) > 1 &&
				(dir == -1 || dir == 1 || dir == -9 || dir == 7 || dir == -7 || dir == 9)) break; // TODO: Could be wrong detection

			Piece targetPiece = board[target].GetPiece();
			if (targetPiece.GetType() == Pieces::NONE)
			{
				moves[target] = true;
				continue;
			}

			if (targetPiece.GetColor() != color)
				moves[target] = true;
			
			break; // Blocked
		}
	}
}
