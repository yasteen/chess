#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chess.h"

void print_chess_board(char board[64], int is_number) {
    for (int y = 7; y >= 0; y--) {
        for (int x = 0; x < 8; x++) {
            putc((is_number ? '0' : 0) + board[8 * y + x], stdout);
            putc(' ', stdout);
        }
        putc('\n', stdout);
    }
}

void read_chess_file(char* filename, CHESS* chess, MOVE_LIST* list) {
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not read file `%s`\n", filename);
        exit(1);
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        move_list_clear(list);
        move_list_generate_moves(list, chess);
        len = strnlen(line, len);

        // Make move
        if (len == 1 && line[0] == '\n') {
        } else if (len < 5 || len > 6) {
            printf("[ERROR]: Invalid command %s; len: %lu\n", line, len);
        } else {
            chess_move(chess, list, line);
        }
    }
    fclose(fp);
    if (line) free(line);
}

int main(int argc, char** argv) {
    if (argc > 2) {
        puts("[USAGE]: cli [FILENAME?]");
        return 1;
    }

    CHESS chess = init_chess();
    MOVE_LIST list = init_move_list();

    if (argc == 2) {
        read_chess_file(argv[1], &chess, &list);
    }

    print_chess_board(chess.board, 0);

    for (int i = 0; i < 10000; i++) {
        move_list_clear(&list);
        move_list_generate_moves(&list, &chess);

        char* line = NULL;
        size_t len;
        printf("> ");
        getline(&line, &len, stdin);
        len = strnlen(line, len);

        // Print moves
        if (len == 2) {
            if (line[0] == 'm') {
                print_move_list(&list);
                i--;
            } else if (line[0] == 't') {
                puts(chess.turn == 1 ? "Turn: white" : "Turn: black");
            } else if (line[0] == 'a') {
                print_chess_board(
                    chess.turn == 1 ? chess.b_attacking : chess.w_attacking, 1);
            } else {
                puts("Invalid command");
            }
            continue;
        }

        // Make move
        if (len < 5 || len > 6) {
            printf("[ERROR]: Invalid command %s\n", line);
        } else {
            chess_move(&chess, &list, line);
        }

        print_chess_board(chess.board, 0);
        free(line);
    }
    return 0;
}

