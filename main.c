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
    char algo[6] = {'L', 'R', 'U', 0, 0, 0}, shmid_str[30] = {0};
    char q[10] = {0}; q[0] = '1';
    char max[10] = {0}; max[0] = '9';
    int _max = 9;
    int frame = 5;
    shared_memory *smemory = NULL;
    int shmid = -1, status_bzip_worker, status_gcc_worker;
    pid_t bzip_worker, gcc_worker;
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
    // shared_memory
    // sizeof(char)*5*frame => sizeof(char)*5 = size of page_num, to get what frames are in main memory
    // sizeof(char)*9*atoi(q) => sizeof(char)*PAGE_NAME = size of page_num, allocate q of these since in each iteration the max amount of traces added is q
    // sizeof(char)*PAGE_NAME*frame => LRU stack
    shmid = shmget(IPC_PRIVATE, sizeof(shared_memory) + sizeof(char)*PAGE_NAME*frame + sizeof(char)*PAGE_NAME*atoi(q) + sizeof(char)*PAGE_NAME*frame, IPC_CREAT | 0666); 
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
    smemory->frames = frame;

    char* frames_array = NULL;
    // casting to char* so it is easier to get the offset right
    frames_array = (char*)((char*)smemory + sizeof(shared_memory));
    if (frames_array == NULL) {
        printf("***Frames array not created***\n");
        return 1;
    }
    // all empty
    memset(frames_array, 0,  sizeof(char)*PAGE_NAME*frame);

    char* pages_removed = NULL;
    pages_removed = (char*)((char*)smemory + sizeof(shared_memory)  + sizeof(char)*PAGE_NAME*frame);
    if (pages_removed == NULL) {
        printf("***Pages_removed array not created***\n");
        return 1;
    }
    // all empty
    memset(pages_removed, 0,  sizeof(char)*PAGE_NAME*atoi(q));

    char* stack = NULL;
    stack = (char*)((char*)smemory + sizeof(shared_memory)  + sizeof(char)*PAGE_NAME* frame +sizeof(char)*PAGE_NAME*atoi(q));
    if (stack == NULL) {
        printf("***stack not created***\n");
        return 1;
    }
    // all empty
    memset(stack, 0,  sizeof(char)*PAGE_NAME*frame);

    //initialize posix semaphores
    if (sem_init(&smemory->edit_frames, 1, 1) != 0) {
        printf("***Init Mutex Failed (edit_frames)***\n");
        return 1;
    }
    
    sprintf(shmid_str, "%d", shmid);

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

    /*
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
                    char temp_addr[9];
                    strncpy(temp_addr, (char*)indexes + 11*j, 8);
                    //printf("%s - %s\n", temp_addr, trc[i].address);
                    if (strncmp(temp_addr, trc[i].address, 8) == 0) {
                        need_to_add = false;
                        // change to write so when we remove it we 
                        // also write to disc
                        if ((indexes + 10*j)[9] == 'W') {
                            str[9] = 'W';
                        }
                    
                        // put back to last used position
                        memcpy((char*)indexes + 11, indexes, 11*j);
                        strncpy(indexes, str, 11);
                       
                        break;
                        // printf("%s\n", str);
                    }
                }
                if (need_to_add) {
                    if ((indexes + 10*(table->capacity-1))[0] != 0) {
                        // write to disc
                    }
                    if (exists_table(table, trc[i].address) == false) {
                        if (table->length >= table->capacity) {
                            // remove one
                            char temp_str[9] = {0};
                            strncpy(temp_str, (char*)(indexes + 11*(table->capacity-1)), 8);
                            // printf("(%d in %d) removing: %s ", table->length, table->capacity, temp_str);
                            // printf("%s\n", exists_table(table, temp_str)?" - exists":" - does not exists");
                            if (exists_table(table, temp_str) == false) {
                                return 1;
                            }
                            remove_table(table, temp_str);
                        }
                        if (table->length >= table->capacity) {
                            printf("Oops...\n");
                            return 1;
                        }
                        insert_table(table, trc[i].address, trc[i].action);
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
    }*/
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

    //destroy semaphores
    if (sem_destroy(&smemory->edit_frames) != 0) {
        printf("***Destroy Mutex Failed (edit_frames)***\n");
        return 1;
    }
   
    //destroy shared memory segment
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        printf("***Delete Shared Memory Failed***\n");
        return 1;
    }
    return 0;
}