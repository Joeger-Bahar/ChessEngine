#include "tablebase.hpp"

extern "C" {
#include "Fathom/src/tbprobe.h"
}

#include <iostream>
#include <cstdint>

#include "core/gamestate.hpp"
#include "core/bitboard.hpp"

#define BOARD_RANK_1            0x00000000000000FFull
#define BOARD_FILE_A            0x8080808080808080ull
#define square(r, f)            (8 * (r) + (f))
#define rank(s)                 ((s) >> 3)
#define file(s)                 ((s) & 0x07)
#define board(s)                ((uint64_t)1 << (s))

inline unsigned flipSquareVertical(unsigned sq)
{
    return ((sq ^ 56) & 56) | (sq & 7);
}

struct pos
{
    uint64_t white;
    uint64_t black;
    uint64_t kings;
    uint64_t queens;
    uint64_t rooks;
    uint64_t bishops;
    uint64_t knights;
    uint64_t pawns;
    uint8_t castling;
    uint8_t rule50;
    uint8_t ep;
    bool turn;
    uint16_t move;
};

static bool parse_FEN(struct pos* pos, const char* fen)
{
    uint64_t white = 0, black = 0;
    uint64_t kings, queens, rooks, bishops, knights, pawns;
    kings = queens = rooks = bishops = knights = pawns = 0;
    bool turn;
    unsigned rule50 = 0, move = 1;
    unsigned ep = 0;
    unsigned castling = 0;
    char c;
    int r, f;

    if (fen == NULL)
        goto fen_parse_error;

    for (r = 7; r >= 0; r--)
    {
        for (f = 0; f <= 7; f++)
        {
            unsigned s = (r * 8) + f;
            uint64_t b = board(s);
            c = *fen++;
            switch (c)
            {
            case 'k':
                kings |= b;
                black |= b;
                continue;
            case 'K':
                kings |= b;
                white |= b;
                continue;
            case 'q':
                queens |= b;
                black |= b;
                continue;
            case 'Q':
                queens |= b;
                white |= b;
                continue;
            case 'r':
                rooks |= b;
                black |= b;
                continue;
            case 'R':
                rooks |= b;
                white |= b;
                continue;
            case 'b':
                bishops |= b;
                black |= b;
                continue;
            case 'B':
                bishops |= b;
                white |= b;
                continue;
            case 'n':
                knights |= b;
                black |= b;
                continue;
            case 'N':
                knights |= b;
                white |= b;
                continue;
            case 'p':
                pawns |= b;
                black |= b;
                continue;
            case 'P':
                pawns |= b;
                white |= b;
                continue;
            default:
                break;
            }
            if (c >= '1' && c <= '8')
            {
                unsigned jmp = (unsigned)c - '0';
                f += jmp - 1;
                continue;
            }
            goto fen_parse_error;
        }
        if (r == 0)
            break;
        c = *fen++;
        if (c != '/')
            goto fen_parse_error;
    }
    c = *fen++;
    if (c != ' ')
        goto fen_parse_error;
    c = *fen++;
    if (c != 'w' && c != 'b')
        goto fen_parse_error;
    turn = (c == 'w');
    c = *fen++;
    if (c != ' ')
        goto fen_parse_error;
    c = *fen++;
    if (c != '-')
    {
        do
        {
            switch (c)
            {
            case 'K':
                castling |= TB_CASTLING_K; break;
            case 'Q':
                castling |= TB_CASTLING_Q; break;
            case 'k':
                castling |= TB_CASTLING_k; break;
            case 'q':
                castling |= TB_CASTLING_q; break;
            default:
                goto fen_parse_error;
            }
            c = *fen++;
        } while (c != ' ');
        fen--;
    }
    c = *fen++;
    if (c != ' ')
        goto fen_parse_error;
    c = *fen++;
    if (c >= 'a' && c <= 'h')
    {
        unsigned file = c - 'a';
        c = *fen++;
        if (c != '3' && c != '6')
            goto fen_parse_error;
        unsigned rank = c - '1';
        ep = square(rank, file);
        if (rank == 2 && turn)
            goto fen_parse_error;
        if (rank == 5 && !turn)
            goto fen_parse_error;
        if (rank == 2 && ((tb_pawn_attacks(ep, true) & (black & pawns)) == 0))
            ep = 0;
        if (rank == 5 && ((tb_pawn_attacks(ep, false) & (white & pawns)) == 0))
            ep = 0;
    }
    else if (c != '-')
        goto fen_parse_error;
    c = *fen++;
    if (c != ' ')
        goto fen_parse_error;
    char clk[4];
    clk[0] = *fen++;
    if (clk[0] < '0' || clk[0] > '9')
        goto fen_parse_error;
    clk[1] = *fen++;
    if (clk[1] != ' ')
    {
        if (clk[1] < '0' || clk[1] > '9')
            goto fen_parse_error;
        clk[2] = *fen++;
        if (clk[2] != ' ')
        {
            if (clk[2] < '0' || clk[2] > '9')
                goto fen_parse_error;
            c = *fen++;
            if (c != ' ')
                goto fen_parse_error;
            clk[3] = '\0';
        }
        else
            clk[2] = '\0';
    }
    else
        clk[1] = '\0';
    rule50 = atoi(clk);
    move = atoi(fen);

    pos->white = white;
    pos->black = black;
    pos->kings = kings;
    pos->queens = queens;
    pos->rooks = rooks;
    pos->bishops = bishops;
    pos->knights = knights;
    pos->pawns = pawns;
    pos->castling = castling;
    pos->rule50 = rule50;
    pos->ep = ep;
    pos->turn = turn;
    pos->move = move;
    return true;

fen_parse_error:
    return false;
}

Tablebase::Tablebase(const std::string& path)
{
	ok = tb_init(path.c_str());

    if (!ok)
        throw "Failed to init Syzygy TB from ";
	else
		std::cout << "Syzygy TB initialized. Largest set: " << TB_LARGEST << "\n";
}

Tablebase::~Tablebase()
{
	tb_free();
}

bool Tablebase::Initialized() const
{
	return TB_LARGEST > 0;
}

bool Tablebase::Probeable(const BitboardBoard& board) const
{
    int allPieces = BoardCalculator::TotalPieces(board);
    bool enoughPieces = (allPieces <= (int)TB_LARGEST);
	return Initialized() && enoughPieces;
}

int Tablebase::ProbeWDL(Engine* engine) const
{
    std::string fen = engine->GetFEN();
    struct pos pos0;
    struct pos* pos = &pos0;
    if (!parse_FEN(pos, fen.c_str()))
        return false;

    unsigned result = tb_probe_root(
        pos->white,
        pos->black,
        pos->kings,
        pos->queens,
        pos->rooks,
        pos->bishops,
        pos->knights,
        pos->pawns,
        pos->rule50,
        pos->castling,
        pos->ep,
        pos->turn,
        NULL
    );

    int wdl = TB_GET_WDL(result);

	if (wdl == TB_RESULT_FAILED)
		return -99;

	if (wdl == TB_LOSS || wdl == TB_BLESSED_LOSS)
		return -1;
	else if (wdl == TB_DRAW)
		return 0;
	else if (wdl == TB_CURSED_WIN || wdl == TB_WIN)
		return +1;
}

int Tablebase::ProbeDTZ(Engine* engine) const
{
    std::string fen = engine->GetFEN();
    struct pos pos0;
    struct pos* pos = &pos0;
    if (!parse_FEN(pos, fen.c_str()))
        return false;

	unsigned result = tb_probe_root(
		pos->white,
		pos->black,
		pos->kings,
		pos->queens,
		pos->rooks,
		pos->bishops,
		pos->knights,
		pos->pawns,
		pos->rule50,
		pos->castling,
		pos->ep,
		pos->turn,
		NULL
	);

    int dtz = TB_GET_DTZ(result);

	if (dtz == 0)
		return -99;

	return dtz;
}

Move Tablebase::GetMove(Engine* engine) const
{
    std::string fen = engine->GetFEN();
    struct pos pos0;
    struct pos* pos = &pos0;
    if (!parse_FEN(pos, fen.c_str()))
        return false;

    // TB_GET_FROM, TB_GET_TO, TB_GET_PROMOTES, TB_GET_EP
    unsigned result = tb_probe_root(
        pos->white,
        pos->black,
        pos->kings,
        pos->queens,
        pos->rooks,
        pos->bishops,
        pos->knights,
        pos->pawns,
        pos->rule50,
        pos->castling,
        pos->ep,
        pos->turn,
        NULL
    );

    int from = TB_GET_FROM(result);
    int to = TB_GET_TO(result);
    int promo = TB_GET_PROMOTES(result);
    int ep = TB_GET_EP(result);

    int parsedPromo = (int)Pieces::NONE;
	if (promo == TB_PROMOTES_QUEEN) parsedPromo = (int)Pieces::QUEEN;
	else if (promo == TB_PROMOTES_ROOK) parsedPromo = (int)Pieces::ROOK;
	else if (promo == TB_PROMOTES_BISHOP) parsedPromo = (int)Pieces::BISHOP;
	else if (promo == TB_PROMOTES_KNIGHT) parsedPromo = (int)Pieces::KNIGHT;

    Move m = EncodeMove(flipSquareVertical(from), flipSquareVertical(to), parsedPromo, false, false);
    return m;
}
