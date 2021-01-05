#include "structs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>            
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

int main(int argc, const char** argv) {
    FILE* fp;
    char* line = NULL;
    size_t len = 0, read;
    char addr[9], tmp[2], filename[FILENAME_LENGTH] = {0};
    int q = 1;
    trace* trc = NULL;

    for (int i = 1; i < argc; i+=2) {
        if (strcmp(argv[i], "-F") == 0) {
            strcpy(filename, argv[i+1]);
        }
        if (strcmp(argv[i], "-q") == 0) {
            q = atoi(argv[i+1]);
        }
    }
    if (filename[0] == 0) {
        printf("You need to give a filename using the '-F' flag.");
        return 1;
    }
    if ((fp = fopen(filename, "r")) == NULL) {
        printf("Could not open file. (%s)\n", filename);
        return 1;
    }
    trc = malloc(sizeof(trace)*q);

    int i = 0;
    while ((read = getline(&line, &len, fp)) != -1) {
        if (i >= q) {
            i = 0;
            /*for (int j = 0; j < q; j++) {  printf("%s %c\n", trc[j].address, trc[j].action); } printf("\n");*/

        }
        sscanf(line, "%s %s\n", addr, tmp);
        strcpy(trc[i].address, addr);
        trc[i].action = tmp[0];
        if (line) {
            free(line);
            line = NULL;
        }
        i++;
    }

    fclose(fp);
    if (trc) {
        free(trc);
        trc = NULL;
    }
    if (line) {
        free(line);
        line = NULL;
    }
    return 0;
}