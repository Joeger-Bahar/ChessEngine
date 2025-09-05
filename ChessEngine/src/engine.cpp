#include "engine.hpp"

#include <iostream>
#include <cmath>

Engine::Engine()
	: firstClick({ -1, -1 }), graphics()
{
	InitializeBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

Engine::Engine(std::string fen)
	: firstClick({ -1, -1 }), graphics()
{
		InitializeBoard(fen);
}

Engine::~Engine()
{
	std::cout << "Chess engine destroyed." << std::endl;
}

void Engine::InitializeBoard(std::string fen)
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
	std::string gameState = fen.substr(gameStateIndex + 1);
	if (gameState.length() < 4) return; // Invalid game state info
	// Remove spaces
	gameState.erase(remove(gameState.begin(), gameState.end(), ' '), gameState.end());
	// Active color
	currentPlayer = (gameState[0] == 'w') ? Color::WHITE : Color::BLACK;
	// Castling rights
	whiteCastlingRights[0] = (gameState.find('Q') != std::string::npos);
	whiteCastlingRights[1] = (gameState.find('K') != std::string::npos);
	blackCastlingRights[0] = (gameState.find('q') != std::string::npos);
	blackCastlingRights[1] = (gameState.find('k') != std::string::npos);
	// Remove all castling notation from string
	gameState.erase(remove_if(gameState.begin(), gameState.end(),
		[](char ch) { return ch == 'K' || ch == 'Q' || ch == 'k' || ch == 'q'; }), gameState.end());
	std::cout << "Game state info: " << gameState << std::endl;
	// En passant target, halfmoves and fullmoves
	moveHistory.clear();
	if (gameState[1] != '-') // If there is an en passant target
	{
		enPassantTarget[1] = gameState[1] - 'a'; // Column
		enPassantTarget[0] = 8 - (gameState[2] - '0'); // Row

		halfmoves = std::stoi(gameState.substr(3, 1));

		int fullmoves = std::stoi(gameState.substr(4));
		for (int i = 0; i < fullmoves * 2; i++)
			moveHistory.push_back(Move());
	}
	else
	{
		enPassantTarget[0] = -1;
		enPassantTarget[1] = -1;

		halfmoves = std::stoi(gameState.substr(2, 1));

		int fullmoves = std::stoi(gameState.substr(3));
		for (int i = 0; i < fullmoves * 2; i++)
			moveHistory.push_back(Move());
	}
}

void Engine::Render()
{
	// If first click is valid highlight the square
	if (firstClick.first != -1 && firstClick.second != -1)
	{
		//graphics.DrawSquareHighlight(firstClick.first, firstClick.second, { 0, 255, 0, 100 }); // Green highlight
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

void Engine::Update()
{
	while (1) // Loop until a valid move is made
	{
		this->Render();

		if (this->IsOver()) return; // Game over
		// TODO: Store move with the result from a processinput function
		if (!this->StoreMove()) continue; // No move to process
		this->ProcessMove();
		if (this->invalidMove) { this->invalidMove = false; continue; } // Invalid move, ask for input again
		break;
	}

	this->ChangePlayers();
}

bool Engine::StoreMove()
{
	std::pair<int, int> click = graphics.GetClick();
	// -2 -2 for undo
	if (click.first == -2 && click.second == -2)
	{
		UndoMove();
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

		// Convert to notation
		// If move is invalid it gets caught and handled

		// Check first for castling
		if (board[firstClick.first][firstClick.second].GetPiece() == Pieces::KING &&
			abs(firstClick.second - click.second) == 2 && firstClick.first == click.first)
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
	// Special commands
	if (HandleSpecialNotation()) return;
	
	if (notationMove == "O-O-O" || notationMove == "O-O")
	{
		HandleCastling();
		if (this->invalidMove) return; // Invalid castling move

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

	else
	{
		move.startCol = notationMove[0] - 'a'; // Convert 'a'-'h' to 0-7
		move.startRow = 8 - (notationMove[1] - '0'); // Convert '1'-'8' to 7-0
		move.endCol = notationMove[2] - 'a';
		move.endRow = 8 - (notationMove[3] - '0');

		Piece movingPiece = board[move.startRow][move.startCol].GetPiece();
		Piece targetPiece = board[move.endRow][move.endCol].GetPiece();

		// Check valid move conditions
		// Don't need out of bounds because 'i'-'z' and '9' aren't valid
		if ((movingPiece == Pieces::NONE || movingPiece.GetColor() != currentPlayer)       // No piece at the starting square or wrong color
		 || (this->board[move.endRow][move.endCol].GetPiece().GetColor() == currentPlayer) // Cannot capture own piece
		 || (!ValidMove(movingPiece)))													   // Invalid move for the piece
		{
			this->invalidMove = true; 
			return;
		}

		// Make the move
		move.capturedPiece = targetPiece;
		board[move.endRow][move.endCol].SetPiece(movingPiece);
		board[move.startRow][move.startCol].SetPiece(Piece(Pieces::NONE, Color::NONE));
		moveHistory.push_back(move);

		// Check if king is in check after move
		CheckKingInCheck();
		if (checkStatus == currentPlayer) // If the current player's king is in check after the move
		{
			UndoMove();
			this->invalidMove = true;
			return;
		}

		// Check if rooks or king is moved to invalidate castling rights
		if (movingPiece == Pieces::KING)
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

		// TODO: Make this a function
		// Update halfmove clock
		if (movingPiece == Pieces::PAWN || targetPiece != Pieces::NONE)
			halfmoves = 0;
		else
			halfmoves++;
		if (halfmoves >= 100) // 50-move rule
		{
			draw = true;
			std::cout << "Game is a draw by the 50-move rule!\n";
			return;
		}
		// Check if opponent has any valid moves for checkmate + stalemate
		bool opponentHasMoves = false;
		Color opponent = (currentPlayer == Color::WHITE) ? Color::BLACK : Color::WHITE;
		for (int r = 0; r < 8; r++)
		{
			for (int c = 0; c < 8; c++)
			{
				Piece p = board[r][c].GetPiece();
				if (p != Pieces::NONE && p.GetColor() == opponent)
				{
					std::vector<uint8_t> moves = BoardCalculator::GetValidMoves(r, c, board);
					if (!moves.empty())
					{
						opponentHasMoves = true;
						break;
					}
				}
			}
			if (opponentHasMoves) break;
		}
		if (!opponentHasMoves)
		{
			if (checkStatus == Color::NONE) // Stalemate
			{
				draw = true;
				std::cout << "Game is a draw by stalemate!\n";
				return;
			}
			checkmate = true;
			std::cout << ((currentPlayer == Color::WHITE) ? "White" : "Black") << " wins by checkmate!\n";
		}

		return;
	}

	invalidMove = true;
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
		ChangePlayers();
		return true;
	}

	return false;
}

void Engine::HandleCastling()
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

	// Update king position
	if (currentPlayer == Color::WHITE)
		whiteKingPos = { row, kingColumn + (step * 2) };
	else
		blackKingPos = { row, kingColumn + (step * 2) };

	// Move rook to d1/d8 or f1/f8 (one up or down from the kings original position)
	board[row][kingColumn + step].SetPiece(board[row][rookColumn].GetPiece());
	board[row][rookColumn].SetPiece(Piece(Pieces::NONE, Color::NONE)); // Clear original rook square
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
