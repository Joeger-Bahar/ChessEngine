#include "engine.hpp"
#include "bot/bot.hpp"

#include <iostream>
#include <cmath>
#include <Windows.h>

// Make GameState variables not need  prefix
using namespace GameState;

Engine::Engine()
	: firstClick({ -1, -1 }), graphics()
{
	LoadPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	AppendUndoList(BoardState(), Move());
}

Engine::Engine(std::string fen)
	: firstClick({ -1, -1 }), graphics()
{
	LoadPosition(fen);
	AppendUndoList(BoardState(), Move());
}

Engine::~Engine() {}

void Engine::Update()
{
	Move move;
	while (true) // Keep looping until we get a valid move
	{
		Render();

		if (IsOver())
			return; // Game is finished

		if (botPlaying && currentPlayer == bot->GetColor())
		{
			move = bot->TTGetMove();
			//Move otherMove = bot->GetMove();
			//std::cout << otherMove.ToString() << "\n";
			std::cout << move.ToString() << "\n";
			break;
		}

		if (!StoreMove())
			continue; // No full move was entered, keep asking

		ProcessMove(move);

		if (invalidMove)
		{
			std::cout << "Invalid move\n";
			invalidMove = false;
			continue; // Move was structurally invalid
		}

		break; // Move was valid, exit loop
	}

	// Make the move
	MakeMove(move);
	//Render();
	//Sleep(1000);

	// Not necessary but good for catching bugs, mostly in cached king pos
	CheckKingInCheck();

	// Update game state after move
	CheckCheckmate();
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
	// Highlight king if in check
	if (checkStatus)
	{
		std::pair<int, int> kingPos = (checkStatus & 0b10) ? whiteKingPos : blackKingPos;
		graphics.QueueRender([=]() { graphics.DrawSquareHighlight(kingPos.first, kingPos.second, { 255, 0, 0, 100 }); }); // Red highlight
	}
	if (moveHistory.size() >= 1)
	{
		Move lastMove = moveHistory.back();
		graphics.QueueRender([=]() { graphics.DrawSquareHighlight(lastMove.startRow, lastMove.startCol, { 180, 255, 0, 100 }); });
		graphics.QueueRender([=]() { graphics.DrawSquareHighlight(lastMove.endRow,   lastMove.endCol,   { 255, 180, 0, 100 }); });
	}

	graphics.Render(board);
}

int Engine::Eval()
{
	int score = 0;
	const int pieceValues[] = { 0, 100, 320, 330, 500, 900, 100000 };
	for (int r = 0; r < 8; ++r)
	{
		for (int c = 0; c < 8; ++c)
		{
			Piece piece = board[r][c].GetPiece();
			if (piece.GetType() != Pieces::NONE)
			{
				int value = pieceValues[static_cast<int>(piece.GetType()) + 1];
				score += (piece.GetColor() == Color::WHITE) ? value : -value;
			}
		}
	}
	return score;
}

int Engine::PieceToIndex(const Piece& p) const
{
	switch (p.GetType())
	{
	case Pieces::PAWN:   return (p.GetColor() == Color::WHITE) ? 0 : 6;
	case Pieces::KNIGHT: return (p.GetColor() == Color::WHITE) ? 1 : 7;
	case Pieces::BISHOP: return (p.GetColor() == Color::WHITE) ? 2 : 8;
	case Pieces::ROOK:   return (p.GetColor() == Color::WHITE) ? 3 : 9;
	case Pieces::QUEEN:  return (p.GetColor() == Color::WHITE) ? 4 : 10;
	case Pieces::KING:   return (p.GetColor() == Color::WHITE) ? 5 : 11;
	default: return -1;
	}
}

uint64_t Engine::ComputeFullHash() const
{
	uint64_t h = 0;
	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			Piece piece = board[r][c].GetPiece();
			if (piece.GetType() != Pieces::NONE) {
				int pieceIndex = PieceToIndex(piece);
				int sq = r * 8 + c;
				h ^= zobrist.piece[pieceIndex][sq];
			}
		}
	}
	if (currentPlayer == Color::BLACK) h ^= zobrist.sideToMove;

	// castling rights
	if (whiteCastlingRights[1]) h ^= zobrist.castling[0]; // WK
	if (whiteCastlingRights[0]) h ^= zobrist.castling[1]; // WQ
	if (blackCastlingRights[1]) h ^= zobrist.castling[2]; // BK
	if (blackCastlingRights[0]) h ^= zobrist.castling[3]; // BQ

	// en passant
	if (enPassantTarget[0] != -1 && enPassantTarget[1] != -1)
	{
		int file = enPassantTarget[1];
		h ^= zobrist.enPassantFile[file];
	}

	return h;
}

bool Engine::StoreMove()
{
	std::pair<int, int> click = graphics.GetInputs();
	// -2 -2 for undo
	if (click.first == -3 && click.second == -3)
	{
		std::cout << GetFEN() << "\n";
		return false;
	}
	if (click.first == -2 && click.second == -2)
	{
		UndoTurn();
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

void Engine::ProcessMove(Move& move)
{
	// Castling
	if (notationMove == "O-O-O" || notationMove == "O-O")
	{
		if (InCheck(currentPlayer)) // Can't castle out of check
		{
			invalidMove = true;
			return;
		}
		// Set start square to king and end square to 2 away
		move.startRow = (currentPlayer == Color::WHITE) ? 7 : 0;
		move.startCol = 4;
		move.endRow = move.startRow;
		move.endCol = (notationMove == "O-O") ? 6 : 2;
		move.wasCastle = true;
		move.promotion = 6; // No promotion
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
	if (!ValidMove(movingPiece, move)) // Invalid move for the piece
	{
		invalidMove = true;
		return;
	}

	move.promotion = static_cast<int>((movingPiece.GetType() == Pieces::PAWN
		&& (move.endRow == 0 || move.endRow == 7)) ?
		Pieces::QUEEN : Pieces::NONE); // Auto promote to queen
	move.wasCastle = false;
	if (movingPiece.GetType() == Pieces::PAWN && move.endCol != move.startCol && // Moved diagonally
		board[move.endRow][move.endCol].IsEmpty()) // Target square is empty
		move.wasEnPassant = true;
}

//void Engine::MakeMove(const Move move)
//{
//	BoardState state;
//
//	const Piece movingPiece = board[move.startRow][move.startCol].GetPiece();
//	const Piece targetPiece = board[move.endRow][move.endCol].GetPiece();
//
//	// 1. Move piece
//	board[move.endRow][move.endCol].SetPiece(movingPiece);
//	board[move.startRow][move.startCol].SetPiece(Piece(Pieces::NONE, Color::NONE));
//
//	state.movedPiece = static_cast<uint8_t>(movingPiece.GetType());
//	state.capturedPiece = static_cast<uint8_t>(targetPiece.GetType());
//	// 2. Save state to undo list
//	// Here so when the move ^ is undone, it will be complimented with the current state
//	// The move above will be undone, but the game state will be applied
//	AppendUndoList(state, move);
//
//	// Update king position if needed
//	if (movingPiece.GetType() == Pieces::KING)
//	{
//		if (currentPlayer == Color::WHITE)
//			whiteKingPos = { move.endRow, move.endCol };
//		else
//			blackKingPos = { move.endRow, move.endCol };
//	}
//
//	// 3. Castling rights
//	UpdateCastlingRights(move, movingPiece, targetPiece);
//	// Handle castling
//	if (move.wasCastle)
//	{
//		bool kingside = (move.endCol == 6);
//		int row = (currentPlayer == Color::WHITE) ? 7 : 0;
//		int rookStartCol = kingside ? 7 : 0;
//		int rookEndCol = kingside ? 5 : 3;
//		// Move rook
//		board[row][rookEndCol].SetPiece(board[row][rookStartCol].GetPiece());
//		board[row][rookStartCol].SetPiece(Piece(Pieces::NONE, Color::NONE));
//		// Update king position
//		if (currentPlayer == Color::WHITE)
//			whiteKingPos = { row, move.endCol };
//		else
//			blackKingPos = { row, move.endCol };
//	}
//
//	// 4. En passant sqaure/capture
//	UpdateEnPassantSquare(move);
//	// Check for capture
//	if (move.wasEnPassant)
//	{
//		int pawnRow = (currentPlayer == Color::WHITE) ? move.endRow + 1 : move.endRow - 1;
//		board[pawnRow][move.endCol].SetPiece(Piece(Pieces::NONE, Color::NONE));
//	}
//
//	// 5. Pawn promotion
//	if (move.promotion != 6)
//	{
//		board[move.endRow][move.endCol].SetPiece(Piece((Pieces)move.promotion, currentPlayer));
//	}
//
//	// 6. Halfmove clock
//	if (movingPiece.GetType() == Pieces::PAWN || targetPiece.GetType() != Pieces::NONE)
//		halfmoves = 0;
//	else if (currentPlayer == Color::BLACK)
//		halfmoves++;
//	if (halfmoves >= 50) // 50-move rule
//	{
//		draw = true;
//		std::cout << "Game is a draw by the 50-move rule!\n";
//		return;
//	}
//
//	// 7. Save move
//	moveHistory.push_back(move);
//
//	// remove old en-passant
//	if (enPassantTarget[0] != -1)
//		zobristKey ^= zobrist.enPassantFile[enPassantTarget[1]];
//
//	// remove old castling rights
//	if (whiteCastlingRights[1]) zobristKey ^= zobrist.castling[0];
//	if (whiteCastlingRights[0]) zobristKey ^= zobrist.castling[1];
//	if (blackCastlingRights[1]) zobristKey ^= zobrist.castling[2];
//	if (blackCastlingRights[0]) zobristKey ^= zobrist.castling[3];
//
//	// remove piece from start square
//	int fromSq = move.startRow * 8 + move.startCol;
//	zobristKey ^= zobrist.piece[PieceToIndex(movingPiece)][fromSq];
//
//	// if capture
//	if (targetPiece.GetType() != Pieces::NONE)
//		zobristKey ^= zobrist.piece[PieceToIndex(targetPiece)][move.endRow * 8 + move.endCol];
//
//	// if en passant capture
//	if (move.wasEnPassant) {
//		int capRow = (currentPlayer == Color::WHITE) ? move.endRow + 1 : move.endRow - 1;
//		Piece capPawn(Pieces::PAWN, (currentPlayer == Color::WHITE ? Color::BLACK : Color::WHITE));
//		zobristKey ^= zobrist.piece[PieceToIndex(capPawn)][capRow * 8 + move.endCol];
//	}
//
//	// add moved piece to end square (promotion handled separately)
//	if (move.promotion != static_cast<int>(Pieces::NONE)) {
//		Piece promo((Pieces)move.promotion, currentPlayer);
//		zobristKey ^= zobrist.piece[PieceToIndex(promo)][move.endRow * 8 + move.endCol];
//	}
//	else {
//		zobristKey ^= zobrist.piece[PieceToIndex(movingPiece)][move.endRow * 8 + move.endCol];
//	}
//
//	// update castling rights
//	if (whiteCastlingRights[1]) zobristKey ^= zobrist.castling[0];
//	if (whiteCastlingRights[0]) zobristKey ^= zobrist.castling[1];
//	if (blackCastlingRights[1]) zobristKey ^= zobrist.castling[2];
//	if (blackCastlingRights[0]) zobristKey ^= zobrist.castling[3];
//
//	// update en-passant (if new pawn double-move)
//	if (movingPiece.GetType() == Pieces::PAWN && abs(move.startRow - move.endRow) == 2) {
//		zobristKey ^= zobrist.enPassantFile[move.startCol];
//	}
//
//	// side to move
//	zobristKey ^= zobrist.sideToMove;
//
//	//std::cout << zobristKey << "\n";
//	if (zobristKey != ComputeFullHash())
//	{
//		std::cout << "Doesn't match\n";
//		std::cout << "Move key " << zobristKey << " vs full hash " << ComputeFullHash() << "\n";
//	}
//
//	// 8. Change side to move
//	ChangePlayers();
//}
void Engine::MakeMove(const Move move)
{
	// --- snapshot old state (before any modification) ---
	int oldEP_row = enPassantTarget[0];
	int oldEP_col = enPassantTarget[1];
	bool oldWhiteCastle0 = whiteCastlingRights[0];
	bool oldWhiteCastle1 = whiteCastlingRights[1];
	bool oldBlackCastle0 = blackCastlingRights[0];
	bool oldBlackCastle1 = blackCastlingRights[1];
	Color oldPlayer = currentPlayer;

	const Piece movingPiece = board[move.startRow][move.startCol].GetPiece();
	const Piece targetPiece = board[move.endRow][move.endCol].GetPiece();
	int fromSq = move.startRow * 8 + move.startCol;
	int toSq = move.endRow * 8 + move.endCol;

	// Save state for undo (snapshot zobrist key too)
	BoardState state;
	state.movedPiece = static_cast<uint8_t>(movingPiece.GetType());
	state.capturedPiece = static_cast<uint8_t>(targetPiece.GetType());
	AppendUndoList(state, move); // make sure AppendUndoList stores zobristKey in the state

	// --- XOR OUT old ephemeral things (use the snapshots) ---
	if (oldEP_row != -1) {
		int oldEP_file = oldEP_col;
		zobristKey ^= zobrist.enPassantFile[oldEP_file];
	}

	if (oldWhiteCastle1) zobristKey ^= zobrist.castling[0];
	if (oldWhiteCastle0) zobristKey ^= zobrist.castling[1];
	if (oldBlackCastle1) zobristKey ^= zobrist.castling[2];
	if (oldBlackCastle0) zobristKey ^= zobrist.castling[3];

	// XOR out moving piece from its FROM square (old board)
	zobristKey ^= zobrist.piece[PieceToIndex(movingPiece)][fromSq];

	// If a normal capture (not en-passant), XOR out captured piece at destination (old board)
	if (targetPiece.GetType() != Pieces::NONE) {
		zobristKey ^= zobrist.piece[PieceToIndex(targetPiece)][toSq];
	}
	// If en-passant *capture*, the captured pawn is not on the 'to' square; it's behind it.
	if (move.wasEnPassant) {
		int capRow = (oldPlayer == Color::WHITE) ? move.endRow + 1 : move.endRow - 1;
		int capSq = capRow * 8 + move.endCol;
		Piece capturedPawn(Pieces::PAWN, (oldPlayer == Color::WHITE ? Color::BLACK : Color::WHITE));
		zobristKey ^= zobrist.piece[PieceToIndex(capturedPawn)][capSq];
	}

	// --- Now apply the physical move to board & state (exactly like your existing code) ---
	// 1. Move piece on board
	board[move.endRow][move.endCol].SetPiece(movingPiece);
	board[move.startRow][move.startCol].SetPiece(Piece(Pieces::NONE, Color::NONE));

	// update king pos if needed (you already have this)
	if (movingPiece.GetType() == Pieces::KING) {
		if (oldPlayer == Color::WHITE) whiteKingPos = { move.endRow, move.endCol };
		else blackKingPos = { move.endRow, move.endCol };
	}

	// 2. Update castling rights (this modifies whiteCastlingRights / blackCastlingRights)
	UpdateCastlingRights(move, movingPiece, targetPiece);

	// 3. Handle castling rook movement (if move.wasCastle)
	if (move.wasCastle) {
		bool kingside = (move.endCol == 6);
		int row = (oldPlayer == Color::WHITE) ? 7 : 0;
		int rookStartCol = kingside ? 7 : 0;
		int rookEndCol = kingside ? 5 : 3;
		int rookFromSq = row * 8 + rookStartCol;
		int rookToSq = row * 8 + rookEndCol;

		Piece rook(Pieces::ROOK, oldPlayer);

		// --- update zobrist for rook ---
		zobristKey ^= zobrist.piece[PieceToIndex(rook)][rookFromSq]; // remove rook from start square
		zobristKey ^= zobrist.piece[PieceToIndex(rook)][rookToSq];   // add rook to end square

		board[row][rookEndCol].SetPiece(board[row][rookStartCol].GetPiece());
		board[row][rookStartCol].SetPiece(Piece(Pieces::NONE, Color::NONE));

		if (oldPlayer == Color::WHITE) whiteKingPos = { row, move.endCol };
		else blackKingPos = { row, move.endCol };

		//bool kingside = (move.endCol == 6);
		//int row = (oldPlayer == Color::WHITE) ? 7 : 0;
		//int rookStartCol = kingside ? 7 : 0;
		//int rookEndCol = kingside ? 5 : 3;
		//int rookFromSq = row * 8 + rookStartCol;
		//int rookToSq = row * 8 + rookEndCol;
		//Piece rook(Pieces::ROOK, oldPlayer);

		//// --- update zobrist for rook ---
		//zobristKey ^= zobrist.piece[PieceToIndex(rook)][rookFromSq]; // remove rook from start square
		//zobristKey ^= zobrist.piece[PieceToIndex(rook)][rookToSq];   // add rook to end square

		//// already have the board updates below
		//board[row][rookEndCol].SetPiece(board[row][rookStartCol].GetPiece());
		//board[row][rookStartCol].SetPiece(Piece(Pieces::NONE, Color::NONE));

		//if (oldPlayer == Color::WHITE) whiteKingPos = { row, move.endCol };
		//else blackKingPos = { row, move.endCol };
	}

	// 4. Update en-passant target (this will change enPassantTarget)
	UpdateEnPassantSquare(move);

	// 5. Handle en-passant capture on board
	if (move.wasEnPassant) {
		int pawnRow = (oldPlayer == Color::WHITE) ? move.endRow + 1 : move.endRow - 1;
		board[pawnRow][move.endCol].SetPiece(Piece(Pieces::NONE, Color::NONE));
	}

	// 6. Handle promotion on board
	if (move.promotion != 6) {
		board[move.endRow][move.endCol].SetPiece(Piece((Pieces)move.promotion, oldPlayer));
	}

	// 7. Update halfmove clock etc. (your existing logic)
	if (movingPiece.GetType() == Pieces::PAWN || targetPiece.GetType() != Pieces::NONE)
		halfmoves = 0;
	else if (oldPlayer == Color::BLACK)
		halfmoves++;
	if (halfmoves >= 50) { draw = true; /*...*/ }

	// 8. Save moveHistory (you already do)
	moveHistory.push_back(move);

	// --- XOR IN new things (after board has new pos & flags) ---
	// add piece at destination (promotion handled)
	if (move.promotion != static_cast<int>(Pieces::NONE)) {
		Piece promo((Pieces)move.promotion, oldPlayer);
		zobristKey ^= zobrist.piece[PieceToIndex(promo)][toSq];
	}
	else {
		zobristKey ^= zobrist.piece[PieceToIndex(movingPiece)][toSq];
	}

	// XOR in new castling rights (use the current whiteCastlingRights/blackCastlingRights)
	if (whiteCastlingRights[1]) zobristKey ^= zobrist.castling[0];
	if (whiteCastlingRights[0]) zobristKey ^= zobrist.castling[1];
	if (blackCastlingRights[1]) zobristKey ^= zobrist.castling[2];
	if (blackCastlingRights[0]) zobristKey ^= zobrist.castling[3];

	// XOR in new en-passant if present (enPassantTarget updated earlier)
	if (enPassantTarget[0] != -1) {
		zobristKey ^= zobrist.enPassantFile[enPassantTarget[1]];
	}

	// --- Finally, flip side-to-move in engine state and in the hash consistently ---
	ChangePlayers(); // flips currentPlayer
	zobristKey ^= zobrist.sideToMove;

	// --- Optional check: recompute and compare ---
	//uint64_t recomputed = ComputeFullHash();
	//if (zobristKey != recomputed) {
	//	std::cout << "Hash mismatch\n";
	//	std::cout << "inc:  " << std::hex << zobristKey << "\n";
	//	std::cout << "full: " << std::hex << recomputed << "\n";
	//	std::cout << GetFEN() << "\n";
	//}
}

bool Engine::HandleSpecialNotation()
{
	if (notationMove == "resign")
	{
		checkmate = true;
		checkStatus = (currentPlayer == Color::WHITE) ? 0b01 : 0b10; // Opponent
		return true;
	}
	else if (notationMove == "draw")
	{
		checkmate = false;
		checkStatus = 0; // Draw
		return true;
	}
	else if (notationMove == "undo")
	{
		UndoMove();
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

bool Engine::ValidMove(const Piece piece, const Move move)
{
	// Check if the move is valid for the given piece type
	std::vector<uint8_t> validMoves = BoardCalculator::GetValidMoves(move.startRow, move.startCol, board);
	uint8_t targetSquare = move.endRow * 8 + move.endCol;

	for (uint8_t validMove : validMoves)
		if (validMove == targetSquare)
			return true;

	return false;
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
	fen += std::to_string(halfmoves); // Halfmove clock
	fen += " ";
	fen += std::to_string(moveHistory.size() / 2 + 1); // Fullmove number

	return fen;
}

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
	undoHistory.clear();
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
	zobristKey = ComputeFullHash();
}

void Engine::AppendUndoList(BoardState state, const Move move)
{
	state.zobristKey = zobristKey;
	state.promotion = move.promotion;
	state.fromSquare = move.startRow * 8 + move.startCol;
	state.toSquare = move.endRow * 8 + move.endCol;
	state.enPassantTarget = (enPassantTarget[0] == -1) ? 64 : (enPassantTarget[0] * 8 + enPassantTarget[1]);
	state.castlingRights = (whiteCastlingRights[0] << 3) | (whiteCastlingRights[1] << 2) |
		(blackCastlingRights[0] << 1) | (blackCastlingRights[1]);
	state.halfmoveClock = halfmoves;
	state.wasEnPassant = move.wasEnPassant;
	state.wasCastling = move.wasCastle;
	state.playerToMove = (currentPlayer == Color::WHITE);
	undoHistory.push_back(state);
}

void Engine::UndoMove()
{
	if (undoHistory.size() <= 1) // Last move is the initial position, can't undo
	{
		invalidMove = true;
		return; // No move to undo
	}

	//undoHistory.pop_back();
	BoardState lastState = undoHistory.back();
	undoHistory.pop_back();

	Color player = lastState.playerToMove ? Color::WHITE : Color::BLACK;
	Color enemy = (player == Color::WHITE) ? Color::BLACK : Color::WHITE;

	// Restore pieces
	int fromRow = lastState.fromSquare / 8;
	int fromCol = lastState.fromSquare % 8;
	int toRow = lastState.toSquare / 8;
	int toCol = lastState.toSquare % 8;

	zobristKey = lastState.zobristKey;

	// Can't restore to previous move because
					// Variable is managed in a way that converts from 0-6 to Piece
	Piece movedPiece = Piece(static_cast<Pieces>(lastState.movedPiece), player);
	board[fromRow][fromCol].SetPiece(movedPiece);
	board[toRow][toCol].SetPiece(Piece(static_cast<Pieces>(lastState.capturedPiece), enemy));

	// Restore king position if needed
	if (movedPiece.GetType() == Pieces::KING)
	{
		if (player == Color::WHITE)
			whiteKingPos = { fromRow, fromCol };
		else
			blackKingPos = { fromRow, fromCol };
	}

	// Handle en passant capture
	if (lastState.wasEnPassant)
	{
		int pawnRow = (player == Color::WHITE) ? toRow + 1 : toRow - 1;
		board[pawnRow][toCol].SetPiece(Piece(Pieces::PAWN, enemy));
		board[toRow][toCol].SetPiece(Piece(Pieces::NONE, Color::NONE));
	}

	// Handle castling
	if (lastState.wasCastling)
	{
		bool kingside = (toCol == 6);
		int row = (player == Color::WHITE) ? 7 : 0;
		int rookStartCol = kingside ? 7 : 0;
		int rookEndCol = kingside ? 5 : 3;
		board[row][rookStartCol].SetPiece(board[row][rookEndCol].GetPiece());
		board[row][rookEndCol].SetPiece(Piece(Pieces::NONE, Color::NONE));
		// King position was updated earlier
	}

	// Restore en passant target square
	if (lastState.enPassantTarget == 64)
	{
		enPassantTarget[0] = -1;
		enPassantTarget[1] = -1;
	}
	else
	{
		enPassantTarget[0] = lastState.enPassantTarget / 8;
		enPassantTarget[1] = lastState.enPassantTarget % 8;
	}

	// Restore castling rights
	whiteCastlingRights[0] = (lastState.castlingRights & 0b1000) != 0;
	whiteCastlingRights[1] = (lastState.castlingRights & 0b0100) != 0;
	blackCastlingRights[0] = (lastState.castlingRights & 0b0010) != 0;
	blackCastlingRights[1] = (lastState.castlingRights & 0b0001) != 0;

	// Restore halfmove clock
	halfmoves = lastState.halfmoveClock;

	// Couldn't be in check 2 moves in a row
	// If in check after undo it gets set later
	checkStatus = 0;

	// Remove last move from history
	if (!moveHistory.empty())
		moveHistory.pop_back();

	// Reset if game ended
	invalidMove = false;
	checkmate = false;
	draw = false;

	currentPlayer = player;
}

void Engine::UndoTurn()
{
	UndoMove();
	UndoMove();
}

void Engine::SetBot(Bot* bot)
{
	this->bot = bot;
	botPlaying = (bot != nullptr);
}

void Engine::CheckKingInCheck()
{
	if (BoardCalculator::IsSquareAttacked(whiteKingPos.first, whiteKingPos.second, Color::BLACK, board))
	{
		checkStatus |= (1 << 1);
	}
	else
	{
		checkStatus &= ~(1 << 1);
	}
	if (BoardCalculator::IsSquareAttacked(blackKingPos.first, blackKingPos.second, Color::WHITE, board))
	{
		checkStatus |= (1 << 0);
	}
	else
	{
		checkStatus &= ~(1 << 0);
	}

	//if (checkStatus & 0b10)
	//{
	//	graphics.QueueRender([=] {graphics.DrawSquareHighlight(whiteKingPos.first, whiteKingPos.second, { 255, 0, 0, 100 }); });
	//}
	//if (checkStatus & 0b01)
	//{
	//	graphics.QueueRender([=] {graphics.DrawSquareHighlight(blackKingPos.first, blackKingPos.second, { 255, 0, 0, 100 }); });
	//}
}

void Engine::UpdateCastlingRights(const Move move, const Piece movingPiece, const Piece targetPiece)
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

void Engine::UpdateEnPassantSquare(const Move move)
{
	const Piece movingPiece = board[move.endRow][move.endCol].GetPiece();
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
		if (!(checkStatus & 0b11)) // Stalemate
		{
			draw = true;
			//std::cout << "Game is a draw by stalemate!\n";
			return;
		}
		checkmate = true;
		// Current player in checkmate
		//std::cout << ((currentPlayer == Color::WHITE) ? "Black" : "White") << " wins by checkmate!\n";
	}
}
