#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>

typedef struct hash_context
{
    unsigned long long mult_A;
    unsigned int capacity_pow2exp;
    unsigned int capacity;
} hash_context;

typedef struct probe_context
{
    hash_context *hash_ctx;
    int key;
    unsigned int hash1;
    unsigned int hash2;
    size_t capacity;
} probe_context;

typedef size_t probe_func(probe_context *ctx, int i);

typedef struct
{
    bool exists;
    int value;
} hash_table_entry;

typedef struct
{
    size_t capacity_pow2exp;
    size_t capacity;
    size_t entries;
    size_t collisions;
    hash_table_entry *values;

    probe_func *probe;
    hash_context hash_ctx;
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
    const int step = INT_MAX / length;
    const int start = 0;
    int *array = calloc(length, sizeof(int));
    for (size_t i = 0; i < length; i++)
    {
        array[i] = start + i * step;
    }
    for (size_t i = 0; i < length; i++)
    {
        swap(&array[i], &array[rand() % length]);
    }
    return array;
}

size_t pow2_round_exponent(size_t value)
{
    const int bits = sizeof(size_t) * CHAR_BIT;
    if (value & 1 << (bits - 1))
    {
        return 0;
    }
    for (int i = bits - 2; i > 0; i--)
    {
        if (value & (size_t)1 << i)
        {
            return (i + 1);
        }
    }
    return 0;
}

size_t pow2_round(size_t value)
{
    return (size_t)1 << pow2_round_exponent(value);
}

//Calculate load factor
float get_load_factor(hash_table *table)
{
    return ((float)table->entries / (float)table->capacity) * 100;
}

void hash_context_init(hash_table *table)
{
    const double sqrt5 = 2.236067977;
    table->hash_ctx = (hash_context){
        .mult_A = (unsigned long long)(0.5 * table->capacity * (sqrt5 - 1)),
        .capacity_pow2exp = table->capacity_pow2exp,
        .capacity = table->capacity,
    };
}

//Hashfunc 1
unsigned int hash1(hash_context ctx, int key)
{
    // multiplicative hash implementation
    return key * ctx.mult_A >> (sizeof(int) * CHAR_BIT - ctx.capacity_pow2exp);
}

//Hashfunc 2
unsigned int hash2(hash_context ctx, int key)
{
    // folding hash implementation
    unsigned int mask = ctx.capacity - 1;
    unsigned int h = 0;
    unsigned int shift = 0;
    while (mask != 0)
    {
        h = (mask & key) >> shift;
        mask <<= ctx.capacity_pow2exp;
        shift += ctx.capacity_pow2exp;
    }

    return h | 1;
}

size_t probe_linear(probe_context *ctx, int i)
{
    return (ctx->hash1 + i) % ctx->capacity;
}

//Uses prime numbers as constants to minimize chance of common denominator.
size_t probe_quadratic(probe_context *ctx, int i)
{
    const int c = 2;
    const int ic = (i / c);
    // equivalent to (h + i*0.5 + 0.5*i^2)
    size_t result = (ctx->hash1 + ic + ic * i);
    return result % ctx->capacity;
}

// h2 and m must be relative prime
size_t probe_doublehash(probe_context *ctx, int i) //int h1, int h2, int i, size_t m)
{
    // hash2 cannot be 0 as it will cancel multiplication by i
    if (ctx->hash2 == 0)
    {
        ctx->hash2 = hash2(*ctx->hash_ctx, ctx->key);
    }
    return (ctx->hash1 + i * ctx->hash2) % ctx->capacity;
}

size_t hash_table_add(hash_table *table, int v)
{
    const size_t capacity = table->capacity;
    hash_context *hash_ctx = &table->hash_ctx;
    probe_context ctx = (probe_context){
        .hash_ctx = hash_ctx,
        .key = v,
        .capacity = table->capacity,
        .hash1 = hash1(*hash_ctx, v),
        .hash2 = 0,
    };

    size_t colls = 0;

    for (int i = 0; i < capacity; i++)
    {
        int j = table->probe(&ctx, i);
        hash_table_entry *value = &table->values[j];
        if (!value->exists)
        {
            value->exists = true;
            value->value = v;
            table->entries++;
            break;
        }
        colls++;
    }

    return colls;
}

hash_table *hash_table_create(size_t min_capacity, probe_func *probe)
{
    hash_table *table = calloc(1, sizeof(hash_table));
    table->capacity_pow2exp = pow2_round_exponent(min_capacity);
    table->capacity = pow2_round(min_capacity);
    table->values = calloc(table->capacity, sizeof(hash_table_entry));
    table->probe = probe;
    hash_context_init(table);
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
        {"linear", &probe_linear},
        {"quadratic", &probe_quadratic},
        {"double-hash", &probe_doublehash}};
    const int probe_types_length = sizeof(probe_types) / sizeof(probe_types[0]);

    const float fill_ratios[] = {0.5, 0.8, 0.9, 0.99, 1.0};
    const int fill_ratios_length = sizeof(fill_ratios) / sizeof(fill_ratios[0]);

    int table_bound = 10000000;

    if (argc > 1) //User can specify capaicity
    {
        table_bound = atoi(argv[1]);
    }

    srand(time(NULL));

    int table_size = pow2_round(table_bound);

    printf("Generating %d unique numbers...\n\n", table_size);
    int *rand_array = create_random_unique_array(table_size);

    for (int i = 0; i < probe_types_length; i++)
    {
        for (int j = 0; j < fill_ratios_length; j++)
        {
            struct timespec start, end;
            float fill_ratio = fill_ratios[j];
            printf("Creating table for %s\n", probe_types[i].name);
            hash_table *table = hash_table_create(table_bound, probe_types[i].probe);
            printf("Filling table (%02.0f%%) ...\n", fill_ratio * 100);

            if (clock_gettime(CLOCK_REALTIME, &start))
            {
                printf("Time failure");
                return -1;
            }
            int col = hash_table_add_all(table, rand_array, table_size * fill_ratio);

            if (clock_gettime(CLOCK_REALTIME, &end))
            {
                printf("Time failure");
                return -1;
            }

            printf("Time       : %.3fms\n", (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0);
            printf("Capacity   : %ld\n", table->capacity);
            printf("Collisions : %d\n", col);
            printf("Loadfactor : %.0f%%\n", get_load_factor(table));
            printf("Entries    : %ld\n", table->entries);
            printf("Freeing table...\n\n");
            hash_table_free(table);
        }
    }
    return 0;
}