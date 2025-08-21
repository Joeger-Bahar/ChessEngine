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
	std::cout << "  a   b   c   d   e   f   g   h\n\n";

	if (invalidMove)
	{
		std::cout << "Invalid move! Please try again.\n";
		invalidMove = false; // Reset invalid move status
	}
}

void Engine::RunTurn()
{
	do
	{
		this->Render();
		this->StoreMove();
		this->ProcessMove();
	} while (invalidMove);

	this->ChangePlayers();

}

void Engine::StoreMove()
{
	std::cout << (currentPlayer == Color::WHITE ? "White" : "Black") << " move: ";
	std::cin >> notationMove;
}

void Engine::ProcessMove()
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
		return;
	}
	else
	{
		int startColumn = notationMove[0] - 'a'; // Convert 'a'-'h' to 0-7
		int startRow = 8 - (notationMove[1] - '0'); // Convert '1'-'8' to 7-0
		int endColumn = notationMove[2] - 'a'; // Convert 'a'-'h' to 0-7
		int endRow = 8 - (notationMove[3] - '0'); // Convert '1'-'8' to 7-0


		Piece movingPiece = this->board[startRow][startColumn].GetPiece();
		// Check valid move conditions
		// Don't need out of bounds because 'i'-'z' and '9' aren't valid
		if (movingPiece == Pieces::NONE || movingPiece.GetColor() != currentPlayer)
		{
			this->invalidMove = true;
			return;
		}
		if (this->board[endRow][endColumn].GetPiece().GetColor() == currentPlayer)
		{
			this->invalidMove = true; // Cannot capture own piece
			return;
		}
		if (!movingPiece.ValidMove(startRow, startColumn, endRow, endColumn, board))
		{
			this->invalidMove = true; // Invalid move for the piece
			return;
		}

		this->board[endRow][endColumn].SetPiece(this->board[startRow][startColumn].GetPiece());
		this->board[startRow][startColumn].SetPiece(Pieces::NONE); // Clear the starting square

		return;
	}

	invalidMove = true;
}

const char* Engine::GetFEN() const
{
	return "rnbqkbnr / pppppppp / 8 / 8 / 8 / 8 / PPPPPPPP / RNBQKBNR w KQkq - 0 1";
}
