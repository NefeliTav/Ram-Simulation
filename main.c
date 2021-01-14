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
    char algo[8] = {'L', 'R', 'U', 0, 0, 0, 0, 0}, shmid_str[30] = {0};
    char q[10] = {0}; q[0] = '1';
    char max[10] = {0};
    strcpy(max, "-1");
    int _max = -1;
    int frame = 5;
    shared_memory *smemory = NULL;
    int shmid = -1, status_bzip_worker, status_gcc_worker;
    pid_t bzip_worker, gcc_worker;
    unsigned int reads = 0, writes = 0, page_faults = 0;

    // get command line arguments
    for (int i = 1; i < argc; i+=2) {
        if (strcmp(argv[i], "-a") == 0) {
            if (strcmp(argv[i+1], "LRU") != 0 && strcmp(argv[i+1], "schance") != 0) {
                printf("-a: should be either LRU or schance\n");
                printf("%s\n", argv[i+1]);
                return 1;
            }
            strcpy(algo, argv[i+1]);
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
    shmid = shmget(IPC_PRIVATE, sizeof(shared_memory) + sizeof(char)*(PAGE_NAME+1)*frame + sizeof(char)*(PAGE_NAME+1)*atoi(q) + sizeof(char)*(PAGE_NAME+1)*frame, IPC_CREAT | 0666); 
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
    memset(frames_array, 0,  sizeof(char)*(PAGE_NAME+1)*frame);

    char* pages_removed = NULL;
    pages_removed = (char*)((char*)smemory + sizeof(shared_memory)  + sizeof(char)*(PAGE_NAME+1)*frame);
    if (pages_removed == NULL) {
        printf("***Pages_removed array not created***\n");
        return 1;
    }
    // all empty
    memset(pages_removed, 0,  sizeof(char)*(PAGE_NAME+1)*atoi(q));

    char* stack = NULL;
    stack = (char*)((char*)smemory + sizeof(shared_memory)  + sizeof(char)*(PAGE_NAME+1)* frame +sizeof(char)*(PAGE_NAME+1)*atoi(q));
    if (stack == NULL) {
        printf("***stack not created***\n");
        return 1;
    }
    // all empty
    memset(stack, 0,  sizeof(char)*(PAGE_NAME+1)*frame);

    //initialize posix semaphores
    if (sem_init(&smemory->mutex_0, 1, 1) != 0) {
        printf("***Init Mutex Failed***\n");
        return 1;
    }
    if (sem_init(&smemory->mutex_1, 1, 0) != 0) {
        printf("***Init Mutex Failed***\n");
        return 1;
    }
    
    sprintf(shmid_str, "%d", shmid);

    if ((bzip_worker = fork()) < 0) {
        printf("***Fork Failed***\n");
        return 1;
    } else if (bzip_worker == 0) {
        // child process
        execl("worker", "worker", "-F", "./bzip.trace", "-q", q, "-s", shmid_str, "-start", "true", "-max", max, "-a", algo, (char *)NULL); 
    }

    if ((gcc_worker = fork()) < 0) {
        printf("***Fork Failed***\n");
        return 1;
    } else if (gcc_worker == 0) {
        // child process
        execl("worker", "worker", "-F", "./gcc.trace", "-q", q, "-s", shmid_str, "-start", "false", "-max", max, "-a", algo, (char *)NULL); 
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

    printf("File %s:\nreads:\t\t\t%d\nwrites:\t\t\t%d\npage faults:\t\t%d\nwrites to disk:\t\t%d\n", smemory->filename_0, smemory->reads_0, smemory->writes_0, smemory->page_faults_0, smemory->writes_to_disk_0);
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("File %s:\nreads:\t\t\t%d\nwrites:\t\t\t%d\npage faults:\t\t%d\nwrites to disk:\t\t%d\n", smemory->filename_1, smemory->reads_1, smemory->writes_1, smemory->page_faults_1, smemory->writes_to_disk_1);
    printf("\n\n~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Global stats:\nreads:\t\t\t%d\nwrites:\t\t\t%d\npage faults:\t\t%d\nwrites to disk:\t\t%d\n", smemory->reads_0 + smemory->reads_1, smemory->writes_0 + smemory->writes_1, smemory->page_faults_0 + smemory->page_faults_1, smemory->writes_to_disk_0 + smemory->writes_to_disk_1);


    //destroy semaphores
    if (sem_destroy(&smemory->mutex_0) != 0) {
        printf("***Destroy Mutex Failed (mutex_0)***\n");
        return 1;
    }
    if (sem_destroy(&smemory->mutex_1) != 0) {
        printf("***Destroy Mutex Failed (mutex_1)***\n");
        return 1;
    }
   
    //destroy shared memory segment
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        printf("***Delete Shared Memory Failed***\n");
        return 1;
    }
    return 0;
}