#define FILENAME_LENGTH 200

typedef enum { 
    false, 
    true 
} bool; 


typedef struct hash_item {
    char* key;
    char* value;
} hash_item;

typedef struct hash_list {
    hash_item item; // item info
    struct hash_list* next;    // next item in list
} hash_list;

typedef struct hash_table {
    int capacity; // how many hash_lists
    hash_list* table;   // array of linked lists
} hash_table;

hash_table* create_table(int capacity);
void delete_table(hash_table* table);

void insert_table(hash_table* table, const char* key, const char* value);
void remove_table(hash_table* table, const char* key);
bool exists_table(hash_table* table, const char* key);
