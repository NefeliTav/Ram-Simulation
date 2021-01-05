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
    hash_table* table = NULL;
    char algo[6] = {'L', 'R', 'U', 0, 0, 0};
    char q[10] = {0}; q[0] = '1';
    int frame = 1, max = 0;
    shared_memory *smemory = NULL;
    int shmid = -1, status_bzip_worker, status_gcc_worker;
    pid_t bzip_worker, gcc_worker;

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
            strncpy(q, argv[i+1], 9);
        }
        if (strcmp(argv[i], "-max") == 0) {
            max = atoi(argv[i+1]);
        }
    }

    shmid = shmget(IPC_PRIVATE, sizeof(shared_memory), IPC_CREAT | 0666); 
    if (shmid < 0) {
        printf("***Shared Memory Failed***\n");
        return 1;
    }
    smemory = shmat(shmid, NULL, 0);                                                 //attach to memory
    if (smemory < 0) {
        printf("***Attach Shared Memory Failed***\n");
        return 1;
    }
    //initialize posix semaphores
    if (sem_init(&smemory->mutex_0, 1, 1) != 0) {
        printf("***Init Mutex Failed***\n");
        return 1;
    }
    if (sem_init(&smemory->mutex_1, 1, 1) != 0) {
        printf("***Init Mutex Failed***\n");
        return 1;
    }

    if ((bzip_worker = fork()) < 0) {
        printf("***Fork Failed***\n");
        return 1;
    } else if (bzip_worker == 0) {
        // child process
        execl("worker", "worker", "-F", "./bzip.trace", "-q", q, (char *)NULL); 
    }

    if ((gcc_worker = fork()) < 0) {
        printf("***Fork Failed***\n");
        return 1;
    } else if (gcc_worker == 0) {
        // child process
        execl("worker", "worker", "-F", "./gcc.trace", "-q", q, (char *)NULL); 
    }

    table = create_table(2);

    delete_table(table);

    do {
        //wait for children to finish
        if (waitpid(bzip_worker, &status_bzip_worker, WUNTRACED | WCONTINUED) == -1) {
            return -1;
        }
        if (waitpid(gcc_worker, &status_gcc_worker, WUNTRACED | WCONTINUED) == -1) {
            return -1;
        }
    } while (   
                !WIFEXITED(status_bzip_worker) && !WIFSIGNALED(status_bzip_worker) && 
                !WIFEXITED(status_gcc_worker) && !WIFSIGNALED(status_gcc_worker)
            );

    return 0;
}