#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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


//Hashfunc 1
unsigned int hash1(int key, int m)
{
    return key % m;
}


int probe_lin(int h, int i, int m)
{
    return  (h + i) % m;
}

//Uses prime numbers as constants to minimize chance of common denominator.
int probe_quad(int h, int i, int m)
{
    const int c1 = 1147419379;
    const int c2 = 547419503;
    return (h + c1*i + c2*i*i) % m;
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

// h2 and m must be relative prime
int probe_doublehash(int h1, int h2, int i, int m)
{
    return (h1 + i*h2) % m;
}

//Open addressing p.161
int add_entry (int *k, int m, int *ht[m])
{
    int h = hashfunc(*k, m);
    for (int i = 0; i < m; i++)
    {
        int j = probe_lin(h,i,m);
        if (!ht[j])
        {
            ht[j] = k;
            return j;
        }
    }
    return -1; //full
}


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

hash_table *hash_table_create(size_t capacity)
{
    hash_table *table = calloc(1, sizeof(hash_table));
    table->values = calloc(capacity, sizeof(hash_table_entry));
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

void hash_table_add(hash_table *table, int value)
{
    hash_table_entry *entry = hash_table_entry_create(true, value);

    table->values[index] = entry;
}



int findpos(int k, int m, int *ht[m])
{
    int h = hashfunc(k,m);
    for (int i = 0; i<m; i++)
    {
        int j = probe_lin(h,i,m);
        if(!ht[j]) return -1;
        if(*ht[j] == k) return j;
    }
    return -1; //Does not exist
}