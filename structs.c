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

hash_table* create_table(int size) {
    hash_table* hs = malloc(sizeof(hash_table));
    hs->size = size;
    hs->table = malloc(sizeof(hash_list)*size);
    for (int i = 0; i < size; i++) {
        hs->table[i].next = NULL;
        hs->table[i].item.key = NULL;
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
    }
    free(table->table);
    free(table);
}

void insert_table(hash_table* table, const char* key, int frame, char action) {
    const int pos = _hash(key, table->size);
    hash_list* list = &table->table[pos];
    if (list->item.key == NULL) {
        list->next = NULL;
        list->item.key = malloc(sizeof(char)*(strlen(key) + 1));
        strcpy(list->item.key, key);
        list->item.frame = frame;
        list->item.action = action;
    } else {
        while (list->next != NULL) {
            list = list->next;
        }
        list->next = malloc(sizeof(hash_list));
        list = list->next;
        list->next = NULL;
        list->item.key = malloc(sizeof(char)*(strlen(key) + 1));
        strcpy(list->item.key, key);
        list->item.frame = frame;
        list->item.action = action;
    }
}

// return true if we need to write to disk
bool remove_table(hash_table* table, const char* key) {
    const int pos = _hash(key, table->size);
    bool rtn = false;
    hash_list* list = &table->table[pos], *prev = NULL;
    if (list->item.key != NULL) {
        // if it's in the list head
        if (strcmp(key, list->item.key) == 0) {
            if (list->next == NULL) {
                if (list->item.action == 'W' ) {
                    rtn = true;
                }
                // if there is no next, we can just free key
                free(list->item.key);
                list->item.key = NULL;
            } else {
                // else we copy data from the second position to head 
                // and then free the second position
                list = table->table[pos].next;
                free(table->table[pos].item.key);
                table->table[pos].item.key = NULL;
                // could just realloc
                table->table[pos].item.key = malloc(sizeof(char)*(strlen(list->item.key) + 1));
                strcpy(table->table[pos].item.key, list->item.key);
                table->table[pos].item.frame = list->item.frame;
                table->table[pos].item.action = list->item.action;
                table->table[pos].next = list->next;
                if (list->item.action == 'W' ) {
                    rtn = true;
                }
                free(list->item.key);
                list->item.key = NULL;
                free(list);
                list = NULL;
            }
            return rtn;
        }
    }
    prev = list;
    while (list->item.key != NULL && strcmp(key, list->item.key) != 0 && list->next != NULL) {
        prev = list;
        list = list->next;
    }
    if (list->item.key != NULL && strcmp(key, list->item.key) == 0) {
        if (list->item.action == 'W' ) {
            rtn = true;
        }
        free(list->item.key);
        list->item.key = NULL;
        prev->next = list->next;
        free(list);
        list = NULL;
    }
    // not found
    return -1;
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

hash_item* get_table(hash_table* table, const char* key) {
    const int pos = _hash(key, table->size);
    hash_list* list = &table->table[pos];
    if (list->item.key == NULL) {
        return NULL;
    }
    while (strcmp(key, list->item.key) != 0 && list->next != NULL) {
        list = list->next;
    }
    if (strcmp(key, list->item.key) == 0) {
        return &list->item;
    }
    return NULL;
}

void update_to_w_table(hash_table* table, const char* key) {
    const int pos = _hash(key, table->size);
    hash_list* list = &table->table[pos];
    if (list->item.key == NULL) {
        return;
    }
    while (strcmp(key, list->item.key) != 0 && list->next != NULL) {
        list = list->next;
    }
    if (strcmp(key, list->item.key) == 0) {
        list->item.action = 'W';
    }
    return;
}

/* private functions */

int _hash(const char *str, const int size) {
    // https://stackoverflow.com/a/7666577/11155628
    unsigned long hash = 7027;
    int c;

    while ((c = *str++)) {
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
    free(list);
}