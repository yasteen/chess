#include <stdio.h>
#include <stdlib.h>

#include "../src/chess.h"

typedef struct result_s {
    int* totals;    // [n]
    int* per_move;  // [n][20]
    int n;
} RESULT;

RESULT init_result(int n) {
    RESULT r = {.n = n,
                .totals = malloc(n * sizeof(int)),
                .per_move = malloc(n * sizeof(int) * 20)};
    return r;
}
void free_result(RESULT* result) {
    free(result->totals);
    free(result->per_move);
}

void gen_moves(CHESS* chess, int mov, int n, RESULT* results) {
    if (n < 1) return;
    MOVE_LIST moves = init_move_list();
    move_list_generate_moves(&moves, chess);

    results->totals[n - 1] += moves.length;
    results->per_move[(n - 1) * 20 + mov] += moves.length;
    for (int i = 0; i < moves.length; i++) {
        CHESS cpy;
        copy_chess(chess, &cpy);
        chess_valid_move(&cpy, moves.moves[i]);
        gen_moves(&cpy, mov, n - 1, results);
    }
}

/**
 * results is [n][20]
 *  */
int gen_moves_start(int n, RESULT* results) {
    CHESS chess = init_chess();
    MOVE_LIST moves = init_move_list();
    move_list_generate_moves(&moves, &chess);
    if (moves.length != 20) {
        printf("There should be 20 possible moves for white at turn 1; got %d",
               moves.length);
        return -1;
    }

    results->totals[n - 1] += moves.length;
    for (int i = 0; i < moves.length; i++) {
        results->per_move[(n - 1) * 20 + i] = 1;
        CHESS cpy;
        copy_chess(&chess, &cpy);
        chess_valid_move(&cpy, moves.moves[i]);
        gen_moves(&cpy, i, n - 1, results);
    }

    return 0;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        puts("Requires one argument");
        return 1;
    }

    int n = atoi(argv[1]);
    RESULT result = init_result(n);
    if (gen_moves_start(n, &result) == -1) return 1;

    CHESS c = init_chess();
    MOVE_LIST m = init_move_list();
    move_list_generate_moves(&m, &c);
    char labels[20][5];
    for (int i = 0; i < 20; i++) {
        labels[i][0] = (m.moves[i].start % 8) + 'a';
        labels[i][1] = (m.moves[i].start / 8) + '1';
        labels[i][2] = (m.moves[i].end % 8) + 'a';
        labels[i][3] = (m.moves[i].end / 8) + '1';
        labels[i][4] = '\0';
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 20; j++)
            printf("%s: %d\n", labels[j],
                   result.per_move[(n - 1 - i) * 20 + j]);
        printf("Depth: %d    Count: %d\n\n", i + 1, result.totals[n - 1 - i]);
    }

    free_result(&result);
    return 0;
}

