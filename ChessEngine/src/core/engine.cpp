#include "engine.hpp"

#include <iostream>
#include <cmath>
#include <Windows.h>

#include <cstdint>
#include <algorithm>
#include <cctype>

#include "bot/bot.hpp"
#include "movegen.hpp"

// Make GameState variables not need prefix
using namespace GameState;

Engine::Engine()
	: firstClick(-1), graphics()
{
	Init("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

Engine::Engine(std::string fen)
	: firstClick(-1), graphics()
{
	Init(fen);
}

Engine::~Engine() {}

void Engine::Init(std::string fen)
{
	LoadPosition(fen);
	AppendUndoList(BoardState(), Move());

	positionStack.clear();
	positionCounts.clear();
	uint64_t startKey = GetZobristKey();
	positionStack.push_back(startKey);
	positionCounts[startKey] = 1;

	Movegen::InitPrecomputedAttacks();
}

void Engine::Reset()
{
	// Clear board + load starting position
	LoadPosition("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

	// Reset zobrist key
	zobristKey = ComputeFullHash();

	// Clear repetition tracking
	positionCounts.clear();
	positionStack.clear();

	// Clear move/undo history
	moveHistory.clear();
	undoHistory.clear();

	// Reset king positions
	whiteKingPos = -1;
	blackKingPos = -1;

	// Reset notation/UI helpers
	firstClick = -1;

	// Reset game state flags
	GameState::currentPlayer = Color::WHITE;
	GameState::checkmate = false;
	GameState::draw = false;
	GameState::checkStatus = 0;

	// Reset bot flags
	botPlaying[0] = false;
	botPlaying[1] = false;

	// Note: don't delete `bots[]` since they are set externally
}

void Engine::Update()
{
	Move move;
	while (true) // Keep looping until we get a valid move
	{
		Render();

		if (IsOver())
			return; // Game is finished
		
		int index = IsWhite(currentPlayer) ? 0 : 1;
		Bot* bot = bots[index];

		if (botPlaying[index] && bot != nullptr)
		{
			move = bot->GetMove();
			StoreMove(move); // Update window and shit
			break;
		}

		if (!StoreMove(move))
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
	UpdateEndgameStatus();
	std::cout << MoveToUCI(move) << '\n';

	// Update game state after move
	CheckCheckmate();
	if (IsDraw())
	{
		draw = true;
		std::cout << "Draw by 3 fold repetition\n";
	}
}

void Engine::Render()
{
	// If first click is valid highlight the square
	if (firstClick != -1)
	{
		graphics.QueueRender([=]() { graphics.DrawSquareHighlight(firstClick, { 0, 255, 0, 100 }); });

		// Render the valid moves for the selected piece
		for (uint8_t moveSq : Movegen::GetValidMoves(firstClick, bitboards))
		{
			graphics.QueueRender([=]() { graphics.DrawSquareHighlight(moveSq, { 0, 0, 255, 100 }); }); // Blue highlight
		}
	}
	// Highlight king if in check
	if (checkStatus)
	{
		int kingPos = (checkStatus & 0b10) ? whiteKingPos : blackKingPos;
		graphics.QueueRender([=]() { graphics.DrawSquareHighlight(kingPos, { 255, 0, 0, 100 }); }); // Red highlight
	}

	if (moveHistory.size() >= 1)
	{
		Move lastMove = moveHistory.back();
		graphics.QueueRender([=]() { graphics.DrawSquareHighlight(GetStart(lastMove), { 180, 255, 0, 100 }); });
		graphics.QueueRender([=]() { graphics.DrawSquareHighlight(GetEnd(lastMove),   { 255, 180, 0, 100 }); });
	}
	
	graphics.Render(board);
}

void Engine::UpdateEndgameStatus()
{
	int whitePawns = 0, blackPawns = 0;
	int whiteKnights = 0, blackKnights = 0;
	int whiteBishops = 0, blackBishops = 0;
	int whiteRooks = 0, blackRooks = 0;
	int whiteQueens = 0, blackQueens = 0;

	for (int sq = 0; sq < 64; ++sq)
	{
		Piece piece = board[sq].GetPiece();
		if (piece.GetType() == Pieces::NONE) continue;

		if (IsWhite(piece.GetColor()))
		{
			switch (piece.GetType())
			{
			case Pieces::PAWN:
				whitePawns++;
				break;
			case Pieces::KNIGHT:
				whiteKnights++;
				break;
			case Pieces::BISHOP:
				whiteBishops++;
				break;
			case Pieces::ROOK:
				whiteRooks++;
				break;
			case Pieces::QUEEN:
				whiteQueens++;
				break;
			default:
				break;
			}
		}
		else // Black piece
		{
			switch (piece.GetType())
			{
			case Pieces::PAWN:
				blackPawns++;
				break;
			case Pieces::KNIGHT:
				blackKnights++;
				break;
			case Pieces::BISHOP:
				blackBishops++;
				break;
			case Pieces::ROOK:
				blackRooks++;
				break;
			case Pieces::QUEEN:
				blackQueens++;
				break;
			default:
				break;
			}
		}
	}

	int totalMaterial =
		whiteQueens * 9 + whiteRooks * 5 + whiteBishops * 3 + whiteKnights * 3 +
		blackQueens * 9 + blackRooks * 5 + blackBishops * 3 + blackKnights * 3;

	// No queens remain
	if (whiteQueens == 0 && blackQueens == 0) endgame = true;
	// Total non-pawn material is <= 14
	if (totalMaterial <= 14) endgame = true;
}

int Engine::PieceToIndex(const Piece& p) const
{
	switch (p.GetType())
	{
	case Pieces::PAWN:   return p.GetColor() == Color::BLACK ? 0 : 1;
	case Pieces::KNIGHT: return p.GetColor() == Color::BLACK ? 2 : 3;
	case Pieces::BISHOP: return p.GetColor() == Color::BLACK ? 4 : 5;
	case Pieces::ROOK:   return p.GetColor() == Color::BLACK ? 6 : 7;
	case Pieces::QUEEN:  return p.GetColor() == Color::BLACK ? 8 : 9;
	case Pieces::KING:   return p.GetColor() == Color::BLACK ? 10 : 11;
	default: return -1;
	}
}

uint64_t Engine::ComputeFullHash() const
{
	uint64_t hash = 0;

	for (int sq = 0; sq < 64; ++sq)
	{
		Piece piece = board[sq].GetPiece();
		if (piece.GetType() != Pieces::NONE)
		{
			int idx = PieceToIndex(piece);  // Polyglot piece index 0..11
			int polyglotSq = (7 - ToRow(sq)) * 8 + ToCol(sq);
			hash ^= zobrist.piece[idx][polyglotSq];
			//hash ^= zobrist.piece[idx][sq];
		}
	}

	// Side to move
	if (IsWhite(currentPlayer)) hash ^= zobrist.sideToMove;

	// Castling rights (Polyglot order: WK, WQ, BK, BQ)
	if (whiteCastlingRights[1]) hash ^= zobrist.castling[0]; // WK
	if (whiteCastlingRights[0]) hash ^= zobrist.castling[1]; // WQ
	if (blackCastlingRights[1]) hash ^= zobrist.castling[2]; // BK
	if (blackCastlingRights[0]) hash ^= zobrist.castling[3]; // BQ

	// En passant (only the file)
	if (enPassantTarget != -1)
	{
		//std::cout << "Adding en passant\n";
		int file = ToCol(enPassantTarget);
		hash ^= zobrist.enPassantFile[file];
	}

	return hash;
}

bool Engine::StoreMove(Move& move)
{
	int click = graphics.GetInputs();
	// -2 for undo
	if (click == -3)
	{
		std::cout << GetFEN() << "\n";
		return false;
	}
	if (click == -2)
	{
		UndoMove();
		return false;
	}
	// Check if -1, and return
	if (click == -1)
	{
		return false;
	}

	if (firstClick == -1) // First click
	{
		// Check if the first click was valid
		if (board[click].IsEmpty() ||
			board[click].GetPiece().GetColor() != currentPlayer)
		{
			std::cout << "Invalid first click\n";
			if (board[firstClick].IsEmpty()) std::cout << "Empty\n";
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
			firstClick = -1;
			return false; // Wait for new first click
		}
		// If click is on a piece of the same color, change firstClick
		if (!board[click].IsEmpty() &&
			 board[click].GetPiece().GetColor() == currentPlayer)
		{
			firstClick = click;
			return false; // Wait for second click
		}

		int row = ToRow(click);
		int col = ToCol(click);

		bool isCastle = false;
		bool isEnPassant = false;
		int promotion = static_cast<int>(Pieces::NONE);

		// If move is invalid it gets caught and handled in ProcessMove
		// Check for castling
		if (board[firstClick].GetPiece().GetType() == Pieces::KING &&
			abs(firstClick - click) == 2 && ToRow(firstClick) == row) // Same row, 2 columns apart (col1 - col2 = sq1 - sq2)
		{
			if (!BoardCalculator::IsCastlingValid(col == 6, bitboards) || InCheck(currentPlayer))
			{
				firstClick = -1; // Reset for next move
				return false; // Move ready to process
			}

			isCastle = true;
		}

		// Normal click, not necessarily valid move
		move = EncodeMove(firstClick, click, promotion, isEnPassant, isCastle);

		firstClick = -1; // Reset for next move
		return true; // Move ready to process
	}
}

void Engine::ProcessMove(Move& move)
{
	// Castling
	if (IsCastle(move)) return; // From and to squares already assigned
	bool isCastle = false;

	const int startSquare = GetStart(move);
	const int endSquare = GetEnd(move);

	int promotion = static_cast<int>(move);
	bool isEnPassant = false;

	Piece movingPiece = board[startSquare].GetPiece();

	// Check valid move conditions
	// Don't need out of bounds because 'i'-'z' and '9' aren't valid
	if (!ValidMove(movingPiece, move)) // Invalid move for the piece
	{
		invalidMove = true;
		return;
	}

	int fromRow = ToRow(startSquare);
	int fromCol = ToCol(startSquare);
	int toRow	= ToRow(endSquare);
	int toCol =   ToCol(endSquare);

	promotion = static_cast<int>((movingPiece.GetType() == Pieces::PAWN &&
		(toRow == 0 || toRow == 7)) ? Pieces::QUEEN : Pieces::NONE); // Auto promote to queen
	if (movingPiece.GetType() == Pieces::PAWN && toCol != fromCol && // Moved diagonally
		board[endSquare].IsEmpty()) // Target square is empty
	{
		isEnPassant = true;
	}

	move = EncodeMove(startSquare, endSquare, promotion, isEnPassant, isCastle);
}

void Engine::MakeMove(const Move move)
{
	//graphics.RenderBitboard(bitboards.pieceBitboards[1][2]);
	//graphics.Render(board);
	const int startSquare = GetStart(move);
	const int endSquare = GetEnd(move);
	const int promotion = GetPromotion(move);
	const bool isEnPassant = IsEnPassant(move);
	const bool isCastle = IsCastle(move);

	const Piece movingPiece = board[startSquare].GetPiece();
	const Piece targetPiece = board[endSquare].GetPiece();

	// Save state for undo
	BoardState state;
	state.movedPiece	= static_cast<uint8_t>(movingPiece.GetType());
	state.capturedPiece = static_cast<uint8_t>(targetPiece.GetType());
	AppendUndoList(state, move);

	// XOR OUT old state
	if (enPassantTarget != -1)
		zobristKey ^= zobrist.enPassantFile[ToCol(enPassantTarget)];

	if (whiteCastlingRights[1]) zobristKey ^= zobrist.castling[0];
	if (whiteCastlingRights[0]) zobristKey ^= zobrist.castling[1];
	if (blackCastlingRights[1]) zobristKey ^= zobrist.castling[2];
	if (blackCastlingRights[0]) zobristKey ^= zobrist.castling[3];

	// XOR out moving piece from its FROM square (old board)
	zobristKey ^= zobrist.piece[PieceToIndex(movingPiece)][startSquare];
	bitboards.Remove(movingPiece, startSquare);

	if (targetPiece.GetType() != Pieces::NONE)
	{
		zobristKey ^= zobrist.piece[PieceToIndex(targetPiece)][endSquare];
		bitboards.Remove(targetPiece, endSquare);
	}

	// Correct en passant capture square
	if (isEnPassant)
	{
		const int capSq = endSquare + (IsWhite(currentPlayer) ? 8 : - 8);
		Piece capturedPawn(Pieces::PAWN, Opponent(currentPlayer));
		zobristKey ^= zobrist.piece[PieceToIndex(capturedPawn)][capSq];
		bitboards.Remove(capturedPawn, capSq);
		board[capSq].SetPiece(Piece(Pieces::NONE, Color::NONE));
	}

	// 1. Move piece on board
	board[endSquare].SetPiece(movingPiece);
	board[startSquare].SetPiece(Piece(Pieces::NONE, Color::NONE));

	// Update king pos if needed
	if (movingPiece.GetType() == Pieces::KING)
		(IsWhite(currentPlayer) ? whiteKingPos : blackKingPos) = endSquare;

	// 2. Castle (and update rights)
	if (isCastle)
	{
		bool kingside = (ToCol(endSquare) == 6);
		int row = (IsWhite(currentPlayer)) ? 7 : 0;
		int rookFromSq	 = ToIndex(row, kingside ? 7 : 0);
		int rookToSq	 = ToIndex(row, kingside ? 5 : 3);

		Piece rook(Pieces::ROOK, currentPlayer);

		// Update zobrist for rook
		zobristKey ^= zobrist.piece[PieceToIndex(rook)][rookFromSq]; // remove rook from start square
		zobristKey ^= zobrist.piece[PieceToIndex(rook)][rookToSq];   // add rook to end square

		board[rookToSq].SetPiece(board[rookFromSq].GetPiece());
		board[rookFromSq].SetPiece(Piece(Pieces::NONE, Color::NONE));

		bitboards.Remove(rook, rookFromSq);
		bitboards.Add(rook, rookToSq);

		// Update castling rights
		if (IsWhite(currentPlayer))
			whiteCastlingRights[0] = whiteCastlingRights[1] = false;
		else
			blackCastlingRights[0] = blackCastlingRights[1] = false;
	}
	else // Update rights if not castle
		UpdateCastlingRights(move, movingPiece, targetPiece);


	// 4. Update en-passant target (this will change enPassantTarget)
	UpdateEnPassantSquare(move);

	// 5. Handle promotion on board
	if (promotion != static_cast<int>(Pieces::NONE))
	{
		Piece promotionPiece = Piece((Pieces)promotion, currentPlayer);
		board[endSquare].SetPiece(promotionPiece);

		bitboards.Add(promotionPiece, endSquare);
	}
	else
		bitboards.Add(movingPiece, endSquare);

	// 6. Update halfmove clock etc.
	if (movingPiece.GetType() == Pieces::PAWN || targetPiece.GetType() != Pieces::NONE)
		halfmoves = 0;
	else if (currentPlayer == Color::BLACK)
		halfmoves++;
	if (halfmoves >= 50) draw = true;

	// 7. Save moveHistory (you already do)
	moveHistory.push_back(move);

	// XOR IN new state
	if (promotion != static_cast<int>(Pieces::NONE))
	{
		Piece promotionPiece = Piece((Pieces)promotion, currentPlayer);
		zobristKey ^= zobrist.piece[PieceToIndex(promotionPiece)][endSquare];
	}
	else
		zobristKey ^= zobrist.piece[PieceToIndex(movingPiece)][endSquare];

	// XOR in new castling rights
	if (whiteCastlingRights[1]) zobristKey ^= zobrist.castling[0];
	if (whiteCastlingRights[0]) zobristKey ^= zobrist.castling[1];
	if (blackCastlingRights[1]) zobristKey ^= zobrist.castling[2];
	if (blackCastlingRights[0]) zobristKey ^= zobrist.castling[3];

	// XOR in new en-passant if present
	if (enPassantTarget != -1)
		zobristKey ^= zobrist.enPassantFile[ToCol(enPassantTarget)];

	// --- Finally, flip side-to-move in engine state and in the hash consistently ---
	ChangePlayers(); // Flips currentPlayer
	if (IsWhite(currentPlayer))
		zobristKey ^= zobrist.sideToMove;

	// Update check
	CheckKingInCheck();

	positionStack.push_back(zobristKey);
	positionCounts[zobristKey] += 1;
}

bool Engine::IsDraw() const
{
	if (draw) return true;
	return (IsThreefold() || Is50Move());
}

bool Engine::IsThreefold() const
{
	if (positionStack.empty()) return false;
	auto it = positionCounts.find(positionStack.back());
	return (it != positionCounts.end() && it->second >= 3);
}

bool Engine::Is50Move() const
{
	return (halfmoves >= 50);
}

bool Engine::ValidMove(const Piece piece, const Move move)
{
	// Check if the move is valid for the given piece type
	std::vector<uint8_t> validMoves = Movegen::GetValidMoves(GetStart(move), bitboards);

	for (uint8_t validMove : validMoves)
		if (validMove == GetEnd(move))
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
			int sq = ToIndex(i, j);
			Piece piece = board[sq].GetPiece();
			if (piece.GetType() == Pieces::NONE) { emptyCount++; continue; }

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

			if (IsWhite(piece.GetColor()))
				pieceChar = toupper(pieceChar);
			fen += pieceChar;
		}

		if (emptyCount > 0)
		{
			fen += std::to_string(emptyCount);
			emptyCount = 0;
		}

		if (i < 7)
			fen += '/';
	}

	fen += IsWhite(currentPlayer) ? " w " : " b ";

	// Castling rights
	std::string castling;
	if (whiteCastlingRights[1]) castling += 'K';
	if (whiteCastlingRights[0]) castling += 'Q';
	if (blackCastlingRights[1]) castling += 'k';
	if (blackCastlingRights[0]) castling += 'q';
	fen += (castling.empty() ? "-" : castling);

	// En passant target square
	if (enPassantTarget != -1)
	{
		char file = 'a' + ToCol(enPassantTarget);
		char rank = '8' - ToRow(enPassantTarget);
		fen += " ";
		fen += file;
		fen += rank;
		fen += " ";
	}
	else
		fen += " - ";

	fen += std::to_string(halfmoves); // Halfmove clock
	fen += " ";
	fen += std::to_string(moveHistory.size() / 2 + 1); // Fullmove number

	return fen;
}

void Engine::LoadPosition(std::string fen)
{
	// Split FEN into fields
	std::istringstream fenStream(fen);
	std::string placement, activeColor, castling, enPassant, halfmoveStr, fullmoveStr;

	fenStream >> placement >> activeColor >> castling >> enPassant >> halfmoveStr >> fullmoveStr;

	// Clear board
	for (int sq = 0; sq < 64; ++sq)
			board[sq] = Square();

	bitboards = BitboardBoard{};

	// 1. Piece placement
	int row = 0, col = 0;
	for (char c : placement)
	{
		if (c == '/') { row++; col = 0; continue; }
		if (isdigit(c)) { col += c - '0'; continue; }

		Color color = isupper(c) ? Color::WHITE : Color::BLACK;
		Pieces pieceType;
		switch (tolower(c))
		{
		case 'p': pieceType = Pieces::PAWN;   break;
		case 'r': pieceType = Pieces::ROOK;   break;
		case 'n': pieceType = Pieces::KNIGHT; break;
		case 'b': pieceType = Pieces::BISHOP; break;
		case 'q': pieceType = Pieces::QUEEN;  break;
		case 'k': pieceType = Pieces::KING;
			(IsWhite(color) ? whiteKingPos : blackKingPos)  = ToIndex(row, col);
			break;
		default: pieceType = Pieces::NONE; break;
		}

		int sq = ToIndex(row, col);
		Piece piece = Piece(pieceType, color);
		board[sq] = Square(piece);
		
		if (pieceType != Pieces::NONE)
			bitboards.Add(piece, sq);

		col++;
	}

	// 2. Active color
	currentPlayer = (activeColor == "w") ? Color::WHITE : Color::BLACK;

	// 3. Castling rights
	whiteCastlingRights[0] = (castling.find('Q') != std::string::npos);
	whiteCastlingRights[1] = (castling.find('K') != std::string::npos);
	blackCastlingRights[0] = (castling.find('q') != std::string::npos);
	blackCastlingRights[1] = (castling.find('k') != std::string::npos);
	// If castling = "-", no rights at all

	// 4. En passant target
	moveHistory.clear();
	undoHistory.clear();
	if (enPassant != "-")
		enPassantTarget = ToIndex(ToRow(7 - (enPassant[1] - '0')), ToCol(enPassant[0] - 'a'));
	else
		enPassantTarget = -1;

	// 5 & 6. Halfmove and fullmove counters
	halfmoves = std::stoi(halfmoveStr);
	int fullmoves = std::stoi(fullmoveStr);

	zobristKey = ComputeFullHash();
	CheckKingInCheck();
}

void Engine::AppendUndoList(BoardState state, const Move move)
{
	state.zobristKey = zobristKey;
	state.promotion = GetPromotion(move);
	state.fromSquare = GetStart(move);
	state.toSquare = GetEnd(move);
	state.enPassantTarget = (enPassantTarget == -1) ? 0 : enPassantTarget;
	state.castlingRights = (whiteCastlingRights[0] << 3) | (whiteCastlingRights[1] << 2) | (blackCastlingRights[0] << 1) | (blackCastlingRights[1]);
	state.halfmoveClock = halfmoves;
	state.wasEnPassant = IsEnPassant(move);
	state.wasCastling = IsCastle(move);
	state.playerToMove = IsWhite(currentPlayer);
	undoHistory.push_back(state);
}

void Engine::UndoMove()
{
	if (undoHistory.size() <= 1) // Last move is the initial position, can't undo
	{
		invalidMove = true;
		return; // No move to undo
	}

	if (positionStack.empty()) return;

	uint64_t top = positionStack.back();
	positionStack.pop_back();

	auto it = positionCounts.find(top);
	if (it != positionCounts.end())
	{
		if (--(it->second) == 0)
			positionCounts.erase(it);
	}

	BoardState lastState = undoHistory.back();
	undoHistory.pop_back();

	Color player = lastState.playerToMove ? Color::WHITE : Color::BLACK;
	Color enemy = Opponent(player);

	zobristKey = lastState.zobristKey;

	// Restore pieces
	Piece movedPiece = Piece(static_cast<Pieces>(lastState.movedPiece), player);

	Piece capturedPiece = Piece();
	if (lastState.capturedPiece) capturedPiece = Piece(static_cast<Pieces>(lastState.capturedPiece), enemy);

	// Remove moved piece from toSquare
	// TODO: Can remove this if statement
	if (board[lastState.toSquare].GetPiece().GetType() != Pieces::NONE)
		bitboards.Remove(board[lastState.toSquare].GetPiece(), lastState.toSquare);

	// Move back moved piece
	board[lastState.fromSquare].SetPiece(movedPiece);
	bitboards.Add(movedPiece, lastState.fromSquare);

	// Captured piece
	board[lastState.toSquare].SetPiece(capturedPiece);
	if (capturedPiece.GetType() != Pieces::NONE)
		bitboards.Add(capturedPiece, lastState.toSquare);

	// Restore king position if needed
	if (movedPiece.GetType() == Pieces::KING)
		(IsWhite(player) ? whiteKingPos : blackKingPos) = lastState.fromSquare;

	// Handle en passant capture
	if (lastState.wasEnPassant)
	{
		int capSq = lastState.toSquare += IsWhite(player) ? 8 : -8; // 8 = one rank

		// Remove ghost pawn
		// TODO: Already removed, dont need this
		if (board[lastState.toSquare].GetPiece().GetType() != Pieces::NONE)
			bitboards.Remove(board[lastState.toSquare].GetPiece(), lastState.toSquare);

		board[lastState.toSquare].SetPiece(Piece(Pieces::NONE, Color::NONE));

		// Restore captured pawn
		Piece epPawn = Piece(Pieces::PAWN, enemy);
		board[capSq].SetPiece(epPawn);
		bitboards.Add(epPawn, capSq);
	}

	// Handle castling
	if (lastState.wasCastling)
	{
		bool kingside = (ToCol(lastState.toSquare) == 6);
		int row = IsWhite(player) ? 7 : 0;

		int rookStartSq = ToIndex(row, kingside ? 7 : 0);
		int rookEndSq   = ToIndex(row, kingside ? 5 : 3);

		Piece rook = board[rookEndSq].GetPiece();

		bitboards.Remove(rook, rookEndSq);
		board[rookEndSq].SetPiece(Piece(Pieces::NONE, Color::NONE));

		bitboards.Add(rook, rookStartSq);
		board[rookStartSq].SetPiece(rook);
		// King position was updated earlier
	}

	// Restore en passant target square
	if (lastState.enPassantTarget == 0)
		enPassantTarget = -1;
	else
		enPassantTarget = lastState.enPassantTarget;

	// Restore castling rights
	whiteCastlingRights[0] = (lastState.castlingRights & 0b1000) != 0;
	whiteCastlingRights[1] = (lastState.castlingRights & 0b0100) != 0;
	blackCastlingRights[0] = (lastState.castlingRights & 0b0010) != 0;
	blackCastlingRights[1] = (lastState.castlingRights & 0b0001) != 0;

	// Restore halfmove clock
	halfmoves = lastState.halfmoveClock;

	// Couldn't be in check 2 moves in a row
	// Reset if game status
	checkStatus = 0;
	invalidMove = false;
	checkmate = false;
	draw = false;

	// Remove last move from history
	if (!moveHistory.empty())
		moveHistory.pop_back();

	currentPlayer = player;
}

void Engine::MakeNullMove()
{
	// Append undo info
	BoardState state;
	state.movedPiece    = static_cast<uint8_t>(Pieces::NONE);
	state.capturedPiece = static_cast<uint8_t>(Pieces::NONE);
	AppendUndoList(state, Move());

	// Clear en passant and castling
	if (enPassantTarget != -1)
		zobristKey ^= zobrist.enPassantFile[ToCol(enPassantTarget)];
	if (whiteCastlingRights[1]) zobristKey ^= zobrist.castling[0];
	if (whiteCastlingRights[0]) zobristKey ^= zobrist.castling[1];
	if (blackCastlingRights[1]) zobristKey ^= zobrist.castling[2];
	if (blackCastlingRights[0]) zobristKey ^= zobrist.castling[3];
	enPassantTarget = -1;

	halfmoves++;

	if (halfmoves >= 50) { draw = true; }

	ChangePlayers();
	if (IsWhite(currentPlayer))
		zobristKey ^= zobrist.sideToMove;

	if (whiteCastlingRights[1]) zobristKey ^= zobrist.castling[0];
	if (whiteCastlingRights[0]) zobristKey ^= zobrist.castling[1];
	if (blackCastlingRights[1]) zobristKey ^= zobrist.castling[2];
	if (blackCastlingRights[0]) zobristKey ^= zobrist.castling[3];

	positionStack.push_back(zobristKey);
	positionCounts[zobristKey] += 1;

}

void Engine::UndoNullMove()
{
	BoardState lastState = undoHistory.back();
	undoHistory.pop_back();

	positionCounts[zobristKey] -= 1;
	positionStack.pop_back();

	ChangePlayers();
	if (IsWhite(currentPlayer))
		zobristKey ^= zobrist.sideToMove;

	if (lastState.enPassantTarget == 0)
		enPassantTarget = -1;
	else
		enPassantTarget = lastState.enPassantTarget;
	whiteCastlingRights[0] = (lastState.castlingRights & 0b1000) != 0;
	whiteCastlingRights[1] = (lastState.castlingRights & 0b0100) != 0;
	blackCastlingRights[0] = (lastState.castlingRights & 0b0010) != 0;
	blackCastlingRights[1] = (lastState.castlingRights & 0b0001) != 0;
	halfmoves = lastState.halfmoveClock;
	draw = false;

	zobristKey = lastState.zobristKey; // restore full hash
}

void Engine::UndoTurn()
{
	UndoMove();
	if (undoHistory.size() > 1)
		UndoMove();
}

void Engine::SetBot(Bot* bot)
{
	if (IsWhite(bot->GetColor()))
	{
		bots[0] = bot;
		botPlaying[0] = true;
	}
	else
	{
		bots[1] = bot;
		botPlaying[1] = true;
	}
}

void Engine::CheckKingInCheck()
{
	if (Movegen::IsSquareAttacked(whiteKingPos, Color::BLACK, bitboards)) checkStatus |= (1 << 1);
	else checkStatus &= ~(1 << 1);
	if (Movegen::IsSquareAttacked(blackKingPos, Color::WHITE, bitboards)) checkStatus |= (1 << 0);
	else checkStatus &= ~(1 << 0);
}

void Engine::UpdateCastlingRights(const Move move, const Piece movingPiece, const Piece targetPiece)
{
	// If neither side can castle, no need to check
	if (!whiteCastlingRights[0] && !whiteCastlingRights[1] &&
		!blackCastlingRights[0] && !blackCastlingRights[1])
		return;

	int startSquare = GetStart(move);
	int endSquare = GetEnd(move);

	int startRow = ToRow(startSquare);
	int startCol = ToCol(startSquare);
	int endRow = ToRow(endSquare);
	int endCol = ToCol(endSquare);

	if (movingPiece.GetType() == Pieces::KING)
	{
		if (IsWhite(currentPlayer))
		{
			whiteKingPos = endSquare;
			whiteCastlingRights[0] = false;
			whiteCastlingRights[1] = false;
		}
		else
		{
			blackKingPos = endSquare;
			blackCastlingRights[0] = false;
			blackCastlingRights[1] = false;
		}
	}
	else if (movingPiece.GetType() == Pieces::ROOK)
	{
		if (IsWhite(currentPlayer))
		{
			if (startSquare == 56) // a1 rook
				whiteCastlingRights[0] = false;
			else if (startSquare == 63) // h1 rook
				whiteCastlingRights[1] = false;
		}
		else
		{
			if (startSquare == 0) // a8 rook
				blackCastlingRights[0] = false;
			else if (startSquare == 7) // h8 rook
				blackCastlingRights[1] = false;
		}
	}
	// If rook is captured, invalidate castling rights
	if (targetPiece.GetType() == Pieces::ROOK)
	{
		if (IsWhite(currentPlayer))
		{
			if (endSquare == 0) // a8 rook
				blackCastlingRights[0] = false;
			else if (endSquare == 7) // h8 rook
				blackCastlingRights[1] = false;
		}
		else
		{
			if (endSquare == 56) // a1 rook
				whiteCastlingRights[0] = false;
			else if (endSquare == 63) // h1 rook
				whiteCastlingRights[1] = false;
		}
	}
}

// TODO: Move update functions to gamestate file?
void Engine::UpdateEnPassantSquare(const Move move)
{
	int startSquare = GetStart(move);
	int endSquare = GetEnd(move);
	const Piece movingPiece = board[endSquare].GetPiece();

	if (movingPiece.GetType() == Pieces::PAWN && abs(ToRow(endSquare) - ToRow(startSquare)) == 2) // If pawn moved 2
		enPassantTarget = startSquare + (IsWhite(movingPiece.GetColor()) ? -8 : 8);
	else // En passant opportunity only lasts for 1 turn
		enPassantTarget = -1;
}

// TODO: Can use better functions for this
void Engine::CheckCheckmate()
{
	bool hasMoves = false;

	for (int sq = 0; sq < 64; sq++)
	{
		Piece p = board[sq].GetPiece();
		if (p.GetType() == Pieces::NONE || p.GetColor() != currentPlayer) // Empty or opponent's piece
			continue;
		
		std::vector<uint8_t> moves = Movegen::GetValidMoves(sq, bitboards);
		if (!moves.empty())
		{
			hasMoves = true;
			break;
		}

		if (hasMoves) break;
	}
	if (!hasMoves)
	{
		if (!(checkStatus & 0b11)) // Stalemate
		{
			draw = true;
			return;
		}
		checkmate = true;
		std::cout << "Checkmate\n";
	}
}
