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

bool PROGRESS = true;
char filename[FILENAME_LENGTH] = {0};

unsigned int writes_to_disk = 0;
unsigned int reads = 0;
unsigned int writes = 0;
unsigned int page_faults = 0;

int add_frame(char *array, unsigned int size, const char *page);
int remove_page(const char *algo, char *array, unsigned int size, hash_table *table, char *removed, int removed_size, char *stack, int stack_size, const char *page);
void stack_push(char *stack, int size, char *page);
void stack_pop(char *stack, int size, char *str);
int lines_in_file(const char *filen);

int main(int argc, const char **argv)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0, read;
    char addr[9], tmp[2], page_num[6] = {0}, offset[4] = {0};
    int q = 1, shmid = -1, max = 1000000;
    shared_memory *smemory = NULL;
    bool start = -1;
    int lines_max = max;
    sem_t *me = NULL, *them = NULL;
    hash_table *table = NULL;
    char algo[8] = {'L', 'R', 'U', 0, 0, 0, 0, 0};

    for (int i = 1; i < argc; i += 2)
    {
        if (strcmp(argv[i], "-F") == 0)
        {
            strcpy(filename, argv[i + 1]);
        }
        // the start flag is necessary
        if (strcmp(argv[i], "-start") == 0)
        {
            if (strcmp("true", argv[i + 1]) == 0)
            {
                start = true;
            }
            else if (strcmp("false", argv[i + 1]) == 0)
            {
                start = false;
            }
        }
        else if (strcmp(argv[i], "-max") == 0)
        {
            if (atoi(argv[i + 1]) > 0)
            {
                max = atoi(argv[i + 1]);
            }
        }
        else if (strcmp(argv[i], "-q") == 0)
        {
            q = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-s") == 0)
        { //get shared memory id
            shmid = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-a") == 0)
        {
            if (strcmp(argv[i + 1], "LRU") != 0 && strcmp(argv[i + 1], "schance") != 0)
            {
                printf("-a: should be either LRU or schance\n");
                return 1;
            }
            strcpy(algo, argv[i + 1]);
        }
    }
    if (PROGRESS)
    {
        // to print progress
        lines_max = lines_in_file(filename);
        if (max < lines_max)
        {
            lines_max = max;
        }
    }
    if (start == -1)
    {
        printf("Please provide a boolean indicator for the first and second process \
            using the '-start' flag, (eg. -start true, -start false)\n");
        return 1;
    }
    smemory = shmat(shmid, NULL, 0); //attach to memory
    if (smemory < 0)
    {
        printf("***Attach Failed***\n");
        return 1;
    }
    //attach to memory
    smemory = shmat(shmid, (void *)0, 0);
    if (smemory < 0)
    {
        printf("***Attach Shared Memory Failed***\n");
        return 1;
    }

    char *frames_array = NULL;
    frames_array = (char *)((char *)smemory + sizeof(shared_memory));
    if (frames_array == NULL)
    {
        printf("***Frames array not created***\n");
        return 1;
    }
    char *pages_removed = NULL;
    pages_removed = (char *)((char *)smemory + sizeof(shared_memory) + sizeof(char) * (PAGE_NAME + 1) * smemory->frames);
    if (pages_removed == NULL)
    {
        printf("***Pages_removed array not created***\n");
        return 1;
    }
    char *stack = NULL;
    stack = (char *)((char *)smemory + sizeof(shared_memory) + sizeof(char) * (PAGE_NAME + 1) * smemory->frames + sizeof(char) * (PAGE_NAME + 1) * q);
    if (stack == NULL)
    {
        printf("***stack not created***\n");
        return 1;
    }

    if (filename[0] == 0)
    {
        printf("You need to give a filename using the '-F' flag.\n");
        return 1;
    }
    if ((fp = fopen(filename, "r")) == NULL)
    {
        printf("Could not open file. (%s)\n", filename);
        return 1;
    }
    if (start == true)
    {
        me = &(smemory->mutex_0);
        them = &(smemory->mutex_1);
    }
    else if (start == false)
    {
        me = &(smemory->mutex_1);
        them = &(smemory->mutex_0);
    }
    table = create_table(smemory->frames);

    int i = 0;
    int count = 0;
    // divide 2 so that we read max/2 from each file = max
    // max /= 2;
    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (PROGRESS)
        {
            if (count == 0 || count % (lines_max / 100) == 0)
            {
                printf("%s: (%d%%)\n", filename, count * 100 / lines_max);
            }
        }
        if (i >= q || max <= 0)
        {
            i = 0;
            sem_post(them);

            if (max <= 0)
            {
                break;
            }
        }
        if (i == 0)
        {
            sem_wait(me);
            if (pages_removed[0] != 0)
            {
                // Some of my pages were removed
                char str[PAGE_NAME] = {0};
                // other process removed my pages
                stack_pop(pages_removed, q, &str[0]);
                while (str[0] != 0)
                {
                    if (remove_table(table, str))
                    {
                        writes_to_disk++;
                    }
                    page_faults++;
                    // printf("REMOVED: %s\n", str);
                    stack_pop(pages_removed, q, &str[0]);
                }
            }
        }
        sscanf(line, "%s %s\n", addr, tmp);
        if (tmp[0] == 'R')
        {
            reads++;
        }
        else if (tmp[0] == 'W')
        {
            writes++;
        }
        // get page number and offset
        strncpy(page_num, addr, 5);
        strncpy(offset, addr + 5, 3);
        if (exists_table(table, page_num))
        {
            if (tmp[0] == 'W')
            {
                update_to_w_table(table, page_num);
            }
            if (strcmp(algo, "LRU") == 0)
            {
                // update position
                stack_push(stack, smemory->frames, page_num);
            }
            else if (strcmp(algo, "schance") == 0)
            {
                hash_item *info = get_table(table, page_num);
                stack[info->frame] = '1';
            }
            else
            {
                printf("ERROR: %s\n", algo);
                exit(-1);
            }
        }
        else
        {
            int f = add_frame(frames_array, smemory->frames, page_num);
            if (f == -1)
            {
                // printf("%s: %s%s %c\n", filename, page_num, offset, tmp[0]);
                // create some space
                f = remove_page(algo, frames_array, smemory->frames, table, pages_removed, q, stack, smemory->frames, page_num);
            }
            insert_table(table, page_num, f, tmp[0]);
            if (strcmp(algo, "LRU") == 0)
            {
                stack_push(stack, smemory->frames, page_num);
            }
            else if (strcmp(algo, "schance") == 0)
            {
                stack[f] = '1';
            }
            else
            {
                printf("ERROR: %s\n", algo);
                exit(-1);
            }
        }
        if (line)
        {
            free(line);
            line = NULL;
        }
        i++;
        count++;
        max--;
    }
    if (PROGRESS)
    {
        printf("%s: (%d%%)\n", filename, count * 100 / lines_max);
    }
    sem_post(them);
    if (start == true)
    {
        strcpy(&smemory->filename_0[FILENAME_LENGTH], filename);
        smemory->writes_to_disk_0 = writes_to_disk;
        smemory->reads_0 = reads;
        smemory->writes_0 = writes;
        smemory->page_faults_0 = page_faults;
    }
    else if (start == false)
    {
        strcpy(&smemory->filename_1[FILENAME_LENGTH], filename);
        smemory->writes_to_disk_1 = writes_to_disk;
        smemory->reads_1 = reads;
        smemory->writes_1 = writes;
        smemory->page_faults_1 = page_faults;
    }
    delete_table(table);
    fclose(fp);
    if (line)
    {
        free(line);
        line = NULL;
    }
    if (shmdt(smemory) == -1)
    {
        printf("***Detach Memory Failed***\n");
        return 1;
    }
    return 0;
}

// check for empty frames
int add_frame(char *array, unsigned int size, const char *page)
{
    for (int i = 0; i < size; i++)
    {
        if (strlen(&array[i * PAGE_NAME]) == 0)
        { // empty
            strcpy(&array[i * PAGE_NAME], page);
            return i;
        }
    }
    // frames are full
    return -1;
}

void stack_pop(char *stack, int size, char *str)
{
    memset(str, 0, PAGE_NAME);
    int i = 0;
    while (i <= size)
    {
        strncpy(str, (char *)(stack + PAGE_NAME * (size - i)), PAGE_NAME - 1);
        if (str[0] != 0)
        {
            break;
        }
        i++;
    }
    if (i <= size && str[0] != 0)
    {
        memset((char *)(stack + PAGE_NAME * (size - i)), 0, PAGE_NAME - 1);
    }
    return;
}

void stack_push(char *stack, int size, char *page)
{
    bool need_to_add = true;
    // printf("%s\n", str);
    for (int j = 0; j < size; j++)
    {
        char temp_addr[PAGE_NAME] = {0};
        strncpy(temp_addr, (char *)stack + PAGE_NAME * j, PAGE_NAME - 1);
        if (temp_addr[0] == 0)
        {
            break;
        }
        if (strncmp(temp_addr, page, PAGE_NAME - 1) == 0)
        {
            need_to_add = false;
            // put back to last used position
            memcpy((char *)stack + PAGE_NAME, stack, PAGE_NAME * j);
            strncpy(stack, page, PAGE_NAME - 1);

            break;
            // printf("%s\n", str);
        }
    }
    if (need_to_add)
    {
        // assume that their is space since I checked
        if (stack[0] != 0)
        {
            memcpy(stack + PAGE_NAME, stack, PAGE_NAME * (size - 1));
        }
        memcpy(stack, page, PAGE_NAME - 1);
    }
}

int remove_page(const char *algo, char *array, unsigned int size, hash_table *table, char *removed, int removed_size, char *stack, int stack_size, const char *page)
{
    char page_to_remove[PAGE_NAME];
    int page_num = -1;
    if (strcmp(algo, "LRU") == 0)
    {
        // test remove first
        stack_pop(stack, stack_size, page_to_remove);
        if (page_to_remove[0] == 0)
        {
            printf("You should not be here...\n");
        }
    }
    else if (strcmp(algo, "schance") == 0)
    {
        // so array is the main memory and we now dont use tha stack var.
        // we will use stack for the bit needed in second chance
        // eg. the item in the 3rd frame, that will be in &array[2*PAGE_NAME] will be linked to stack[2] that will be the bit information
        // use size since it should be the same
        int j;
        for (j = 0; j < size; j++)
        {
            if (stack[j] == '0')
            {
                strncpy(page_to_remove, &array[j * PAGE_NAME], PAGE_NAME - 1);
                break;
            }
            if (stack[j] == 0)
            {
                break;
            }
        }
        if (j == size || stack[j] == 0)
        {
            strncpy(page_to_remove, &array[0], PAGE_NAME - 1);
        }
        memset(stack, '0', sizeof(char) * size);
    }
    else
    {
        printf("ERROR: %s\n", algo);
        exit(-1);
    }
    for (int i = 0; i < size; i++)
    {
        // find at what frame the page was
        // I cannot find the frame from the hashtable since it could belong in the hash table of the other process
        if (strncmp(page_to_remove, &array[i * PAGE_NAME], PAGE_NAME - 1) == 0)
        {
            page_num = i;
            break;
        }
    }
    if (page_num < 0)
    {
        printf("ERROR: %s Page not found in main memory\n", page_to_remove);
        exit(-1);
    }
    // is it mine or belongs to the other process
    if (exists_table(table, page_to_remove))
    {
        if (remove_table(table, page_to_remove))
        {
            writes_to_disk++;
        }
        page_faults++;
    }
    else
    {
        stack_push(removed, removed_size, page_to_remove);
    }
    strcpy(&array[page_num * PAGE_NAME], page);
    return page_num;
}

int lines_in_file(const char *filen)
{
    FILE *fp = fopen(filen, "r");
    int count = 0;
    char c;

    if (fp == NULL)
    {
        return 0;
    }
    for (c = getc(fp); c != EOF; c = getc(fp))
    {
        if (c == '\n')
        {
            count = count + 1;
        }
    }
    fclose(fp);
    return count;
}