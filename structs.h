#pragma once

#include <semaphore.h>

#define FILENAME_LENGTH 200
#define PAGE_NAME 6

typedef enum { 
    false, 
    true 
} bool; 

typedef struct hash_item {
    char* key;
    int frame;
    char action;
} hash_item;

typedef struct hash_list {
    hash_item item; // item info
    struct hash_list* next;    // next item in list
} hash_list;

typedef struct hash_table {
    int size; // how many hash_lists
    hash_list* table;   // array of linked lists
} hash_table;

hash_table* create_table(int size);
void delete_table(hash_table* table);

void insert_table(hash_table* table, const char* key, int frame, char action);
bool remove_table(hash_table* table, const char* key);
bool exists_table(hash_table* table, const char* key);
hash_item* get_table(hash_table* table, const char* key);

typedef struct shared_memory {
    char filename_0[FILENAME_LENGTH];
    unsigned int writes_to_disk_0;
    unsigned int reads_0;
    unsigned int writes_0;
    unsigned int page_faults_0;

    char filename_1[FILENAME_LENGTH];
    unsigned int writes_to_disk_1;
    unsigned int reads_1;
    unsigned int writes_1;
    unsigned int page_faults_1;

    unsigned int frames;
    sem_t mutex_0;
    sem_t mutex_1;
} shared_memory;