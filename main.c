#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define TABLE_SIZE 1024

typedef struct Entry
{
    char *key;
    char *value;
    struct Entry *next;
} Entry;

typedef struct
{
    Entry *buckets[TABLE_SIZE];
} HashMap;

unsigned int hash(char *key)
{
    unsigned int hash = 5381;
    while (*key)
    {
        hash = ((hash << 5) + hash) + *key++;
    }
    return hash % TABLE_SIZE;
}

FILE *log_file;

void hashmap_set(HashMap *map, char *key, char *value)
{
    int index = hash(key);
    Entry *curr = map->buckets[index];

    while (curr)
    {
        if (strcmp(curr->key, key) == 0)
        {
            free(curr->value);
            curr->value = strdup(value);
            return;
        }
        curr = curr->next;
    }

    Entry *new_entry = malloc(sizeof(Entry));
    new_entry->key = strdup(key);
    new_entry->value = strdup(value);
    new_entry->next = map->buckets[index];

    map->buckets[index] = new_entry;
}

char *hashmap_get(HashMap *map, char *key)
{
    int index = hash(key);
    Entry *curr = map->buckets[index];

    while (curr)
    {
        if (strcmp(curr->key, key) == 0)
        {
            return curr->value;
        }

        curr = curr->next;
    }

    return NULL;
}

void hashmap_delete(HashMap *map, char *key)
{
    unsigned int index = hash(key);
    Entry *curr = map->buckets[index];
    Entry *prev = NULL;

    while (curr)
    {
        if (strcmp(curr->key, key) == 0)
        {
            if (prev)
                prev->next = curr->next;
            else
                map->buckets[index] = curr->next;

            free(curr->key);
            free(curr->value);
            free(curr);
            return;
        }

        prev = curr;
        curr = curr->next;
    }
}

// Persistent Layer

void log_set(FILE *f, char *key, char *value)
{
    uint8_t op = 1;
    uint32_t klen = strlen(key);
    uint32_t vlen = strlen(value);

    fwrite(&op, 1, 1, f);
    fwrite(&klen, sizeof(uint32_t), 1, f);
    fwrite(&vlen, sizeof(uint32_t), 1, f);
    fwrite(key, 1, klen, f);
    fwrite(value, 1, vlen, f);

    fflush(f);
}

void log_delete(FILE *f, char *key)
{
    uint8_t op = 2;
    uint32_t klen = strlen(key);

    fwrite(&op, 1, 1, f);
    fwrite(&klen, sizeof(uint32_t), 1, f);
    fwrite(key, 1, klen, f);

    fflush(f);
}

void load_from_file(HashMap *map, FILE *f)
{
    rewind(f);

    while (1)
    {
        uint8_t op;
        uint32_t klen, vlen;

        if (fread(&op, 1, 1, f) != 1)
            break;
        fread(&klen, sizeof(uint32_t), 1, f);
        fread(&vlen, sizeof(uint32_t), 1, f);

        // Add 1 to klen to account for null terminator
        char *key = malloc(klen + 1);
        fread(key, 1, klen, f);
        key[klen] = '\0';

        if (op == 1)
        {
            char *value = malloc(vlen + 1);
            fread(value, 1, klen, f);
            value[vlen] = '\0';

            hashmap_set(map, key, value);
        }
        else if (op == 2)
        {
            hashmap_delete(map, key);
        }

        free(key);
    }
}

void kv_set(FILE *log_file, HashMap *map, char *key, char *value)
{
    hashmap_set(map, key, value);
    log_set(log_file, key, value);
}

void kv_delete(FILE *log_file, HashMap *map, char *key)
{
    hashmap_delete(map, key);
    log_delete(log_file, key);
}

char *kv_get(HashMap *map, char *key)
{
    return hashmap_get(map, key);
}

void trim_newline(char *str)
{
    str[strcspn(str, "\n")] = '\0';
}

void start_cli(HashMap *m, FILE *f)
{
    char line[1024];

    while (1)
    {
        printf("\nkv > ");
        if (!fgets(line, sizeof(line), stdin))
            break;

        trim_newline(line);

        // Tokenize the input
        char *cmd = strtok(line, " ");

        if (!cmd)
            continue;

        if (strcmp(cmd, "SET") == 0)
        {
            char *key = strtok(NULL, " ");
            char *value = strtok(NULL, " ");

            if (!key || !value)
            {
                printf("Usage: SET key/value\n");
                continue;
            }

            kv_set(f, m, key, value);
            printf("OK\n");
        }
        else if (strcmp(cmd, "GET") == 0)
        {
            char *key = strtok(NULL, " ");

            if (!key)
            {
                printf("Usage: GET key\n");
                continue;
            }

            char *value = kv_get(m, key);

            if (value)
                printf("[%s]: %s\n", key, value);
            else
                printf("(nil)\n");
        }
        else if (strcmp(cmd, "DEL") == 0)
        {
            char *key = strtok(NULL, " ");

            if (!key)
            {
                printf("Usage: DEL key\n");
                continue;
            }

            kv_delete(f, m, key);
            printf("OK\n");
        }
        else if (strcmp(cmd, "EXIT") == 0)
        {
            printf("Exiting\n");
            break;
        }
        else
        {
            printf("Unknown command");
        }
    }
}

int main()
{
    HashMap map = {0};

    log_file = fopen("store.db", "ab+");
    if (!log_file)
    {
        perror("Log file failed to open!");
        exit(1);
    }

    load_from_file(&map, log_file);

    printf("Simple KV Store (SET/GET/DEL/EXIT)\n");

    start_cli(&map, log_file); 
    
    fclose(log_file);
    return 0;
}