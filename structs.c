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

int _hash(const char *str, const int size);
void _delete_list(hash_list* list);

hash_table* create_table(int size, int capacity) {
    hash_table* hs = malloc(sizeof(hash_table));
    hs->size = size;
    hs->capacity = capacity;
    hs->length = 0;
    hs->table = malloc(sizeof(hash_list)*size);
    for (int i = 0; i < size; i++) {
        hs->table[i].next = NULL;
        hs->table[i].item.key = NULL;
        hs->table[i].item.value = NULL;
    }
    return hs;
}

void delete_table(hash_table* table) {
    for (int i = 0; i < table->size; i++) {
        if (table->table[i].next != NULL) {
            _delete_list(table->table[i].next);
        }
        if (table->table[i].item.key != NULL) {
            free(table->table[i].item.key);
        }
        if (table->table[i].item.value != NULL) {
            free(table->table[i].item.value);
        }
    }
    free(table->table);
    free(table);
}

void insert_table(hash_table* table, const char* key, const char* value) {
    if (exists_table(table, key) == true) {
        return;
    }
    if (table->length == table->capacity) {
        // remove one 
    }
    const int pos = _hash(key, table->size);
    hash_list* list = &table->table[pos];
    if (list->item.key == NULL || list->item.value == NULL) {
        list->next = NULL;
        list->item.key = malloc(sizeof(char)*(strlen(key) + 1));
        list->item.value = malloc(sizeof(char)*(strlen(value) + 1));
        strcpy(list->item.key, key);
        strcpy(list->item.value, value);
        table->length++;
    } else {
        while (list->next != NULL) {
            list = list->next;
        }
        list->next = malloc(sizeof(hash_list));
        list = list->next;
        list->next = NULL;
        list->item.key = malloc(sizeof(char)*(strlen(key) + 1));
        list->item.value = malloc(sizeof(char)*(strlen(value) + 1));
        strcpy(list->item.key, key);
        strcpy(list->item.value, value);
        table->length++;
    }
}

void remove_table(hash_table* table, const char* key) {
    const int pos = _hash(key, table->size);
    hash_list* list = &table->table[pos], *prev = NULL;
    if (list->item.key != NULL && list->item.value != NULL) {
        // if it's in the list head
        if (strcmp(key, list->item.key) == 0) {
            if (list->next == NULL) {
                // if there is no next, we can just free key and value
                free(list->item.key);
                list->item.key = NULL;
                free(list->item.value);
                list->item.value = NULL;
            } else {
                // else we copy data from the second position to head 
                // and then free the second position
                list = table->table[pos].next;
                free(table->table[pos].item.key);
                table->table[pos].item.key = NULL;
                free(table->table[pos].item.value);
                table->table[pos].item.value = NULL;
                // could just realloc
                table->table[pos].item.key = malloc(sizeof(char)*(strlen(list->item.key) + 1));
                table->table[pos].item.value = malloc(sizeof(char)*(strlen(list->item.value) + 1));
                strcpy(table->table[pos].item.key, list->item.key);
                strcpy(table->table[pos].item.value, list->item.value);
                table->table[pos].next = list->next;

                free(list->item.key);
                list->item.key = NULL;
                free(list->item.value);
                list->item.value = NULL;
                free(list);
                list = NULL;
            }
            table->length++;
            return;
        }
    }
    prev = list;
    while (strcmp(key, list->item.key) != 0 && list->next != NULL) {
        prev = list;
        list = list->next;
    }
    if (strcmp(key, list->item.key) == 0) {
        free(list->item.key);
        list->item.key = NULL;
        free(list->item.value);
        list->item.value = NULL;
        prev->next = list->next;
        free(list);
        list = NULL;
        table->length++;
    }
    // not found
}

bool exists_table(hash_table* table, const char* key) {
    const int pos = _hash(key, table->size);
    hash_list* list = &table->table[pos];
    if (list->item.key == NULL) {
        return false;
    }
    while (strcmp(key, list->item.key) != 0 && list->next != NULL) {
        list = list->next;
    }
    return strcmp(key, list->item.key) == 0 ? true : false;
}

/* private functions */

int _hash(const char *str, const int size) {
    // https://stackoverflow.com/a/7666577/11155628
    unsigned long hash = 7027;
    int c;

    while (c = *str++) {
        hash = ((hash << 5) + hash) + c;
    }
    return (int)(hash % size);
}

void _delete_list(hash_list* list) {
    if (list == NULL) {
        return;
    }
    if (list->next != NULL) {
        _delete_list(list->next);
    }
    if (list->item.key != NULL) {
        free(list->item.key);
    }
    if (list->item.value != NULL) {
        free(list->item.value);
    }
    free(list);
}