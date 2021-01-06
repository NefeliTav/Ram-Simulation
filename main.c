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

#define HT_SIZE 10

int main(int argc, const char** argv) {
    hash_table* table = NULL;
    char algo[6] = {'L', 'R', 'U', 0, 0, 0}, shmid_str[30] = {0};
    char q[10] = {0}; q[0] = '1';
    char max[10] = {0}; max[0] = '9';
    int _max = 9;
    int frame = 5;
    shared_memory *smemory = NULL;
    int shmid = -1, status_bzip_worker, status_gcc_worker;
    pid_t bzip_worker, gcc_worker;
    trace* trc = NULL;
    char* indexes = NULL;
    unsigned int reads = 0, writes = 0, page_faults = 0;

    // get command line arguments
    for (int i = 1; i < argc; i+=2) {
        if (strcmp(argv[i], "-a") == 0) {
            if (strcmp(argv[i+1], "LRU") != 0 && strcmp(argv[i+1], "clock") != 0) {
                printf("-a: should be either LRU or clock\n");
                return 1;
            }
            strncpy(algo, argv[i+1], 5);
        }
        else if (strcmp(argv[i], "-f") == 0) {
            frame = atoi(argv[i+1]);
        }
        else if (strcmp(argv[i], "-q") == 0) {
            strncpy(q, argv[i+1], 9);
        }
        else if (strcmp(argv[i], "-max") == 0) {
            strncpy(max, argv[i+1], 9);
            _max = atoi(max);
        }
    }
    shmid = shmget(IPC_PRIVATE, sizeof(shared_memory) + sizeof(trace)*atoi(q), IPC_CREAT | 0666); 
    if (shmid < 0) {
        printf("***Shared Memory Failed***\n");
        return 1;
    }
    //attach to memory
    smemory = shmat(shmid, (void*)0, 0);
    if (smemory < 0) {
        printf("***Attach Shared Memory Failed***\n");
        return 1;
    }

    // casting to char* so it is easier to get the offset right
    trc = (trace*)((char*)smemory + sizeof(shared_memory));
    if (trc == NULL) {
        printf("***Trace array not created***\n");
        return 1;
    }
    
    //initialize posix semaphores
    if (sem_init(&smemory->mutex_0, 1, 1) != 0) {
        printf("***Init Mutex Failed***\n");
        return 1;
    }
    if (sem_init(&smemory->mutex_1, 1, 0) != 0) {
        printf("***Init Mutex Failed***\n");
        return 1;
    }
    if (sem_init(&smemory->read, 1, 0) != 0) {
        printf("***Init Mutex Failed***\n");
        return 1;
    }
    if (sem_init(&smemory->can_write, 1, 0) != 0) {
        printf("***Init Mutex Failed***\n");
        return 1;
    }
    
    sprintf(shmid_str, "%d", shmid);
    table = create_table(HT_SIZE, frame);

    if ((bzip_worker = fork()) < 0) {
        printf("***Fork Failed***\n");
        return 1;
    } else if (bzip_worker == 0) {
        // child process
        execl("worker", "worker", "-F", "./bzip.trace", "-q", q, "-s", shmid_str, "-start", "true", "-max", max, (char *)NULL); 
    }

    if ((gcc_worker = fork()) < 0) {
        printf("***Fork Failed***\n");
        return 1;
    } else if (gcc_worker == 0) {
        // child process
        execl("worker", "worker", "-F", "./gcc.trace", "-q", q, "-s", shmid_str, "-start", "false", "-max", max, (char *)NULL); 
    }

    
    if (strcmp(algo, "LRU") == 0) {
        // 1D array so we can memset
        indexes = malloc(11*table->capacity);
        memset(indexes, 0, 11*table->capacity);
    }
    while (true) {
        sem_post(&(smemory->can_write));
        sem_wait(&(smemory->read));
        for (int i = 0; i < atoi(q); i++) {
            if (trc[i].address[0] == 0 || trc[i].action[0] == 0) {
                sem_post(&(smemory->can_write));
                _max = 0;
                break;
            }
            // printf("%s %s\n", trc[i].address, trc[i].action);
            // printf("%d\n", table->length);
            insert_table(table, trc[i].address, trc[i].action);
            if (trc[i].action[0] == 'R') {
                reads++;
            } else if (trc[i].action[0] == 'W') {
                writes++;
            }
            if (strcmp(algo, "LRU") == 0) {
                bool need_to_add = true;
                char str[11] = {0};
                strncpy(str, trc[i].address, 8);
                str[8] = '_';
                str[9] = trc[i].action[0];
                str[10] = 0;
                // printf("%s\n", str);
                for (int j = 0; j < table->capacity; j++) {
                    if (strncmp((char*)indexes + 10*j, trc[i].address, 8) == 0) {
                        need_to_add = false;
                        // change to write so when we remove it we 
                        // also write to disc
                        if ((indexes + 10*j)[9] == 'W') {
                            str[9] = 'W';
                        }
                        /*printf("~DANCE~ %s\n", str);
                        for (int k = 0; k < 11*table->capacity; k++) {
                            if (indexes[k] == 0) {
                                printf("-");
                            }
                            printf("%c", indexes[k]);
                        }
                        printf("\n");*/
                        memcpy((char*)indexes + 11, indexes, 11*j);
                        strncpy(indexes, str, 11);
                        /*for (int k = 0; k < 11*table->capacity; k++) {
                            if (indexes[k] == 0) {
                                printf("-");
                            }
                            printf("%c", indexes[k]);
                        }
                        printf("\n");
                        printf("~~~~~~\n");
                        sleep(5);*/
                        break;
                        // printf("%s\n", str);
                    }
                }
                if (need_to_add) {
                    if ((indexes + 10*(table->capacity-1))[0] != 0) {
                        // write to disc
                    }
                    if (indexes[0] != 0) {
                        memcpy(indexes + 11, indexes, 11*(table->capacity-1));
                    }
                    memcpy(indexes, str, 10);
                    page_faults++;
                }
            }
            if (--_max <= 0) {
                break;
            }
        }
        if (_max <= 0) {
            break;
        }
        memset(trc, 0, sizeof(trace)*atoi(q));
    }
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

    free(indexes);
    delete_table(table);
    printf("Reads: %d\n", reads);
    printf("Writes: %d\n", writes);
    printf("Page faults: %d\n", page_faults);
    //destroy semaphores
    if (sem_destroy(&smemory->mutex_0) != 0) {
        printf("***Destroy 'mutex_0' Failed***\n");
        return 1;
    }
    if (sem_destroy(&smemory->mutex_1) != 0) {
        printf("***Destroy 'mutex_1' Failed***\n");
        return 1;
    }
    if (sem_destroy(&smemory->read) != 0) {
        printf("***Destroy 'read' Failed***\n");
        return 1;
    }
    if (sem_destroy(&smemory->can_write) != 0) {
        printf("***Destroy 'can_write' Failed***\n");
        return 1;
    }
    //destroy shared memory segment
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        printf("***Delete Shared Memory Failed***\n");
        return 1;
    }
    return 0;
}