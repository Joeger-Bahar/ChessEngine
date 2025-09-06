#include "engine.hpp"

#include <iostream>
#include <cmath>

// Make GameState variables not need  prefix
using namespace GameState;

Engine::Engine()
	: firstClick({ -1, -1 }), graphics()
{
	LoadPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

Engine::Engine(std::string fen)
	: firstClick({ -1, -1 }), graphics()
{
		LoadPosition(fen);
}

Engine::~Engine() {}

void Engine::LoadPosition(std::string fen)
{
	// Load the board from a FEN string
	// Loop through the FEN string and set up the board accordingly
	// For simplicity, this implementation assumes a valid FEN string
	int row = 0, col = 0;
	int gameStateIndex = 0; // To track where the game state info starts
	for (char c : fen)
	{
		if (c == ' ') break;
		gameStateIndex++;
		if (c == '/') { row++; col = 0; continue; } // Next row
		if (isdigit(c)) { col += c - '0'; continue; } // Empty squares
		Color color = isupper(c) ? Color::WHITE : Color::BLACK;
		Pieces pieceType;
		switch (tolower(c))
		{
		case 'p': pieceType = Pieces::PAWN; break;
		case 'r': pieceType = Pieces::ROOK; break;
		case 'n': pieceType = Pieces::KNIGHT; break;
		case 'b': pieceType = Pieces::BISHOP; break;
		case 'q': pieceType = Pieces::QUEEN; break;
		case 'k': pieceType = Pieces::KING;
			if (color == Color::WHITE)
				whiteKingPos = { row, col };
			else
				blackKingPos = { row, col };
			break;
		default: pieceType = Pieces::NONE; break;
		}
		board[row][col] = Square(color, Piece(pieceType, color));
		col++;
	}

	// Parse game state info
	// After string sanitization, w KQkq - 0 1 turns into w-01
	std::string fenGameState = fen.substr(gameStateIndex + 1);
	if (fenGameState.length() < 4) return; // Invalid game state info
	// Remove spaces
	fenGameState.erase(remove(fenGameState.begin(), fenGameState.end(), ' '), fenGameState.end());
	// Active color
	currentPlayer = (fenGameState[0] == 'w') ? Color::WHITE : Color::BLACK;
	// Castling rights
	whiteCastlingRights[0] = (fenGameState.find('Q') != std::string::npos);
	whiteCastlingRights[1] = (fenGameState.find('K') != std::string::npos);
	blackCastlingRights[0] = (fenGameState.find('q') != std::string::npos);
	blackCastlingRights[1] = (fenGameState.find('k') != std::string::npos);
	// Remove all castling notation from string
	fenGameState.erase(remove_if(fenGameState.begin(), fenGameState.end(),
		[](char ch) { return ch == 'K' || ch == 'Q' || ch == 'k' || ch == 'q'; }), fenGameState.end());
	
	// En passant target, halfmoves and fullmoves
	moveHistory.clear();
	if (fenGameState[1] != '-') // If there is an en passant target
	{
		enPassantTarget[1] = fenGameState[1] - 'a'; // Column
		enPassantTarget[0] = 8 - (fenGameState[2] - '0'); // Row

		halfmoves = std::stoi(fenGameState.substr(3, 1));

		int fullmoves = std::stoi(fenGameState.substr(4));
		//for (int i = 0; i < fullmoves * 2; i++)
		//	moveHistory.push_back(Move());
	}
	else
	{
		enPassantTarget[0] = -1;
		enPassantTarget[1] = -1;

		halfmoves = std::stoi(fenGameState.substr(2, 1));

		int fullmoves = std::stoi(fenGameState.substr(3));
		
		//for (int i = 0; i < fullmoves * 2; i++)
		//	moveHistory.push_back(Move());
	}
}

void Engine::Render()
{
	// If first click is valid highlight the square
	if (firstClick.first != -1 && firstClick.second != -1)
	{
		graphics.QueueRender([=]() { graphics.DrawSquareHighlight(firstClick.first, firstClick.second, { 0, 255, 0, 100 }); });

		// Render the valid moves for the selected piece
		std::vector<uint8_t> validMoves = BoardCalculator::GetValidMoves(firstClick.first, firstClick.second, board);
		for (uint8_t move : validMoves)
		{
			int row = move / 8;
			int col = move % 8;
			graphics.QueueRender([=]() { graphics.DrawSquareHighlight(row, col, { 0, 0, 255, 100 }); }); // Blue highlight
		}
	}

	graphics.Render(board);
}

void Engine::ProcessWindowInput()
{
	std::pair<int, int> click = graphics.GetInputs(); // Returns the clicked row/col or a special code

	if (click.first == -1 && click.second == -1)
		return;

	if (click.first == -2 && click.second == -2)
	{
		notationMove = "undo";
	}
}

void Engine::Update()
{
	while (true) // Keep looping until we get a valid move
	{
		Render();

		if (IsOver())
			return; // Game is finished

		if (!StoreMove())
			continue; // No full move was entered, keep asking

		ProcessMove();

		if (invalidMove)
		{
			invalidMove = false;
			continue; // Move was structurally invalid
		}

		// Try to make the move
		MakeMove();

		CheckKingInCheck();
		if (InCheck((currentPlayer == Color::WHITE) ? Color::BLACK : Color::WHITE))
		{
			// Illegal: move leaves player in check
			UndoMove();
			continue; // Ask for input again
		}

		break; // Move was valid, exit loop
	}

	// Update game state after move
	CheckCheckmate();
}

bool Engine::StoreMove()
{
	std::pair<int, int> click = graphics.GetInputs();
	// -2 -2 for undo
	if (click.first == -2 && click.second == -2)
	{
		UndoMove();
		if (!invalidMove)
			ChangePlayers();
		return false;
	}
	// Check if -1, -1, and return
	if (click.first == -1 && click.second == -1)
	{
		return false;
	}

	if (firstClick.first == -1 && firstClick.second == -1) // First click
	{
		// Check if the first click was valid
		if (board[click.first][click.second].IsEmpty() ||
			board[click.first][click.second].GetPiece().GetColor() != currentPlayer)
		{
			std::cout << "Invalid first click\n";
			return false; // Invalid first click, wait for another
		}

		firstClick = click;
		return false; // Wait for second click
	}
	
	else // Second click
	{
		// If first and second click are the same, reset firstClick
		if (firstClick == click)
		{
			firstClick = { -1, -1 };
			return false; // Wait for new first click
		}
		// If click is on a piece of the same color, change firstClick
		if (!board[click.first][click.second].IsEmpty() &&
			board[click.first][click.second].GetPiece().GetColor() == currentPlayer)
		{
			firstClick = click;
			return false; // Wait for second click
		}

		// Convert to notation
		// If move is invalid it gets caught and handled in ProcessMove

		// Check first for castling
		if (board[firstClick.first][firstClick.second].GetPiece().GetType() == Pieces::KING &&
			abs(firstClick.second - click.second) == 2 && firstClick.first == click.first) // Same row, 2 columns apart
		{
			if (click.second == 6) // Kingside
				notationMove = "O-O";
			else if (click.second == 2) // Queenside
				notationMove = "O-O-O";
			else
			{
				firstClick = { -1, -1 }; // Reset for next move
				return false; // Invalid castling move, wait for new input
			}
			if (!BoardCalculator::IsCastlingValid(notationMove == "O-O", board))
			{
				firstClick = { -1, -1 }; // Reset for next move
				invalidMove = true;
				return false; // Move ready to process
			}

			firstClick = { -1, -1 }; // Reset for next move
			return true; // Move ready to process
		}

		// Normal move
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
	// Castling
	if (notationMove == "O-O-O" || notationMove == "O-O")
	{
		// Set start square to king and end square to 2 away
		move.startRow = (currentPlayer == Color::WHITE) ? 7 : 0;
		move.startCol = 4;
		move.endRow = move.startRow;
		move.endCol = (notationMove == "O-O") ? 6 : 2;
		return;
	}

	if (notationMove.length() != 4 || notationMove[0] < 'a' || notationMove[0] > 'h' ||
		notationMove[1] < '1' || notationMove[1] > '8' ||
		notationMove[2] < 'a' || notationMove[2] > 'h' ||
		notationMove[3] < '1' || notationMove[3] > '8')
	{
		invalidMove = true;
		return;
	}

	move.startCol = notationMove[0] - 'a'; // Convert 'a'-'h' to 0-7
	move.startRow = 8 - (notationMove[1] - '0'); // Convert '1'-'8' to 7-0
	move.endCol = notationMove[2] - 'a';
	move.endRow = 8 - (notationMove[3] - '0');

	Piece movingPiece = board[move.startRow][move.startCol].GetPiece();

	// Check valid move conditions
	// Don't need out of bounds because 'i'-'z' and '9' aren't valid
	if (!ValidMove(movingPiece)) // Invalid move for the piece
	{
		invalidMove = true;
		return;
	}
}

void Engine::MakeMove()
{
	// 1. Save current info for undo
	AppendUndoList();

	// 1. Move piece
	Piece movingPiece = board[move.startRow][move.startCol].GetPiece();
	Piece targetPiece = board[move.endRow][move.endCol].GetPiece();

	board[move.endRow][move.endCol].SetPiece(movingPiece);
	board[move.startRow][move.startCol].SetPiece(Piece(Pieces::NONE, Color::NONE));
	move.capturedPiece = targetPiece;
	// Update king position if needed
	if (movingPiece.GetType() == Pieces::KING)
	{
		if (currentPlayer == Color::WHITE)
			whiteKingPos = { move.endRow, move.endCol };
		else
			blackKingPos = { move.endRow, move.endCol };
	}

	// 3. Castling rights
	UpdateCastlingRights(movingPiece, targetPiece);
	// Handle castling
	if (movingPiece.GetType() == Pieces::KING && abs(move.startCol - move.endCol) == 2)
	{
		bool kingside = (move.endCol == 6);
		int row = (currentPlayer == Color::WHITE) ? 7 : 0;
		int rookStartCol = kingside ? 7 : 0;
		int rookEndCol = kingside ? 5 : 3;
		// Move rook
		board[row][rookEndCol].SetPiece(board[row][rookStartCol].GetPiece());
		board[row][rookStartCol].SetPiece(Piece(Pieces::NONE, Color::NONE));
		// Update king position
		if (currentPlayer == Color::WHITE)
			whiteKingPos = { row, move.endCol };
		else
			blackKingPos = { row, move.endCol };
	}

	// 4. En passant sqaure/capture
	UpdateEnPassantSquare(movingPiece);
	// Check for capture
	if (movingPiece.GetType() == Pieces::PAWN && targetPiece.GetType() == Pieces::NONE && move.startCol != move.endCol)
	{
		int pawnRow = (currentPlayer == Color::WHITE) ? move.endRow + 1 : move.endRow - 1;
		move.capturedPiece = board[pawnRow][move.endCol].GetPiece();
		board[pawnRow][move.endCol].SetPiece(Piece(Pieces::NONE, Color::NONE));
	}

	// 5. Pawn promotion
	if (movingPiece.GetType() == Pieces::PAWN && (move.endRow == 0 || move.endRow == 7))
	{
		board[move.endRow][move.endCol].SetPiece(Piece(Pieces::QUEEN, currentPlayer)); // Auto promote to queen
		move.promotion = Pieces::QUEEN;
	}

	// 6. Halfmove clock
	if (movingPiece.GetType() == Pieces::PAWN || targetPiece.GetType() != Pieces::NONE)
		halfmoves = 0;
	else
		halfmoves++;
	if (halfmoves >= 100) // 50-move rule
	{
		draw = true;
		std::cout << "Game is a draw by the 50-move rule!\n";
		return;
	}

	// 7. Save move
	moveHistory.push_back(move);
	// 2. Change side to move
	ChangePlayers();
}

bool Engine::HandleSpecialNotation()
{
	if (notationMove == "resign")
	{
		checkmate = true;
		checkStatus = (currentPlayer == Color::WHITE) ? Color::BLACK : Color::WHITE; // Opponent wins
		return true;
	}
	else if (notationMove == "draw")
	{
		checkmate = false;
		checkStatus = Color::NONE; // Draw
		return true;
	}
	else if (notationMove == "undo")
	{
		UndoMove();
		if (!invalidMove) // Only change players if undo was successful
			ChangePlayers();
	
		return true;
	}

	return false;
}

//void Engine::Benchmark()
//{
//	constexpr int iterations = 1'000'000;
//
//	auto start = std::chrono::high_resolution_clock::now();
//	for (int i = 0; i < iterations; ++i) {
//		auto result = GetAttackedSquares(Color::WHITE);
//	}
//	auto end = std::chrono::high_resolution_clock::now();
//
//	auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
//	std::cout << "Time per call: " << (ns / iterations) << " ns\n";
//}

bool Engine::ValidMove(const Piece piece)
{
	// Check if the move is valid for the given piece type
	std::vector<uint8_t> validMoves = BoardCalculator::GetValidMoves(move.startRow, move.startCol, board);
	uint8_t targetSquare = move.endRow * 8 + move.endCol;

	for (uint8_t validMove : validMoves)
		if (validMove == targetSquare)
			return true;

	return false;
}

bool Engine::IsOver()
{
	return checkmate || draw;
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
			if (piece.GetType() == Pieces::NONE)
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
				switch (piece.GetType())
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
		fen += " - ";
	}
	fen += halfmoves / 2; // Halfmove clock
	fen += " ";
	fen += std::to_string(moveHistory.size() / 2 + 1); // Fullmove number

	return fen;
}

void Engine::UndoMove()
{
	std::cout << "Undoing move\n";
	if (moveHistory.empty())
	{
		invalidMove = true; // No moves to undo
		return;
	}

	// Undo last move
	move = moveHistory.back();
	moveHistory.pop_back();
	Piece movingPiece = board[move.endRow][move.endCol].GetPiece();
	if (move.promotion != Pieces::NONE) // If it was a promotion, revert to pawn
		movingPiece = Piece(Pieces::PAWN, movingPiece.GetColor());

	board[move.startRow][move.startCol].SetPiece(movingPiece);
	board[move.endRow][move.endCol].SetPiece(move.capturedPiece); // Restore captured piece, if any

	if (movingPiece.GetType() == Pieces::KING)
	{
		if (movingPiece.GetColor() == Color::WHITE)
			whiteKingPos = { move.startRow, move.startCol };
		else if (movingPiece.GetColor() == Color::BLACK)
			blackKingPos = { move.startRow, move.startCol };
	}
}

void Engine::CheckKingInCheck()
{
	if (BoardCalculator::IsSquareAttacked(whiteKingPos.first, whiteKingPos.second, Color::BLACK, board))
		checkStatus = Color::WHITE;
	else if (BoardCalculator::IsSquareAttacked(blackKingPos.first, blackKingPos.second, Color::WHITE, board))
		checkStatus = Color::BLACK;
	else
		checkStatus = Color::NONE;
}

void Engine::UpdateCastlingRights(Piece movingPiece, Piece targetPiece)
{
	// If neither side can castle, no need to check
	if (!whiteCastlingRights[0] && !whiteCastlingRights[1] &&
		!blackCastlingRights[0] && !blackCastlingRights[1])
		return;

	if (movingPiece.GetType() == Pieces::KING)
	{
		if (currentPlayer == Color::WHITE)
		{
			whiteKingPos = { move.endRow, move.endCol };
			whiteCastlingRights[0] = false;
			whiteCastlingRights[1] = false;
		}
		else
		{
			blackKingPos = { move.endRow, move.endCol };
			blackCastlingRights[0] = false;
			blackCastlingRights[1] = false;
		}
	}
	else if (movingPiece.GetType() == Pieces::ROOK)
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
	if (targetPiece.GetType() == Pieces::ROOK)
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
}

void Engine::UpdateEnPassantSquare(Piece movingPiece)
{
	if (movingPiece.GetType() == Pieces::PAWN && abs(move.endRow - move.startRow) == 2) // If pawn moved 2
	{
		enPassantTarget[0] = (move.startRow + move.endRow) / 2;
		enPassantTarget[1] = move.startCol;
	}
	else // En passant opportunity only lasts for 1 turn
	{
		enPassantTarget[0] = -1;
		enPassantTarget[1] = -1;
	}
}

void Engine::AppendUndoList()
{
}

void Engine::CheckCheckmate()
{
	bool hasMoves = false;
	
	for (int r = 0; r < 8; r++)
	{
		for (int c = 0; c < 8; c++)
		{
			Piece p = board[r][c].GetPiece();
			if (p.GetType() == Pieces::NONE || p.GetColor() != currentPlayer) // Empty or opponent's piece
				continue;

			std::vector<uint8_t> moves = BoardCalculator::GetValidMoves(r, c, board);
			if (!moves.empty())
			{
				hasMoves = true;
				break;
			}
		}

		if (hasMoves) break;
	}
	if (!hasMoves)
	{
		if (checkStatus == Color::NONE) // Stalemate
		{
			draw = true;
			std::cout << "Game is a draw by stalemate!\n";
			return;
		}
		checkmate = true;
		// Current player in checkmate
		std::cout << ((currentPlayer == Color::WHITE) ? "Black" : "White") << " wins by checkmate!\n";
	}
}
