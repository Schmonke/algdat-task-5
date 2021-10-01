#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <stdarg.h>
#include <time.h>

typedef struct
{
    bool exists;
    int value;
} hash_table_entry;

typedef size_t probe_func(int a, int b, int key, size_t hash_table_capacity);

typedef struct
{
    size_t capacity;
    size_t entries;
    size_t collisions;
    hash_table_entry *values;

    probe_func *probe;
} hash_table;

/**
 * Left-rotates the value by the specified amount of bits.
 * This can be seen as a left shift that carries the bits over to the right side.
 * 
 * E.g. lrot(0b010001, 1) would return 0b100010 if ints were 6 bits.
 */
size_t lrot(size_t value, size_t bits)
{
    return (value << bits) | (value >> (sizeof(value) * CHAR_BIT) - bits);
}


void swap(int *a, int *b)
{
    int s = *a;
    *a = *b;
    *b = s;
}

int *create_random_unique_array(size_t length)
{
    int *array = calloc(length, sizeof(int));
    int mult = (rand() % (length / 2)) + (length / 2);
    for (size_t i = 0; i < length; i++)
    {
        array[i] = i*mult;
    }
    for (size_t i = 0; i < length; i++)
    {
        swap(&array[i], &array[rand() % length]);
    }
    return array;
}

size_t gpo2stv(size_t value)
{
    const int bits = sizeof(size_t) * CHAR_BIT;
    if (value & 1 << (bits - 1))
    {
        return 0;
    }
    for (int i = bits - 2; i > 0; i--)
    {
        if (value & 1 << i)
        {
            return 1 << i + 1;
        }
    }
    return 1;
}

//Hashfunc 1
unsigned int hash1(int key, int m)
{
    return key % m;
}

//Hashfunc 2
unsigned int hash2(int key, int m)
{
    return (2 * abs(key) + 1) % m;
}

size_t probe_linear(int h, int _, int i, size_t m)
{
    return (h + i) % m;
}

//Uses prime numbers as constants to minimize chance of common denominator.
size_t probe_quadratic(int h, int _, int i, size_t m)
{
    //printf("QUADPROBE: %d\n", i);
    const unsigned long long c1 = 211;
    const unsigned long long c2 = 857;
    return (h + c1 * i + c2 * i * i) % m;
}

// h2 and m must be relative prime
size_t probe_doublehash(int h1, int h2, int i, size_t m)
{
    return (h1 + i * h2) % m;
}

int hash_table_add(hash_table *table, int v) 
{
    const size_t capacity = table->capacity;
    const int h1 = hash1(v, capacity);
    const int h2 = hash2(v, capacity);
    int colls = 0;

    for (int i = 0; i < capacity; i++)
    {
        int j = table->probe(h1, h2, i, capacity);
        hash_table_entry *value = &table->values[j];
        if (!value->exists)
        {
            value->exists = true;
            value->value = v;
            break;
        }
        colls++;
    }
    
    if (colls == capacity)
    {
        //printf("TABLE FULL!\n");
    }

    return colls;
}

int findpos(int k, int m, int *ht[m], int (*probefunc)(int, int))
{
    for (int i = 0; i < m; i++)
    {
        int j = probefunc(i, m);
        if (!ht[j])
            return -1;
        if (*ht[j] == k)
            return j;
    }
    return -1; //Does not exist
}

hash_table *hash_table_create(size_t min_capacity, probe_func *probe)
{
    hash_table *table = calloc(1, sizeof(hash_table));
    table->capacity = gpo2stv(min_capacity);
    table->values = calloc(table->capacity, sizeof(hash_table_entry));
    table->probe = probe;
    return table;
}

hash_table_entry *hash_table_entry_create(bool b, int val)
{
    hash_table_entry *entry = malloc(sizeof(hash_table_entry));
    entry->exists = b;
    entry->value = val;
    return entry;
}

void hash_table_free(hash_table *table)
{
    free(table->values);
    free(table);
}

int hash_table_add_all(hash_table *table, int *values, size_t values_length)
{
    int col = 0;
    for (size_t i = 0; i < values_length; i++)
    {
        col += hash_table_add(table, values[i]);
    }
    return col;
}

int main(int argc, char *argv[])
{
    const struct 
    {
        const char *name;
        probe_func *probe;
    } probe_types[] = {
        { "linear", &probe_linear },
        { "quadratic", &probe_quadratic },
        { "double-hash", &probe_doublehash }
    };
    const int probe_types_length = sizeof(probe_types)/sizeof(probe_types[0]);
    
    const float fill_ratios[] = { 0.5, 0.8, 0.9, 0.99, 1.0 };
    const int fill_ratios_length = sizeof(fill_ratios)/sizeof(fill_ratios[0]);

    int table_bound = 10000000;

    if (argc > 1) //User can specify capaicity
    {
        table_bound = atoi(argv[1]);
    }

    srand(time(NULL));

    printf("Generating %d unique numbers...\n\n", table_bound);
    int *rand_array = create_random_unique_array(table_bound);

    for (int i = 0; i < probe_types_length; i++)
    {
        for (int j = 0; j < fill_ratios_length; j++)
        {
            float fill_ratio = fill_ratios[j];
            printf("Creating table for %s\n", probe_types[i].name);
            hash_table *table = hash_table_create(table_bound, probe_types[i].probe);
            printf("Filling table (%02.0f%%) ...\n", fill_ratio*100);
            time_t start = time(NULL);
            int col = hash_table_add_all(table, rand_array, table_bound * fill_ratio);
            time_t end = time(NULL);
            printf("Time       : %lds\n", end - start);
            printf("Collisions : %d\n", col);
            printf("Freeing table...\n\n");
            hash_table_free(table);
        }
    }

    return 0;
}