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
//#define DEBUG

#if defined(DEBUG)
// for debugging this is public
char filename[FILENAME_LENGTH] = {0};
#endif

int add_frame(char* array, unsigned int size, const char* page);
int remove_page(const char* algo, char* array, unsigned int size, hash_table* table, char* removed, int removed_size, char* stack, int stack_size, const char* page);
void stack_push(char* stack, int size, char* page);
void stack_pop(char* stack, int size, char* str);
int lines_in_file(const char* filen);

int main(int argc, const char** argv) {
    FILE* fp;
    char* line = NULL;
    size_t len = 0, read;
    char addr[9], tmp[2], page_num[6] = {0}, offset[4] = {0};
    int q = 1, shmid = -1, max = 1000000;
    shared_memory *smemory = NULL;
    bool start = -1;
    int lines_max = max;
    sem_t *me = NULL, *them = NULL;
    hash_table* table = NULL;
    #if !defined(DEBUG)
    char filename[FILENAME_LENGTH] = {0};
    #endif

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
            if (atoi(argv[i+1]) > 0){
                max = atoi(argv[i+1]);
            }
        }
        else if (strcmp(argv[i], "-q") == 0) {
            q = atoi(argv[i+1]);
        }
        else if (strcmp(argv[i], "-s") == 0) {                            //get shared memory id
            shmid = atoi(argv[i + 1]);
        }
    }
    // to print progress
    lines_max = lines_in_file(filename);
    if (max < lines_max) {
        lines_max = max;
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
    //attach to memory
    smemory = shmat(shmid, (void*)0, 0);
    if (smemory < 0) {
        printf("***Attach Shared Memory Failed***\n");
        return 1;
    }

    char* frames_array = NULL;
    frames_array = (char*)((char*)smemory + sizeof(shared_memory));
    if (frames_array == NULL) {
        printf("***Frames array not created***\n");
        return 1;
    }
    char* pages_removed = NULL;
    pages_removed = (char*)((char*)smemory + sizeof(shared_memory)  + sizeof(char)*(PAGE_NAME+1)*smemory->frames);
    if (pages_removed == NULL) {
        printf("***Pages_removed array not created***\n");
        return 1;
    }
    char* stack = NULL;
    stack = (char*)((char*)smemory + sizeof(shared_memory)  + sizeof(char)*(PAGE_NAME+1)*smemory->frames +sizeof(char)*(PAGE_NAME+1)*q);
    if (stack == NULL) {
        printf("***stack not created***\n");
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
    table =  create_table(smemory->frames);

    int i = 0;
    int count = 0;
    // divide 2 so that we read max/2 from each file = max
    max /= 2;
    while ((read = getline(&line, &len, fp)) != -1) {
        printf("%s: %d/%d\n", filename, count, lines_max);
        if (i >= q || max <= 0) {
            i = 0;
            // memcpy(smemory->trc, trc, sizeof(trace)*q);
            // sem_post(&(smemory->read));
            // sem_post(them);
            sem_post(them);

            if (max <= 0) {
                break;
            }
        }
        if (i == 0) {
            sem_wait(me);
            #if defined(DEBUG)
            printf("%s\n", filename);
            sleep(0.5);
            #endif
            if (pages_removed[0] != 0) {
                // Some of my pages were removed
                char str[PAGE_NAME] = {0};
                // other process removed my pages
                stack_pop(pages_removed, q, &str[0]);
                while (str[0] != 0) {
                    remove_table(table, str);
                    // printf("REMOVED: %s\n", str);
                    stack_pop(pages_removed, q, &str[0]);
                }
            }
            // sem_wait(me);
            // sem_wait(&smemory->can_write);
            // memset(trc, 0, sizeof(trace)*q);
        }
        sscanf(line, "%s %s\n", addr, tmp);
        // get page number and offset
        strncpy(page_num, addr, 5);
        strncpy(offset, addr+5, 3);
        printf("%s: %s%s %c\n", filename, page_num, offset, tmp[0]);
        #if defined(DEBUG)
        printf("%s: %s%s %c\n", filename, page_num, offset, tmp[0]);
        #endif
        if (exists_table(table, page_num)) {
            // do nothing ?!
        } else {
            int f = add_frame(frames_array, smemory->frames, page_num);
            if (f == -1) {
                // printf("%s: %s%s %c\n", filename, page_num, offset, tmp[0]);
                // create some space
                f = remove_page("LRU", frames_array, smemory->frames, table, pages_removed, q, stack, smemory->frames, page_num);
            }
            insert_table(table, page_num, f, tmp[0]);
            stack_push(stack, smemory->frames, page_num);

        }
        if (line) {
            free(line);
            line = NULL;
        }
        i++;
        count++;
        max--;
    }
    sem_post(them);
    delete_table(table);
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

// check for empty frames
int add_frame(char* array, unsigned int size, const char* page) {
    for (int i = 0; i < size; i++) {
        if (strlen(&array[i*PAGE_NAME]) == 0) { // empty
            strcpy(&array[i*PAGE_NAME], page);
            return i;
        }
    }
    // frames are full
    return -1;
}

void stack_pop(char* stack, int size, char* str) {
    memset(str, 0, PAGE_NAME);
    int i = 0;
    while (i <= size) {
        strncpy(str, (char*)(stack + PAGE_NAME*(size - i)), PAGE_NAME-1);
        if (str[0] != 0) {
            break;
        }
        i++;
    }
    if (i <= size && str[0] != 0) {
        memset((char*)(stack + PAGE_NAME*(size - i)), 0, PAGE_NAME-1);
    }
    return;
}


void stack_push(char* stack, int size, char* page) {
    bool need_to_add = true;
    // printf("%s\n", str);
    for (int j = 0; j < size; j++) {
        char temp_addr[PAGE_NAME] = {0};
        strncpy(temp_addr, (char*)stack + PAGE_NAME*j, PAGE_NAME-1);
        if (temp_addr[0] == 0) {
            break;
        }
        if (strncmp(temp_addr, page, PAGE_NAME-1) == 0) {
            need_to_add = false;
        
            // put back to last used position
            memcpy((char*)stack + PAGE_NAME, stack, PAGE_NAME*j);
            strncpy(stack, page, PAGE_NAME - 1);
           
            break;
            // printf("%s\n", str);
        }
    }
    if (need_to_add) {
        // assume that their is space
        if (false /*table->length >= table->capacity*/) {
            // remove one
            char temp_str[PAGE_NAME] = {0};
            strncpy(temp_str, (char*)(stack + PAGE_NAME*(size-1)), PAGE_NAME-1);
            // printf("(%d in %d) removing: %s ", table->length, table->capacity, temp_str);
            // printf("%s\n", exists_table(table, temp_str)?" - exists":" - does not exists");
            /*if (exists_table(table, temp_str) == false) {
                return;
            }*/
            // remove_table(table, temp_str); TODO: somewhere else
        }
        // insert_table(table, trc[i].address, trc[i].action); TODO: somewhere else
        if (stack[0] != 0) {
            memcpy(stack + PAGE_NAME, stack, PAGE_NAME*(size-1));
        }
        memcpy(stack, page, PAGE_NAME-1);
                   
        // page_faults++;
    }
}

int remove_page(const char* algo, char* array, unsigned int size, hash_table* table, char* removed, int removed_size, char* stack, int stack_size, const char* page) {
    char page_to_remove[PAGE_NAME];
    int page_num = -1;
    if (strcmp(algo, "LRU") == 0) {
        // test remove first
        #if defined(DEBUG)
            printf("(%s) STACK: ", filename);
            for (int j = 0; j < stack_size; j++) {
                char temp_addr[PAGE_NAME] = {0};
                strncpy(temp_addr, (char*)stack + PAGE_NAME*j, PAGE_NAME-1);
                if (temp_addr[0] == 0) {
                    break;
                }
                printf("%s ", temp_addr);
            }
            printf("\n");
        #endif
        stack_pop(stack, stack_size, page_to_remove);
        if (page_to_remove[0] == 0) {
            printf("You should not be here...\n");
        }
        #if defined(DEBUG)
        printf("REMOVE: %s\n", page_to_remove);
        printf("~~~~~~~~~~~~~~~\n");
        #endif
    } else if (strcmp(algo, "clock") == 0) {
        
    } else {
        printf("ERROR: %s\n", algo);
        exit(-1);
    }
    for (int i = 0; i < size; i++) {
        #if defined(DEBUG)
        printf("(%s__%s)\n", page_to_remove, &array[i*PAGE_NAME]);
        #endif
        if (strncmp(page_to_remove, &array[i*PAGE_NAME], PAGE_NAME-1) == 0) {
            page_num = i;
            break;
        }
    }
    #if defined(DEBUG)
    printf("\n");
    #endif
    if (page_num < 0) {
        printf("ERROR: %s Page not found in main memory\n", page_to_remove);
        exit(-1);
    }
    if (exists_table(table, page_to_remove)) {
        remove_table(table, page_to_remove);
    } else {
        stack_push(removed, removed_size, page_to_remove);
    }
    strcpy(&array[page_num*PAGE_NAME], page);
    return page_num;
}

int lines_in_file(const char* filen) {
    FILE *fp = fopen(filen, "r");
    int count = 0;
    char c;

    if (fp == NULL) { 
        return 0; 
    }
    for (c = getc(fp); c != EOF; c = getc(fp)) {
        if (c == '\n') {
            count = count + 1;
        }
    }
    fclose(fp);
    return count;
}