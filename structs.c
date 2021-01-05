#include "structs.h"
#include <stdlib.h>
#include <string.h>

// temp
#include <stdio.h>

int hash(const char *str, const int capacity);
void delete_list(hash_list* list);

hash_table* create_table(int capacity) {
    hash_table* hs = malloc(sizeof(hash_table));
    hs->capacity = capacity;
    hs->table = malloc(sizeof(hash_list)*capacity);
    for (int i = 0; i < capacity; i++) {
        hs->table[i].next = NULL;
        hs->table[i].item.key = NULL;
        hs->table[i].item.value = NULL;
    }
    return hs;
}

void delete_table(hash_table* table) {
    for (int i = 0; i < table->capacity; i++) {
        if (table->table[i].next != NULL) {
            delete_list(table->table[i].next);
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
    const int pos = hash(key, table->capacity);
    hash_list* list = &table->table[pos];
    if (list->item.key == NULL || list->item.value == NULL) {
        list->next = NULL;
        list->item.key = malloc(sizeof(char)*(strlen(key) + 1));
        list->item.value = malloc(sizeof(char)*(strlen(value) + 1));
        strcpy(list->item.key, key);
        strcpy(list->item.value, value);
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
    }
}

void remove_table(hash_table* table, const char* key, const char* value) {

}

/* private functions */

int hash(const char *str, const int capacity) {
    // https://stackoverflow.com/a/7666577/11155628
    unsigned long hash = 7027;
    int c;

    while (c = *str++) {
        hash = ((hash << 5) + hash) + c;
    }

    return (int)(hash % capacity);
}

void delete_list(hash_list* list) {
    if (list == NULL) {
        return;
    }
    if (list->next != NULL) {
        delete_list(list->next);
    }
    if (list->item.key != NULL) {
        free(list->item.key);
    }
    if (list->item.value != NULL) {
        free(list->item.value);
    }
    free(list);
}