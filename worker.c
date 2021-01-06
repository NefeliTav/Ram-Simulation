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
    int q = 1, shmid = -1;
    trace* trc = NULL;
    shared_memory *smemory = NULL;
    bool start = -1;
    sem_t *me = NULL, *them = NULL;

    for (int i = 1; i < argc; i+=2) {
        if (strcmp(argv[i], "-F") == 0) {
            strcpy(filename, argv[i+1]);
        }
        // the start flag is necessary
        if (strcmp(argv[i], "-start") == 0) {
            if (strcmp("true", argv[i+1]) == 0) {
                start = true;
            } else if (strcmp("false", argv[i+1]) == 0) {
                start = false;
            }
        }
        if (strcmp(argv[i], "-q") == 0) {
            q = atoi(argv[i+1]);
        }
        if (strcmp(argv[i], "-s") == 0)                             //get shared memory id
            shmid = atoi(argv[i + 1]);
    }
    if (start == -1) {
        printf("Please provide a boolean indicator for the first and second process \
            using the '-start' flag, (eg. -start true, -start false)\n");
        return 1;
    }
    smemory = shmat(shmid, NULL, 0);                                //attach to memory
    if (smemory < 0) {
        printf("***Attach Failed***\n");
        return 1;
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
    if (start == true) {
        me = &(smemory->mutex_1);
        them = &(smemory->mutex_0);
    } else if (start == false){
        me = &(smemory->mutex_0);
        them = &(smemory->mutex_1);
    }

    int i = 0;
    while ((read = getline(&line, &len, fp)) != -1) {
        if (i >= q) {
            i = 0;
            /*for (int j = 0; j < q; j++) {  printf("%s %c\n", trc[j].address, trc[j].action); } printf("\n");*/
            sem_post(them);
        }
        if (i == 0) {
            sem_wait(me);
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