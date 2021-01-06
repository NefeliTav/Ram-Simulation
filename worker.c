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
    char addr[8], tmp[2], filename[FILENAME_LENGTH] = {0};
    int q = 1, shmid = -1, max = 10;
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
        else if (strcmp(argv[i], "-max") == 0) {
            max = atoi(argv[i+1]);
        }
        else if (strcmp(argv[i], "-q") == 0) {
            q = atoi(argv[i+1]);
        }
        else if (strcmp(argv[i], "-s") == 0) {                            //get shared memory id
            shmid = atoi(argv[i + 1]);
        }
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
    trc = (trace*)((char*)smemory + sizeof(shared_memory));
    if (trc == NULL) {
        printf("***Trace array not created***\n");
        return 1;
    }

    if (filename[0] == 0) {
        printf("You need to give a filename using the '-F' flag.\n");
        return 1;
    }
    if ((fp = fopen(filename, "r")) == NULL) {
        printf("Could not open file. (%s)\n", filename);
        return 1;
    }
    if (start == true) {
        me = &(smemory->mutex_0);
        them = &(smemory->mutex_1);
    } else if (start == false) {
        me = &(smemory->mutex_1);
        them = &(smemory->mutex_0);
    }

    int i = 0;
    // divide 2 so that we read max/2 from each file = max
    max /= 2;
    while ((read = getline(&line, &len, fp)) != -1) {
        if (i >= q || max <= 0) {
            i = 0;
            // memcpy(smemory->trc, trc, sizeof(trace)*q);
            sem_post(&(smemory->read));
            sem_post(them);
            if (max <= 0) {
                break;
            }
        }
        if (i == 0) {
            sem_wait(me);
            // memset(trc, 0, sizeof(trace)*q);
            sem_wait(&smemory->can_write);
        }
        sscanf(line, "%s %s\n", addr, tmp);
        strcpy(trc[i].address, addr);
        strcpy(trc[i].action, tmp);
        if (line) {
            free(line);
            line = NULL;
        }
        i++;
        max--;
    }
    sem_post(them);

    fclose(fp);
    if (line) {
        free(line);
        line = NULL;
    }
    if (shmdt(smemory) == -1) {
        printf("***Detach Memory Failed***\n");
        return 1;
    }
    return 0;
}