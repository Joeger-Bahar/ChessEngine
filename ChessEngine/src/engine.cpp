#include "engine.hpp"

#include <iostream>
#include <sstream>
#include <cstdlib> // For console clearing
#include <cmath>
#include <array>

Engine::Engine()
{
	// Initialize the board with pieces in their starting positions
	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 8; ++j)
		{
			if (i == 1) // Black pawns
				board[i][j] = Square(Color::BLACK, Piece(Pieces::PAWN, Color::BLACK));
			else if (i == 6) // White pawns
				board[i][j] = Square(Color::WHITE, Piece(Pieces::PAWN, Color::WHITE));
			else if (i == 0 || i == 7) // Rooks, Knights, Bishops, Queen, King
			{
				Color color = (i == 0) ? Color::BLACK : Color::WHITE;
				if (j == 0 || j == 7)
					board[i][j] = Square(color, Piece(Pieces::ROOK, color));
				else if (j == 1 || j == 6)
					board[i][j] = Square(color, Piece(Pieces::KNIGHT, color));
				else if (j == 2 || j == 5)
					board[i][j] = Square(color, Piece(Pieces::BISHOP, color));
				else if (j == 3)
					board[i][j] = Square(color, Piece(Pieces::QUEEN, color));
				else if (j == 4)
					board[i][j] = Square(color, Piece(Pieces::KING, color));
			}
			else // Empty squares
				board[i][j] = Square((i + j) % 2 == 0 ? Color::WHITE : Color::BLACK);
		}
	}
}

Engine::~Engine()
{
	std::cout << "Chess engine destroyed." << std::endl;
}

void Engine::Render()
{
//	// Clear the console (platform-specific)
//#ifdef _WIN32
//	system("cls");
//#else
//	system("clear");
//#endif
//
//	// Use stringstream to build the output for efficiency (console slow)
//	std::ostringstream output;
//
//	output << "Current Player..." << (currentPlayer == Color::WHITE ? "White" : "Black") << '\n';
//	output << "FEN.............." << this->GetFEN() << '\n';
//	output << "In Check........." << ((checkStatus == Color::NONE) ? "None" : (checkStatus == Color::WHITE) ? "Black" : "White");
//	output << "\nCheckmate........" << ((!checkmate) ? "No" : (checkStatus == Color::WHITE) ? "Yes (White)" : "Yes (Black)") << '\n';
//
//
//	for (int i = 0; i < 8; ++i)
//	{
//		output << "\n+---+---+---+---+---+---+---+---+\n| ";
//		for (int j = 0; j < 8; ++j)
//		{
//			output << board[i][j].GetPiece().Render();
//		}
//		output << (8 - i); // Row number
//	}
//	output << "\n+---+---+---+---+---+---+---+---+\n";
//	output << "  a   b   c   d   e   f   g   h\n\n";
//
//	if (invalidMove)
//	{
//		output << "Invalid move! Please try again.\n";
//		invalidMove = false; // Reset invalid move status
//	}
//
//	std::cout << output.str();

	graphics.Render(board);
}

void Engine::Update()
{
	this->Render();
	if (!this->StoreMove()) return; // No move to process
	this->ProcessMove();

	this->ChangePlayers();
}

bool Engine::StoreMove()
{
	std::pair<int, int> click = graphics.GetClick();
	// Check if -1, -1, and return
	if (click.first == -1 && click.second == -1)
	{
		return false;
	}

	static std::pair<int, int> firstClick = { -1, -1 }; // Static to retain value between calls
	if (firstClick.first == -1 && firstClick.second == -1) // First click
	{
		firstClick = click;
		return false; // Wait for second click
	}

	else // Second click
	{
		// Convert to notation
		notationMove = "";
		notationMove += ('a' + firstClick.second); // Column a-h
		notationMove += ('8' - firstClick.first); // Row 1-8
		notationMove += ('a' + click.second); // Column a-h
		notationMove += ('8' - click.first); // Row 1-8
		firstClick = { -1, -1 }; // Reset for next move
		return true; // Move ready to process
	}
}

void Engine::ProcessMove()
{
	// Special commands
	if (notationMove == "resign")
	{
		checkmate = true;
		checkStatus = (currentPlayer == Color::WHITE) ? Color::BLACK : Color::WHITE; // Opponent wins
		return;
	}
	else if (notationMove == "draw")
	{
		checkmate = false;
		checkStatus = Color::NONE; // Draw
		return;
	}
	else if (notationMove == "undo")
	{
		UndoMove();
		return;
	}
	// TODO: If king or rook has moved in the game, cannot castle
	if (notationMove == "O-O-O" || notationMove == "O-O") // Castling
	{
		bool kingside = (notationMove == "O-O");

		// Check if castling is allowed
		if ((currentPlayer == Color::WHITE && !whiteCastlingRights[kingside ? 1 : 0]) ||
		    (currentPlayer == Color::BLACK && !blackCastlingRights[kingside ? 1 : 0]))
		{
			this->invalidMove = true;
			return;
		}

		int row = (currentPlayer == Color::WHITE) ? 7 : 0; // Row 7 for white, 0 for black
		int rookColumn = (kingside) ? 7 : 0;
		int kingColumn = 4; // Only used to avoid '4' as a pseudo magic number
		// +1 or -1, used for stepping to check missing pieces and to move rook and king to correct space
		int step = (kingside) ? 1 : -1; // -1 for queenside (down the columns), 1 for kingside

		// King or rook not in starting position					// Color of piece
		if (board[row][kingColumn].GetPiece() != Piece(Pieces::KING, currentPlayer) ||
		    board[row][rookColumn].GetPiece() != Piece(Pieces::ROOK, currentPlayer))
		{
			this->invalidMove = true;
			return;
		}
		// Squares between king and rook not empty
		// Only need to check 2 squares for kingside castling (f1/f8 and g1/g8)
		int squaresToCheck = kingside ? 2 : 3;
		for (int offset = 1; offset <= squaresToCheck; offset++)
		{
			if (!board[row][kingColumn + (step * offset)].IsEmpty())
			{
				this->invalidMove = true;
				return;
			}
		}

		// Perform castling
		// Move king to c1/c8 or g1/g8 (two up or down from the kings original position)
		board[row][kingColumn + (step * 2)].SetPiece(board[row][kingColumn].GetPiece());
		board[row][kingColumn].SetPiece(Piece(Pieces::NONE, Color::NONE)); // Clear original king square

		// Move rook to d1/d8 or f1/f8 (one up or down from the kings original position)
		board[row][kingColumn + step].SetPiece(board[row][rookColumn].GetPiece());
		board[row][rookColumn].SetPiece(Piece(Pieces::NONE, Color::NONE)); // Clear original rook square

		return; // Successfully castled
	}

	// Outside bounds or invalid notation
	else if (notationMove.length() != 4 || notationMove[0] < 'a' || notationMove[0] > 'h' ||
		notationMove[1] < '1' || notationMove[1] > '8' ||
		notationMove[2] < 'a' || notationMove[2] > 'h' ||
		notationMove[3] < '1' || notationMove[3] > '8')
	{
		this->invalidMove = true;
		return;
	}

	else // Convert notation to board indices and validate the move
	{

		move.startCol = notationMove[0] - 'a'; // Convert 'a'-'h' to 0-7
		move.startRow = 8 - (notationMove[1] - '0'); // Convert '1'-'8' to 7-0
		move.endCol = notationMove[2] - 'a'; // Convert 'a'-'h' to 0-7
		move.endRow = 8 - (notationMove[3] - '0'); // Convert '1'-'8' to 7-0

		Piece movingPiece = board[move.startRow][move.startCol].GetPiece();
		Piece targetPiece = board[move.endRow][move.endCol].GetPiece();

		// Check valid move conditions
		// Don't need out of bounds because 'i'-'z' and '9' aren't valid
		if (movingPiece == Pieces::NONE || movingPiece.GetColor() != currentPlayer)
		{
			this->invalidMove = true; // No piece at the starting square or wrong color
			return;
		}
		if (this->board[move.endRow][move.endCol].GetPiece().GetColor() == currentPlayer)
		{
			this->invalidMove = true; // Cannot capture own piece
			return;
		}
		if (!ValidMove(movingPiece))
		{
			this->invalidMove = true; // Invalid move for the piece
			return;
		}

		// Check if rooks or king is moved to invalidate castling rights
		if (movingPiece == Pieces::KING)
		{
			if (currentPlayer == Color::WHITE)
			{
				whiteCastlingRights[0] = false;
				whiteCastlingRights[1] = false;
			}
			else
			{
				blackCastlingRights[0] = false;
				blackCastlingRights[1] = false;
			}
		}
		else if (movingPiece == Pieces::ROOK)
		{
			if (currentPlayer == Color::WHITE)
			{
				if (move.startRow == 7 && move.startCol == 0) // a1 rook
					whiteCastlingRights[0] = false;
				else if (move.startRow == 7 && move.startCol == 7) // h1 rook
					whiteCastlingRights[1] = false;
			}
			else
			{
				if (move.startRow == 0 && move.startCol == 0) // a8 rook
					blackCastlingRights[0] = false;
				else if (move.startRow == 0 && move.startCol == 7) // h8 rook
					blackCastlingRights[1] = false;
			}
		}
		// If rook is captured, invalidate castling rights
		if (targetPiece == Pieces::ROOK)
		{
			if (currentPlayer == Color::WHITE)
			{
				if (move.endRow == 0 && move.endCol == 0) // a8 rook
					blackCastlingRights[0] = false;
				else if (move.endRow == 0 && move.endCol == 7) // h8 rook
					blackCastlingRights[1] = false;
			}
			else
			{
				if (move.endRow == 7 && move.endCol == 0) // a1 rook
					whiteCastlingRights[0] = false;
				else if (move.endRow == 7 && move.endCol == 7) // h1 rook
					whiteCastlingRights[1] = false;
			}
		}

		// Check for en passant target
		if (movingPiece == Pieces::PAWN && abs(move.endRow - move.startRow) == 2) // If pawn moved 2
		{
			enPassantTarget[0] = (move.startRow + move.endRow) / 2;
			enPassantTarget[1] = move.startCol;
		}
		else // En passant opportunity only lasts for 1 turn
		{
			enPassantTarget[0] = -1;
			enPassantTarget[1] = -1;
		}

		move.capturedPiece = targetPiece;
		// Move piece
		board[move.endRow][move.endCol].SetPiece(movingPiece);
		board[move.startRow][move.startCol].SetPiece(Pieces::NONE); // Clear the starting square

		// Add move to history
		moveHistory.push_back(move);

		return;
	}

	invalidMove = true;
}

bool Engine::ValidMove(const Piece piece)
{
	switch (piece)
	{
	case Pieces::PAWN: // Complete
		// Line 1: Pawn moves forward 1
		// Line 2: Pawn moves forward 2 from starting pos
		// Line 3: Ensures line 1 and 2 only work if square is empty
		// Line 4: Move diagonally to capture a piece
		// Line 5: Ensures line 4 only works if square is occupied
		if (piece.GetColor() == Color::WHITE)
		{
			if ((((move.startCol == move.endCol && move.endRow == move.startRow - 1) ||
				(move.startCol == move.endCol && move.startRow == 6 && move.endRow == 4))
				&& board[move.endRow][move.endCol].IsEmpty()) ||
				((abs(move.endCol - move.startCol) == 1 && move.endRow == move.startRow - 1)
					&& !board[move.endRow][move.endCol].IsEmpty())) return true;
		}
		else // Black
		{
			if ((((move.startCol == move.endCol && move.endRow == move.startRow + 1) ||
				(move.startCol == move.endCol && move.startRow == 1 && move.endRow == 3))
				&& board[move.endRow][move.endCol].IsEmpty()) ||
				((abs(move.endCol - move.startCol) == 1 && move.endRow == move.startRow + 1)
					&& !board[move.endRow][move.endCol].IsEmpty())) return true;
		}
		// En passant
		if (move.endRow == enPassantTarget[0] && move.endCol == enPassantTarget[1]) // If target square is an en passant square
		{
			// Capture the pawn
			board[move.startRow][move.endCol].SetPiece(Pieces::NONE);
			return true;
		}
		break;
	case Pieces::KNIGHT:
		// If row changes by 2 (abs) and column changes by 1 (abs), or the other way around
		if (abs(move.endRow - move.startRow) == 2 && abs(move.endCol - move.startCol) == 1) return true;
		if (abs(move.endCol - move.startCol) == 2 && abs(move.endRow - move.startRow) == 1) return true;
		break;
	case Pieces::BISHOP:
		if (abs(move.endCol - move.startCol) == abs(move.endRow - move.startRow)) goto check_pieces_in_way; // Change in up and across is the same (diagonally)
		break;
	case Pieces::ROOK:
		if (move.startCol == move.endCol || move.startRow == move.endRow) goto check_pieces_in_way; // Only move up or across
		break;
	case Pieces::QUEEN: // Copy of rook and bishop
		if ((abs(move.endCol - move.startCol) == abs(move.endRow - move.startRow)) ||
			(move.startCol == move.endCol || move.startRow == move.endRow)) goto check_pieces_in_way;
		break;
	case Pieces::KING:
		if (abs(move.endCol - move.startCol) <= 1 && abs(move.endRow - move.startRow) <= 1) return true;
		break;

	default:
		return false;
	}

	return false;

	// Uses goto. Best way I could think of
check_pieces_in_way:
	for (int i = 1; i < 8; ++i) // 7 loops because chess board has 8 squares
	{
		int rowChange = (move.endRow - move.startRow) / max(1, abs(move.endRow - move.startRow)); // Normalize row difference to 1 or -1 (up or down)
		int colChange = (move.endCol - move.startCol) / max(1, abs(move.endCol - move.startCol)); // Normalize column difference to 1 or -1 (left or right)
		int currentRow = move.startRow + i * rowChange; // Starts at move.startRow and moves towards move.endRow
		int currentCol = move.startCol + i * colChange; // Starts at move.startCol and moves towards move.endCol

		if (currentRow == move.endRow && currentCol == move.endCol) return true; // Reached the end square
		if (!board[currentRow][currentCol].IsEmpty()) // Piece in the way
		{
			// Set move.endRow and move.endCol to the current square if a piece of the opponent is found
			// Or the previous square if a piece of the same color is found
			if (board[currentRow][currentCol].GetPiece().GetColor() != piece.GetColor())
			{
				move.endRow = currentRow;
				move.endCol = currentCol;
			}
			else
			{
				move.endRow = currentRow - rowChange; // Set to the previous square
				move.endCol = currentCol - colChange; // Set to the previous square

				// Check if the previous square is the start square (wasted move)
				if (move.endRow == move.startRow && move.endCol == move.startCol)
					return false;
			}

			return true;
		}
	}
}

//std::vector<uint8_t> Engine::GetAttackedSquares(Color color)
//{
//	std::vector<uint8_t> attackedSquares;
//	for (int i = 0; i < 8; ++i)
//	{
//		for (int j = 0; j < 8; ++j)
//		{
//			Piece piece = board[i][j].GetPiece();
//			if (piece == Pieces::NONE || piece.GetColor() != color) continue; // Skip empty squares and pieces of the wrong color
//
//			// For each piece of the given color, check all squares on the board
//			for (int r = 0; r < 8; ++r)
//			{
//				for (int c = 0; c < 8; ++c)
//				{
//					if (i == r && j == c) continue; // Skip the piece's own square
//					Move testMove;
//					testMove.startRow = i;
//					testMove.startCol = j;
//					testMove.endRow = r;
//					testMove.endCol = c;
//					// Temporarily set the move to test if it's valid
//					move = testMove;
//					if (ValidMove(piece))
//					{
//						uint8_t squareIndex = r * 8 + c; // Convert (row, col) to single index (0-63)
//						if (std::find(attackedSquares.begin(), attackedSquares.end(), squareIndex) == attackedSquares.end())
//						{
//							attackedSquares.push_back(squareIndex); // Add if not already in the list
//						}
//					}
//				}
//			}
//		}
//	}
//
//	return attackedSquares;
//}

// This looks messy and annoying, and it takes up a lot of space, but it's about 5x faster
// than a brute force (above)
std::vector<uint8_t> Engine::GetAttackedSquares(Color color)
{
	// Rook directions (up, down, left, right)
	const std::vector<std::pair<int, int>> rookDirs = {
		{-1, 0}, {1, 0}, {0, -1}, {0, 1}
	};

	// Bishop directions (diagonals)
	const std::vector<std::pair<int, int>> bishopDirs = {
		{-1, -1}, {-1, 1}, {1, -1}, {1, 1}
	};

	// Queen = rook + bishop
	const std::vector<std::pair<int, int>> queenDirs = {
		{-1, 0}, {1, 0}, {0, -1}, {0, 1},
		{-1, -1}, {-1, 1}, {1, -1}, {1, 1}
	};

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
				AddSlidingAttacks(i, j, attacked, bishopDirs);
				break;
			case Pieces::ROOK:
				AddSlidingAttacks(i, j, attacked, rookDirs);
				break;
			case Pieces::QUEEN:
				AddSlidingAttacks(i, j, attacked, queenDirs);
				break;
			case Pieces::KING:
				AddKingAttacks(i, j, attacked);
				break;
			}
		}
	}

	std::vector<uint8_t> attackedSquares;
	for (int idx = 0; idx < 64; ++idx)
		if (attacked[idx]) attackedSquares.push_back(idx);

	return attackedSquares;
}

std::string Engine::GetFEN() const
{
	std::string fen;
	int emptyCount = 0;
	for (int i = 0; i < 8; ++i)
	{
		for (int j = 0; j < 8; ++j)
		{
			Piece piece = board[i][j].GetPiece();
			if (piece == Pieces::NONE)
			{
				emptyCount++;
			}
			else
			{
				if (emptyCount > 0)
				{
					fen += std::to_string(emptyCount);
					emptyCount = 0;
				}
				char pieceChar;
				switch (piece)
				{
				case Pieces::KING:   pieceChar = 'k'; break;
				case Pieces::QUEEN:  pieceChar = 'q'; break;
				case Pieces::ROOK:   pieceChar = 'r'; break;
				case Pieces::BISHOP: pieceChar = 'b'; break;
				case Pieces::KNIGHT: pieceChar = 'n'; break;
				case Pieces::PAWN:   pieceChar = 'p'; break;
				default:             pieceChar = '?'; break; // Should not happen
				}
				if (piece.GetColor() == Color::WHITE)
					pieceChar = toupper(pieceChar);
				fen += pieceChar;
			}
		}
		if (emptyCount > 0)
		{
			fen += std::to_string(emptyCount);
			emptyCount = 0;
		}
		if (i < 7)
			fen += '/';
	}

	fen += (currentPlayer == Color::WHITE) ? " w " : " b ";
	// Castling rights
	std::string castling;
	if (whiteCastlingRights[1]) castling += 'K';
	if (whiteCastlingRights[0]) castling += 'Q';
	if (blackCastlingRights[1]) castling += 'k';
	if (blackCastlingRights[0]) castling += 'q';
	fen += (castling.empty() ? "-" : castling);
	// En passant target square
	if (enPassantTarget[0] != -1 && enPassantTarget[1] != -1)
	{
		char file = 'a' + enPassantTarget[1];
		char rank = '8' - enPassantTarget[0];
		fen += " ";
		fen += file;
		fen += rank;
	}
	else
	{
		fen += " -";
	}
	fen += " 0 "; // Halfmove clock
	fen += std::to_string(moveHistory.size() / 2 + 1); // Fullmove number

	return fen;
}

void Engine::UndoMove()
{
	if (moveHistory.empty())
	{
		this->invalidMove = true; // No moves to undo
		return;
	}
	// Undo last move
	move = moveHistory.back();
	moveHistory.pop_back();
	Piece movingPiece = board[move.endRow][move.endCol].GetPiece();
	board[move.startRow][move.startCol].SetPiece(movingPiece);
	board[move.endRow][move.endCol].SetPiece(move.capturedPiece); // Restore captured piece, if any
	// Don't need to change players because this counts as a "turn"
}

void Engine::AddKnightAttacks(int row, int col, std::array<bool, 64>& attacked)
{
	static const int dr[8] = { -2, -2, -1, -1, 1, 1, 2, 2 };
	static const int dc[8] = { -1, 1, -2, 2, -2, 2, -1, 1 };

	for (int k = 0; k < 8; ++k) {
		int nr = row + dr[k], nc = col + dc[k];
		if (0 <= nr && nr < 8 && 0 <= nc && nc < 8) 
			attacked[nr * 8 + nc] = true;
	}
}

void Engine::AddSlidingAttacks(int row, int col, std::array<bool, 64>& attacked, const std::vector<std::pair<int, int>>& dirs)
{
	for (const auto &[dr, dc] : dirs) {
		int nr = row + dr, nc = col + dc;
		while (0 <= nr && nr < 8 && 0 <= nc && nc < 8) {
			attacked[nr * 8 + nc] = true;
			if (board[nr][nc].GetPiece() != Pieces::NONE) break; // Blocked
			nr += dr; nc += dc;
		}
	}
}

void Engine::AddPawnAttacks(int row, int col, Color color, std::array<bool, 64>& attacked)
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

void Engine::AddKingAttacks(int row, int col, std::array<bool, 64>& attacked)
{
	static const int dr[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
	static const int dc[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };

	for (int k = 0; k < 8; ++k)
	{
		int nr = row + dr[k];
		int nc = col + dc[k];

		if (0 <= nr && nr < 8 && 0 <= nc && nc < 8)
			attacked[nr * 8 + nc] = true;
	}
}
