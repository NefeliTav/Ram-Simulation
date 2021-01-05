#include "structs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char** argv) {
    hash_table* table = NULL;
    char algo[6] = {'L', 'R', 'U', 0, 0, 0};
    int frame = 1, q = 1, max = 0;

    // get command line arguments
    for (int i = 1; i < argc; i+=2) {
        if (strcmp(argv[i], "-a") == 0) {
            if (strcmp(argv[i+1], "LRU") != 0 && strcmp(argv[i+1], "clock") != 0) {
                printf("-a: should be either LRU or clock\n");
                return 1;
            }
            strncpy(algo, argv[i+1], 5);
        }
        if (strcmp(argv[i], "-f") == 0) {
            frame = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-q") == 0) {
            q = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-max") == 0) {
            max = atoi(argv[i+1]);
        }
    }
    
    table = create_table(10);
    insert_table(table, "hello", "world");
    insert_table(table, "hello2", "world");
    insert_table(table, "hello", "world");
    delete_table(table);

    return 0;
}