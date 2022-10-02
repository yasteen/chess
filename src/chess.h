#ifndef CHESS_H
#define CHESS_H

#define NON '.'
#define MOVE_LIST_MAX 200

/**
 * @brief Offset on chess board starting from UP, going clockwise
 *
 */
int dir_offsets[8];
int num_squares_to_edge[64][8];
int CHESS_CONSTANTS_INITIALIZED;

typedef struct chess_s {
    char board[64];
    // 1: targeted; 2: pinned to king; 3: stops check
    char w_attacking[64];
    char b_attacking[64];
    int under_check;
    int turn;
    char castle;  // XXXX: white castle L/R; black castle L/R
    char K, k;    // Index of each king 00XXXXXX
    char P, p;  // P/p: pawn can be en passant-ed. Rightmost = 0 index, leftmost
                // = 7 index
} CHESS;

typedef struct move_s {
    int start;
    int end;
    char promote;
} MOVE;

typedef struct move_list_s {
    MOVE moves[MOVE_LIST_MAX];
    int length;
} MOVE_LIST;

CHESS init_chess();
MOVE init_move(int start, int end, char promote);
MOVE_LIST init_move_list();

void copy_chess(CHESS* old_board, CHESS* new_board);
int chess_move(CHESS* chess, MOVE_LIST* move_list, char* move_str);
void chess_valid_move(CHESS* chess, MOVE move);
void update_attacking_squares(CHESS* chess);

void move_list_generate_moves(MOVE_LIST* move_list, CHESS* chess);
void move_list_add(MOVE_LIST* move_list, int start, int end);
void move_list_clear(MOVE_LIST* move_list);

void print_move_list(MOVE_LIST* list);

#endif
