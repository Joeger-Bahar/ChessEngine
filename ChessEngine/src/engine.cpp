#include "engine.hpp"

#include <iostream>
#include <cstdlib> // For console clearing

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
	// Clear the console (platform-specific)
#ifdef _WIN32
	system("cls");
#else
	system("clear");
#endif

	if (invalidMove)
	{
		std::cout << "Invalid move! Please try again.\n\n";
		invalidMove = false; // Reset invalid move status
	}
	std::cout << "Current Player..." << (currentPlayer == Color::WHITE ? "White" : "Black") << "\n";
	std::cout << "FEN.............." << this->GetFEN() << "\n"; // Placeholder function
	std::cout << "Check............" << ((checkStatus == Color::NONE) ? "None" : (checkStatus == Color::WHITE) ? "White" : "Black");
	std::cout << "\nCheckmate........" << ((!checkmate) ? "No" : (checkStatus == Color::WHITE) ? "Yes (White)" : "Yes (Black)") << "\n";


	for (int i = 0; i < 8; ++i)
	{
		std::cout << "\n+---+---+---+---+---+---+---+---+\n| ";
		for (int j = 0; j < 8; ++j)
		{
			board[i][j].GetPiece().Render();
		}
		std::cout << (8 - i); // Row number
	}
	std::cout << "\n+---+---+---+---+---+---+---+---+\n";
	std::cout << "  a   b   c   d   e   f   g   h\n";
}

void Engine::StoreMove()
{
	std::cout << (currentPlayer == Color::WHITE ? "White" : "Black") << " move: ";
	std::cin >> notationMove;
}

bool Engine::ProcessMove()
{
	if (notationMove == "O-O-O")
	{
		// Queen-side castling
	}
	else if (notationMove == "O-O")
	{
		// King-side castling
	}
	else if (notationMove.length() != 4 || notationMove[0] < 'a' || notationMove[0] > 'h' ||
		notationMove[1] < '1' || notationMove[1] > '8' ||
		notationMove[2] < 'a' || notationMove[2] > 'h' ||
		notationMove[3] < '1' || notationMove[3] > '8')
	{
		this->invalidMove = true;
		return false;
	}
	else
	{
		int startColumn = notationMove[0] - 'a'; // Convert 'a'-'h' to 0-7
		int startRow = 8 - (notationMove[1] - '0'); // Convert '1'-'8' to 7-0
		int endColumn = notationMove[2] - 'a'; // Convert 'a'-'h' to 0-7
		int endRow = 8 - (notationMove[3] - '0'); // Convert '1'-'8' to 7-0

		this->board[endRow][endColumn].SetPiece(this->board[startRow][startColumn].GetPiece());
		this->board[startRow][startColumn].SetPiece(Pieces::NONE); // Clear the starting square

		return true;
	}
	return false;
}

const char* Engine::GetFEN() const
{
	return "rnbqkbnr / pppppppp / 8 / 8 / 8 / 8 / PPPPPPPP / RNBQKBNR w KQkq - 0 1";
}
