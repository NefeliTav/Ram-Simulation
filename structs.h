#pragma once

#include <semaphore.h>

#define FILENAME_LENGTH 200

typedef enum { 
    false, 
    true 
} bool; 

typedef struct trace {
    char address[8];
    char action[8];
} trace;

typedef struct hash_item {
    char* key;
    char* value;
} hash_item;

typedef struct hash_list {
    hash_item item; // item info
    struct hash_list* next;    // next item in list
} hash_list;

typedef struct hash_table {
    int size; // how many hash_lists
    int capacity; // How many pages memory can fit (number of frames)
    int length; // How many pages inside the memory
    hash_list* table;   // array of linked lists
} hash_table;

hash_table* create_table(int size, int capacity);
void delete_table(hash_table* table);

void insert_table(hash_table* table, const char* key, const char* value);
void remove_table(hash_table* table, const char* key);
bool exists_table(hash_table* table, const char* key);

typedef struct shared_memory {
    sem_t mutex_0;
    sem_t mutex_1;
    sem_t read;
    sem_t can_write;
} shared_memory;