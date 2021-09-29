#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <stdarg.h>

typedef struct {
    bool exists;
    int value;
} hash_table_entry;

typedef struct {
    size_t capacity;
    size_t entries;
    size_t collisions;
    hash_table_entry *values;
} hash_table;

typedef enum {
    LIN,
    QUAD,
    DOUBLEH
} probe_func;

//typedef int probe_func(int key, int size);

void swap(int *a, int*b)
{    
    int s = *a;
    *a = *b;
    *b = s;
}

int *create_random_unique_array(size_t size)
{
    int *array = calloc(size, sizeof(int));
    for (size_t i = 0; i < size; i++)
    {
        array[i] = i;
    }
    for (size_t i = 0; i < size; i++)
    {
        array[i] = array[rand() % size];
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
int hash2(int key, int m)
{
    int result = -1;
    if (m != 0 && m & (m -1) == 0) // m is power of two. 
    {
        result = (2 * abs(key) + 1) % m;
    }
    else
    {
        result = key % (m -1) +1; // otherwise m is a prime.
    }
    return result;
}

int probe_lin(int i, int m)
{
    return (hash1(i, m) + i) % m;
}

//Uses prime numbers as constants to minimize chance of common denominator.
int probe_quad(int i, int m)
{
    const int h = hash1(i, m);
    const int c1 = 1147419379;
    const int c2 = 547419503;
    return (h + c1*i + c2*i*i) % m;
}

// h2 and m must be relative prime
int probe_doublehash(int i, int m)
{
    const int h1 = hash1(i, m);
    const int h2 = hash2(i, m);
    return (h1 + i*h2) % m;
}

//Open addressing p.161
int add_entry(int *k, int m, int *ht[m], int (*probefunc)(int, int))
{
    for (int i = 0; i < m; i++)
    {
        int j = probefunc(i,m);
        if (!ht[j])
        {
            ht[j] = k;
            return j;
        }
    }
    return -1; //full
}

int findpos(int k, int m, int *ht[m], int (*probefunc)(int, int))
{
    for (int i = 0; i<m; i++)
    {
        int j = probefunc(i, m);
        if(!ht[j]) return -1;
        if(*ht[j] == k) return j;
    }
    return -1; //Does not exist
}

// probe_func *probe_array[3] = {
//     probe_lin,
//     probe_quad,
//     probe_doublehash
// }

hash_table *hash_table_create(size_t min_capacity)
{
    hash_table *table = calloc(1, sizeof(hash_table));
    table->capacity = gpo2stv(min_capacity);
    table->values = calloc(table->capacity, sizeof(hash_table_entry));
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

// void hash_table_add(hash_table *table, int value)
// {
//     hash_table_entry *entry = hash_table_entry_create(true, value);

//     int j = probefunc(value, table->capacity);
//         if (!table->values[j])
//         {
//             table->values[j] = *entry;
//             return j;
//         }
//     return -1; //full
// }

void filltable(hash_table *table, int * array, size_t tablesize, probe_func probe_enum)
{
    int (*fptr)(int, int);

    switch (probe_enum)
    {
        case LIN:
            fptr = &probe_lin;
            break;
        case QUAD:
            fptr = &probe_quad;
            break;
        case DOUBLEH:
            fptr = &probe_doublehash;
            break;
        default:
            return;
    }
    for(int i = 0; i < tablesize; i++)
    {
        add_entry(array[i], tablesize, table, (*fptr)(i, tablesize));
    }
}

int main(int argc, char *argv[]) 
{
    int randbound = 10000000;

    if(argc > 1) //User can specify capaicity
    {
        randbound = atoi(argv[1]);
    }

    int *randarray = create_random_unique_array(randbound);

    hash_table *tablelin = hash_table_create(randbound);
    hash_table *tablequad = hash_table_create(randbound);
    hash_table *tabledouble = hash_table_create(randbound);


    filltable(tablelin, randarray, tablelin->capacity, LIN);
    filltable(tablequad, randarray, tablelin->capacity, QUAD);
    filltable(tabledouble, randarray, tablelin->capacity, DOUBLEH);




    hash_table_free(tablelin);
    hash_table_free(tablequad);
    hash_table_free(tabledouble);
    
    return 0;
}