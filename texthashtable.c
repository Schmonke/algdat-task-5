#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

/***************************************
 * Utility functions                   *
 ***************************************/

/**
 * Left-rotates the value by the specified amount of bits.
 * This can be seen as a left shift that carries the bits over to the right side.
 * 
 * E.g. lrot(0b010001, 1) would return 0b100010 if ints were 6 bits.
 */
unsigned int lrot(unsigned int value, unsigned int bits)
{
    return (value << bits) | (value >> (sizeof(value) * CHAR_BIT) - bits);
}

/**
 * Gets a simple hash from the provided buffer.
 * 
 * 32-bit integers have a unique 1-to-1 mapping to a specific hash.
 */
unsigned int hash(void *buf, size_t size)
{
    unsigned int result = 0;
    unsigned char *cbuf = (unsigned char *)buf;

    for (size_t i = 0; i < size; i++)
    {
        result = lrot(result, sizeof(char) * CHAR_BIT) ^ cbuf[i];
    }

    return result;
}

/***************************************
 * Hash table implementation           *
 ***************************************/

/**
 * hash_table_entry represents an entry in the hash table.
 * It is a key-value-pair that points to the next element if it exists.
 */
typedef struct hash_table_entry
{
    struct hash_table_entry *next;

    void *key;
    size_t key_size;
    void *value;
} hash_table_entry;

/**
 * hash_table represents a hash table.
 * The 'buckets' member contains the linked lists.
 */
typedef struct
{
    size_t entries;
    size_t capacity;
    size_t collisions;
    hash_table_entry **buckets;
} hash_table;

/**
 * Prints the collision that occured between two hash table entries.
 */
void hash_table_print_collision(const char *prefix, hash_table_entry *a, hash_table_entry *b)
{
    fwrite(prefix, sizeof(char), strlen(prefix), stdout);
    fwrite(a->key, sizeof(char), a->key_size, stdout);
    printf(" -> ");
    fwrite(b->key, sizeof(char), b->key_size, stdout);
    printf("\n");
}

/**
 * Creates a new hash table with the specified capacity.
 */
hash_table *hash_table_create(size_t capacity)
{
    hash_table *table = (hash_table *)malloc(sizeof(hash_table));
    table->entries = 0;
    table->collisions = 0;
    table->capacity = capacity;
    table->buckets = (hash_table_entry **)calloc(capacity, sizeof(hash_table_entry));
    return table;
}

/**
 * Frees the hash table.
 * NOTE: Manually allocated data that is used in entries must be freed manually.
 */
void hash_table_free(hash_table *table)
{
    free(table->buckets);
    free(table);
}

/**
 * Adds an entry to the hash table.
 */
void hash_table_add(hash_table *table, void *key, size_t key_size, void *value)
{
    // Create new entry.
    hash_table_entry *new_entry = calloc(1, sizeof(hash_table_entry));
    new_entry->key = key;
    new_entry->key_size = key_size;
    new_entry->value = value;

    // Find target index.
    unsigned int hashed_key = hash(key, key_size);
    size_t index = hashed_key % table->capacity;
    hash_table_entry **entry = &table->buckets[index];
    hash_table_entry **first_entry = entry;

    // Find first empty entry.
    while (*entry != NULL)
    {
        entry = &(*entry)->next;
    }

    // Update the first empty entry to the newly created entry.
    *entry = new_entry;
    table->entries++;

    // Collision occurred.
    if (*first_entry != new_entry)
    {
        table->collisions++;
        hash_table_print_collision("!COLLISION! | add: ", *first_entry, new_entry);
    }
}

/**
 * Performs lookup in the hash table based on the key and writes the value to the value pointer.
 * If the function does not find a match, it returns false and the value pointer is not written to.
 */
bool hash_table_lookup(hash_table *table, void *key, size_t key_size, void **value)
{
    // Find target index.
    unsigned int hashed_key = hash(key, key_size);
    size_t index = hashed_key % table->capacity;
    hash_table_entry *entry = table->buckets[index];
    hash_table_entry *first_entry = entry;

    // Multiple keys can have same hash!
    // We need to find the entry with precisely the same key.
    bool found = false;
    while (entry != NULL && !found)
    {
        // We have found a match if key size and key contents are equal.
        if (entry->key_size == key_size && memcmp(entry->key, key, key_size) == 0)
        {
            *value = entry->value;
            found = true;
            break;
        }

        entry = entry->next;
    }

    // Print collision
    if (found && first_entry != entry)
    {
        hash_table_print_collision("!COLLISION! | lookup: ", first_entry, entry);
    }

    return found;
}

/**
 * Gets the number of entries in the hash table.
 */
size_t hash_table_entries(hash_table *table)
{
    return table->entries;
}

/**
 * Gets the load factor (entries / capacity) of the hash table.
 */
float hash_table_load_factor(hash_table *table)
{
    return (float)table->entries / (float)table->capacity;
}

/**
 * Gets the number of collisions in the hash table.
 */
size_t hash_table_collisions(hash_table *table)
{
    return table->collisions;
}

/***************************************
 * Program code                        *
 ***************************************/
const char MAIKEN_NAME[] = "Maiken Louise Brechan";
const char MARTIN_NAME[] = "Martin Dolmen Helmersen";
const char MAGNUS_NAME[] = "Magnus Hektoen Steensland";
const char NORBERT_NAME[] = "Norbert Arkadiusz Görke";
const char EULER_NAME[] = "Leonhard Euler";
const int HASH_TABLE_CAPACITY = 127;

/**
 * Performs a person ID lookup in the hash table.
 * Returns -1 if a person with the specified name is not found.
 */
int person_lookup(hash_table *table, const char *name, size_t name_size)
{
    void *id;
    if (!hash_table_lookup(table, (void *)name, name_size, &id))
    {
        return -1;
    }
    return (int)(uintptr_t)id;
}

/**
 * Runs the program with the specified buffer.
 * The names in the buffer, including the final one, must be newline-delimited.
 */
void run_with_buffer(const char *names, size_t size)
{
    hash_table *table = hash_table_create(HASH_TABLE_CAPACITY);

    printf("Filling hash table...\n");
    int person_id = 0;
    size_t start = 0;
    for (size_t i = 0; i < size; i++)
    {
        if (names[i] == '\n')
        {
            hash_table_add(table, (void *)&names[start], i - start, (void *)(uintptr_t)person_id++);
            start = i + 1;
        }
    }
    printf("Hash table filled!\n\n");

    size_t entries = hash_table_entries(table);
    float load_factor = hash_table_load_factor(table);
    size_t collisions = hash_table_collisions(table);
    float collisions_per_person = (float)collisions / (float)hash_table_entries(table);

    printf("Statistics:\n");
    printf("   Persons registered    : %ld\n", entries);
    printf("   Collisions            : %ld\n", collisions);
    printf("   Load factor           : %f\n", load_factor);
    printf("   Collisions per person : %f\n", collisions_per_person);

    printf("Lookups:\n");
    printf("   %s: (ID=%d)\n", MAIKEN_NAME, person_lookup(table, MAIKEN_NAME, sizeof(MAIKEN_NAME) - 1));
    printf("   %s: (ID=%d)\n", MARTIN_NAME, person_lookup(table, MARTIN_NAME, sizeof(MARTIN_NAME) - 1));
    printf("   %s: (ID=%d)\n", MAGNUS_NAME, person_lookup(table, MAGNUS_NAME, sizeof(MAGNUS_NAME) - 1));
    printf("   %s: (ID=%d)\n", NORBERT_NAME, person_lookup(table, NORBERT_NAME, sizeof(NORBERT_NAME) - 1));
    printf("   %s: (ID=%d)\n", EULER_NAME, person_lookup(table, EULER_NAME, sizeof(EULER_NAME) - 1));

    hash_table_free(table);
}

/**
 * Runs the program using a file.
 * The names in the file, including the final one, must be newline-delimited.
 */
int run_with_file(FILE *fp)
{
    // Determine the file size.
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *names = malloc(size);

    // Read file into buffer.
    fread(names, sizeof(char), size, fp);
    run_with_buffer(names, size);

    free(names);
    return 0;
}

int handle_file(const char *file_path)
{
    FILE *file = fopen(file_path, "r");

    if (file == NULL)
    {
        perror("Unable to open file.");
        return 1;
    }
    return run_with_file(file);
}

void print_help()
{
    printf(
        "You must specify which file to read from as an argument to the program.\n"
        "Usage: texthashtable <file_name>\n"
        "E.g. ./texthashtable ~/navn.txt\n");
}

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        print_help();
        return 1;
    }
    handle_file(argv[1]);
}
