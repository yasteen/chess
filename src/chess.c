#include "chess.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helpers.h"

int dir_offsets[8] = {8, 9, 1, -7, -8, -9, -1, 7};
int num_squares_to_edge[64][8];
int CHESS_CONSTANTS_INITIALIZED = 0;
int KNIGHT_MOVE[8][2] = {
    {-2, -1}, {-2, 1}, {2, -1}, {2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2},
};

static inline int is_owner(char p, int owner);
static inline void chess_update_castle_state(CHESS* chess, MOVE move);
static inline void chess_update_attacking_squares(CHESS* chess);

static inline void move_list_generate_sliding_moves(MOVE_LIST* move_list,
                                                    CHESS* chess,
                                                    int start_square);
static inline void move_list_generate_knight_moves(MOVE_LIST* move_list,
                                                   CHESS* chess,
                                                   int start_square);
static inline void move_list_generate_pawn_moves(MOVE_LIST* move_list,
                                                 CHESS* chess,
                                                 int start_square);
static inline void move_list_generate_king_moves(MOVE_LIST* move_list,
                                                 CHESS* chess,
                                                 int start_square);
static inline void move_list_add_verify(MOVE_LIST* move_list, CHESS* chess,
                                        int start, int end);
static inline void move_list_add_pawn(MOVE_LIST* move_list, CHESS* chess,
                                      int start, int end, char do_promote);

static inline void init_chess_constants();
// INIT
CHESS init_chess() {
    if (!CHESS_CONSTANTS_INITIALIZED) init_chess_constants();

    CHESS chess = {
        .board =
            "RNBQKBNRPPPPPPPP................................pppppppprnbqkbnr",
        .w_attacking = {0},
        .b_attacking = {0},
        .under_check = 0,
        .turn = 1,
        .castle = 0b1111,
        .K = 60,
        .k = 4,
        .P = 0,
        .p = 0};
    return chess;
}
MOVE init_move(int start, int end, char promote) {
    MOVE move = {.start = start, .end = end, .promote = promote};
    return move;
}
MOVE_LIST init_move_list() {
    MOVE_LIST list = {.moves = {{0}}, .length = 0};
    return list;
}

void copy_chess(CHESS* old_board, CHESS* new_board) {
    memcpy(new_board, old_board, sizeof(CHESS));
}

int chess_move(CHESS* chess, MOVE_LIST* move_list, char* move_str) {
    MOVE move = {
        .start =
            ((move_str[0] - 'a') & 0xff) + 8 * ((move_str[1] - '1') & 0xff),
        .end = ((move_str[2] - 'a') & 0xff) + 8 * ((move_str[3] - '1') & 0xff),
        .promote = move_str[4] == '\n' ? '\0' : move_str[4]};
    if (move.start > 63 || move.end > 63) {
        printf("[ERROR]: Invalid command %s\n", move_str);
        return 0;
    }

    if (!is_owner(chess->board[move.start], chess->turn)) {
        printf("[ERROR]: Current turn: %s; Tried %s, but %c%c is %c\n",
               chess->turn == 1 ? "white" : "black", move_str, move_str[0],
               move_str[1], chess->board[move.start]);
        return 0;
    }

    int is_valid_move = 0;
    for (int i = 0; i < move_list->length; i++) {
        MOVE* valid_mv = &move_list->moves[i];
        if (valid_mv->start == move.start && valid_mv->end == move.end &&
            valid_mv->promote == move.promote) {
            is_valid_move = 1;
            break;
        }
    }
    if (!is_valid_move) {
        printf("[ERROR]: Move %s is not valid\n", move_str);
        return 0;
    }

    // add special cases here
    chess_valid_move(chess, move);

    return 1;
}

void chess_valid_move(CHESS* chess, MOVE move) {
    chess_update_castle_state(chess, move);

    if ((chess->board[move.start] == 'p' || chess->board[move.start] == 'P')) {
        if ((move.start / 8) != (move.end / 8) &&
            // En passant
            (move.start % 8) != (move.end % 8) &&
            chess->board[move.end] == NON) {
            int x_diff = move.end % 8 - move.start % 8;
            chess->board[(move.start / 8) * 8 + (move.start % 8) + x_diff] =
                NON;
        } else if (move.start == move.end + 16 || move.end == move.start + 16) {
            // Mark en-passantable
            char* p = chess->turn == 1 ? &chess->P : &chess->p;
            *p |= (0b1 << (7 - (move.start % 8)));
        }
    }

    if ((chess->board[move.start] == 'K' && move.start == 4) ||
        (chess->board[move.start] == 'k' && move.start == 60)) {
        if (move.end == move.start + 2) {
            chess->board[move.start + 1] = chess->board[move.start + 3];
            chess->board[move.start + 3] = NON;
        } else if (move.end == move.start - 2) {
            chess->board[move.start - 1] = chess->board[move.start - 4];
            chess->board[move.start - 4] = NON;
        }
    }

    chess->board[move.end] = chess->board[move.start];
    chess->board[move.start] = NON;

    char* other_p = chess->turn == 1 ? &chess->p : &chess->P;
    *other_p = 0;

    chess->turn *= -1;
}

void move_list_generate_moves(MOVE_LIST* move_list, CHESS* chess) {
    chess_update_attacking_squares(chess);
    for (int i = 0; i < 64; i++) {
        char p = chess->board[i];
        if (is_owner(p, chess->turn)) {
            if (p == 'K' || p == 'k') {
                move_list_generate_king_moves(move_list, chess, i);
            } else if (p == 'Q' || p == 'q' || p == 'R' || p == 'r' ||
                       p == 'B' || p == 'b') {
                move_list_generate_sliding_moves(move_list, chess, i);
            } else if (p == 'N' || p == 'n') {
                move_list_generate_knight_moves(move_list, chess, i);
            } else if (p == 'P' || p == 'p') {
                move_list_generate_pawn_moves(move_list, chess, i);
            }
        }
    }
}

void move_list_add(MOVE_LIST* move_list, int start, int end) {
    if (move_list->length + 1 > MOVE_LIST_MAX) {
        puts("Move list reached maximum length.");
        exit(1);
    }
    move_list->moves[move_list->length] = init_move(start, end, 0);
    move_list->length++;
}

static inline void move_list_add_verify(MOVE_LIST* move_list, CHESS* chess,
                                        int start, int end) {
    char* attacking =
        chess->turn == 1 ? chess->b_attacking : chess->w_attacking;
    char king = chess->turn == 1 ? 'K' : 'k';
    if (attacking[start] >= 64 &&  // attacking[start] < 128 &&
        attacking[end] != attacking[start])
        return;  // if cur piece is pinned
    // if king moves to dangerous spot
    if (chess->board[start] == king && attacking[end] != 0) return;
    // if king is under check and we try to ignore it
    if (chess->under_check && chess->board[start] != king &&
        attacking[end] != 3)
        return;
    // TODO: Verify en passant pinning

    move_list_add(move_list, start, end);
}

void move_list_add_pawn(MOVE_LIST* move_list, CHESS* chess, int start, int end,
                        char do_promote) {
    if ((do_promote && move_list->length + 4 > MOVE_LIST_MAX)) {
        puts("Move list reached maximum length.");
        exit(1);
    }
    if (do_promote) {
        move_list->moves[move_list->length] = init_move(start, end, 'q');
        move_list->moves[move_list->length + 1] = init_move(start, end, 'r');
        move_list->moves[move_list->length + 2] = init_move(start, end, 'b');
        move_list->moves[move_list->length + 3] = init_move(start, end, 'n');
        move_list->length += 4;
    } else {
        move_list_add_verify(move_list, chess, start, end);
    }
}

void move_list_clear(MOVE_LIST* move_list) { move_list->length = 0; }

void print_move_list(MOVE_LIST* list) {
    printf("\033[36mNumber of moves: %d\n", list->length);
    for (int i = 0; i < list->length; i++) {
        printf("%c%c%c%c%c\t", (list->moves[i].start % 8) + 'a',
               list->moves[i].start / 8 + '1', (list->moves[i].end % 8) + 'a',
               list->moves[i].end / 8 + '1', list->moves[i].promote);
    }
    printf("\033[0m\n");
}

// HELPERS

static inline int is_owner(char p, int owner) {
    if (p >= 'a' && p <= 'z') return -1 == owner;
    if (p >= 'A' && p <= 'Z') return 1 == owner;
    return 0;
}

static inline void chess_update_castle_state(CHESS* chess, MOVE move) {
    char c = chess->board[move.start];
    if (c == 'K')
        chess->castle &= 0b0011;
    else if (c == 'k')
        chess->castle &= 0b1100;
    else if (c == 'R') {
        if (move.start == 0)
            chess->castle &= 0b0111;
        else if (move.start == 7)
            chess->castle &= 0b1011;
    } else if (c == 'r') {
        if (move.start == 56)
            chess->castle &= 0b1101;
        else if (move.start == 63)
            chess->castle &= 0b1110;
    }
}

static inline void chess_update_attacking_squares(CHESS* chess) {
    // TODO: double check that this is okay
    chess->under_check = 0;

    char* attacking =
        chess->turn == 1 ? chess->b_attacking : chess->w_attacking;
    char king = chess->turn == 1 ? 'K' : 'k';
    for (int i = 0; i < 64; i++) attacking[i] = 0;
    for (int i = 0; i < 64; i++) {
        if (is_owner(chess->board[i], -chess->turn)) {
            char c = chess->board[i];
            if (c == 'K' || c == 'k')
                for (int dir = 0; dir < 8; dir++) {
                    if (num_squares_to_edge[i][dir] > 0 &&
                        attacking[i + dir_offsets[dir]] == 0)
                        attacking[i + dir_offsets[dir]] = 1;
                }
            else if (c == 'N' || c == 'n') {
                for (int dir = 0; dir < 8; dir++) {
                    int x = i % 8 + KNIGHT_MOVE[dir][0];
                    int y = i / 8 + KNIGHT_MOVE[dir][1];
                    if (x >= 0 && x <= 7 && y >= 0 && y <= 7 &&
                        attacking[x + 8 * y] == 0)
                        attacking[x + 8 * y] = 1;
                    if (chess->board[x + 8 * y] == king) {
                        attacking[i] = 3;
                        chess->under_check = 1;
                    }
                }
            } else if (c == 'P' || c == 'p') {
                int dir = chess->turn == 1 ? 4 : 0;  // reversed for pawns
                int forward_i = i + dir_offsets[dir];
                if (i % 8 != 7) {
                    if (attacking[forward_i + 1] == 0)
                        attacking[forward_i + 1] = 1;
                    if (chess->board[forward_i + 1] == king) {
                        attacking[i] = 3;
                        chess->under_check = 1;
                    }
                }
                if (i % 8 != 0) {
                    if (attacking[forward_i - 1] == 0)
                        attacking[i + dir_offsets[dir] - 1] = 1;
                    if (chess->board[forward_i - 1] == king) {
                        attacking[i] = 3;
                        chess->under_check = 1;
                    }
                }
            } else {
                int start_dir_index = (c == 'B' || c == 'b') ? 1 : 0;
                int dir_increment = (c == 'Q' || c == 'q') ? 1 : 2;

                for (int dir = start_dir_index; dir < 8; dir += dir_increment) {
                    int critical_piece = -1;  // check or pin
                    int is_critical = 0;
                    for (int n = 0; n < num_squares_to_edge[i][dir]; n++) {
                        int target_square = i + dir_offsets[dir] * (n + 1);
                        char target_piece = chess->board[target_square];
                        if (target_piece != NON) {
                            if (target_piece == king) {
                                is_critical = 1;
                                if (critical_piece == -1) {
                                    chess->under_check = 1;
                                    critical_piece = target_square;
                                    for (n++; n < num_squares_to_edge[i][dir];
                                         n++) {
                                        attacking[i + dir_offsets[dir] *
                                                          (n + 1)] = 2;
                                    }
                                    if (!attacking[target_square])
                                        attacking[target_square] = 1;
                                }
                                break;
                            }
                            if (critical_piece != -1) {
                                critical_piece = -1;
                                break;
                            }
                            critical_piece = target_square;
                            if (!attacking[target_square])
                                attacking[target_square] = 1;
                        } else {
                            if (critical_piece == -1)
                                if (!attacking[target_square])
                                    attacking[target_square] = 1;
                        }
                    }
                    if (!is_critical) continue;
                    int attacking_val =
                        chess->board[critical_piece] == king ? 3 : i + 64;

                    for (int n = 0; n <= num_squares_to_edge[i][dir]; n++) {
                        int target_square = i + dir_offsets[dir] * n;
                        attacking[target_square] = attacking_val;
                        if (target_square == critical_piece) break;
                    }
                }
            }
        }
    }
}

static inline void move_list_generate_sliding_moves(MOVE_LIST* move_list,
                                                    CHESS* chess,
                                                    int start_square) {
    char p = chess->board[start_square];
    int start_dir_index = (p == 'B' || p == 'b') ? 1 : 0;
    int dir_increment = (p == 'Q' || p == 'q') ? 1 : 2;

    for (int i = start_dir_index; i < 8; i += dir_increment) {
        for (int n = 0; n < num_squares_to_edge[start_square][i]; n++) {
            int target_square = start_square + dir_offsets[i] * (n + 1);
            int piece_on_target = chess->board[target_square];

            if (is_owner(piece_on_target, chess->turn)) break;
            move_list_add_verify(move_list, chess, start_square, target_square);
            if (is_owner(piece_on_target, -chess->turn)) break;
        }
    }
}

static inline void move_list_generate_knight_moves(MOVE_LIST* move_list,
                                                   CHESS* chess,
                                                   int start_square) {
    for (int i = 0; i < 8; i++) {
        int x = start_square % 8 + KNIGHT_MOVE[i][0];
        int y = start_square / 8 + KNIGHT_MOVE[i][1];
        int potential_i = 8 * y + x;
        if (x < 0 || x > 7 || y < 0 || y > 7 ||
            is_owner(chess->board[potential_i], chess->turn))
            continue;
        move_list_add_verify(move_list, chess, start_square, potential_i);
    }
}

static inline void move_list_generate_pawn_moves(MOVE_LIST* move_list,
                                                 CHESS* chess,
                                                 int start_square) {
    int direction = chess->turn == 1 ? 0 : 4;
    int dir_offset = dir_offsets[direction];
    int can_promote = num_squares_to_edge[start_square][direction] == 1;

    if (num_squares_to_edge[start_square][direction] >= 1) {
        if (chess->board[start_square + dir_offset] == NON) {
            // Move forward
            move_list_add_pawn(move_list, chess, start_square,
                               start_square + dir_offset, can_promote);
            // Double move forward
            int opposite = (direction + 4) % 8;
            if (num_squares_to_edge[start_square][opposite] == 1 &&
                chess->board[start_square + dir_offset * 2] == NON) {
                move_list_add_verify(move_list, chess, start_square,
                                     start_square + dir_offset * 2);
            }
        }

        char p = chess->turn == 1 ? chess->p : chess->P;

        // Capture
        int x = start_square % 8;
        if (x != 7) {
            if (is_owner(chess->board[start_square + dir_offset + 1],
                         -chess->turn))
                move_list_add_pawn(move_list, chess, start_square,
                                   start_square + dir_offset + 1, can_promote);
            else if (num_squares_to_edge[start_square][direction] == 3 &&
                     ((p >> (7 - (x + 1))) & 0b1))
                move_list_add_verify(move_list, chess, start_square,
                                     start_square + dir_offset + 1);
        }
        if (x != 0) {
            if (is_owner(chess->board[start_square + dir_offset - 1],
                         -chess->turn))
                move_list_add_pawn(move_list, chess, start_square,
                                   start_square + dir_offset - 1, can_promote);
            else if (num_squares_to_edge[start_square][direction] == 3 &&
                     ((p >> (7 - (x - 1))) & 0b1))
                move_list_add_verify(move_list, chess, start_square,
                                     start_square + dir_offset - 1);
        }
    }
}

static inline void move_list_generate_king_moves(MOVE_LIST* move_list,
                                                 CHESS* chess,
                                                 int start_square) {
    char* attacking =
        chess->turn == 1 ? chess->b_attacking : chess->w_attacking;
    for (int i = 0; i < 8; i++) {
        if (num_squares_to_edge[start_square][i] > 0 &&
            !is_owner(chess->board[start_square + dir_offsets[i]],
                      chess->turn) &&
            attacking[start_square + dir_offsets[i]] == 0) {
            move_list_add_verify(move_list, chess, start_square,
                                 start_square + dir_offsets[i]);
        }
    }

    // Castling
    int castle_lr_offset = chess->turn == 1 ? 3 : 1;
    char correct_rook = chess->turn == 1 ? 'R' : 'r';
    if (((chess->castle >> castle_lr_offset) & 0b1) &&
        chess->board[start_square + dir_offsets[2]] == NON &&
        chess->board[start_square + dir_offsets[2] * 2] == NON &&
        chess->board[start_square + dir_offsets[2] * 3] == correct_rook
        // TODO: And chess->board[start_square + dir_offsets[2]] is not attacked
    )
        move_list_add_verify(move_list, chess, start_square,
                             start_square + dir_offsets[2] * 2);

    if ((chess->castle >> (castle_lr_offset - 1)) & 0b1 &&
        chess->board[start_square + dir_offsets[6]] == NON &&
        chess->board[start_square + dir_offsets[6] * 2] == NON &&
        chess->board[start_square + dir_offsets[6] * 3] == NON &&
        chess->board[start_square + dir_offsets[6] * 4] == correct_rook
        // TODO: And chess->board[start_square + dir_offsets[6]] is not attacked
    )
        move_list_add_verify(move_list, chess, start_square,
                             start_square + dir_offsets[6] * 2);
}

static inline void init_chess_constants() {
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            num_squares_to_edge[rank * 8 + file][0] = 7 - rank;
            num_squares_to_edge[rank * 8 + file][1] = MIN(7 - rank, 7 - file);
            num_squares_to_edge[rank * 8 + file][2] = 7 - file;
            num_squares_to_edge[rank * 8 + file][3] = MIN(7 - file, rank);
            num_squares_to_edge[rank * 8 + file][4] = rank;
            num_squares_to_edge[rank * 8 + file][5] = MIN(rank, file);
            num_squares_to_edge[rank * 8 + file][6] = file;
            num_squares_to_edge[rank * 8 + file][7] = MIN(file, 7 - rank);
        }
    }
}
