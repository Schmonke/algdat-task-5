#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <stdarg.h>
#include <time.h>

/**
 * Struct for the Hash Context
 */
typedef struct hash_context
{
    unsigned long long mult_A;
    unsigned int capacity_pow2exp;
    unsigned int capacity;
} hash_context;

/**
 * Struct for the probe context
 */
typedef struct probe_context
{
    hash_context *hash_ctx;
    int key;
    unsigned int hash1;
    unsigned int hash2;
    size_t capacity;
} probe_context;

// A general typedef for every probe function
typedef size_t probe_func(probe_context *ctx, int i);

/**
 * Struct for the hash table entries
 */
typedef struct
{
    bool exists;
    int value;
} hash_table_entry;

/**
 * Struct for the Hash Table
 */
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
 * Swaps values in two pointer locations.
 */
void swap(int *a, int *b)
{
    int s = *a;
    *a = *b;
    *b = s;
}

/**
 * Create random array
 * which creates a random array with all unique numbers
 * from c lib rand() function and our own way of multiplication
 * to ensure unique numbers
 */
int *create_random_unique_array(size_t length)
{
    const int step = INT_MAX / length;
    const int start = 0;
    int *array = calloc(length, sizeof(int));
    int acc = 0;
    for (size_t i = 0; i < length; i++)
    {
        acc += ((rand() % step) + 1);
        array[i] = acc;
    }
    for (size_t i = 0; i < length; i++)
    {
        swap(&array[i], &array[rand() % length]);
    }
    return array;
}

/**
 * Returns the nearest power of 2 exponent than will create a
 * number larger than specified value.
 */
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

/**
 * Rounds number to the next power of two.
 */
size_t pow2_round(size_t value)
{
    return (size_t)1 << pow2_round_exponent(value);
}

/**
* Calculates and returns the loadfactor.
*/
float get_load_factor(hash_table *table)
{
    return ((float)table->entries / (float)table->capacity) * 100;
}

/**
 * Determines the capacity and context 
 * with the equation 0.5*2^x (sqrt(5)-1)
 */
void hash_context_init(hash_table *table)
{
    const double sqrt5 = 2.236067977;
    table->hash_ctx = (hash_context){
        .mult_A = (unsigned long long)(0.5 * table->capacity * (sqrt5 - 1)),
        .capacity_pow2exp = table->capacity_pow2exp,
        .capacity = table->capacity,
    };
}

/**
* First hashing function with a multiplicative implementation
*/
unsigned int hash1(hash_context ctx, int key)
{
    return key * ctx.mult_A >> (sizeof(int) * CHAR_BIT - ctx.capacity_pow2exp);
}

/**
* Second hashing function with a folding hash implementation
*/
unsigned int hash2(hash_context ctx, int key)
{
    unsigned int mask = ctx.capacity - 1;
    unsigned int h = 0;
    unsigned int shift = 0;
    unsigned int capacity_pow2exp = ctx.capacity_pow2exp;
    while (mask != 0)
    {
        h = (mask & key) >> shift;
        mask <<= capacity_pow2exp;
        shift += capacity_pow2exp;
    }

    return h | 1; // make the number always odd at the expense of more collisions
}

size_t probe_linear(probe_context *ctx, int i)
{
    return (ctx->hash1 + i) % ctx->capacity;
}

size_t probe_quadratic(probe_context *ctx, int i)
{
    const double c = 0.5;
    const double ic = i * c;
    size_t result = (size_t)(ctx->hash1 + ic + ic * i + 1);
    return result % ctx->capacity;
}

size_t probe_doublehash(probe_context *ctx, int i)
{
    // hash2 cannot be 0 as it will cancel multiplication by i
    if (ctx->hash2 == 0)
    {
        ctx->hash2 = hash2(*ctx->hash_ctx, ctx->key);
    }
    return (ctx->hash1 + i * ctx->hash2) % ctx->capacity;
}

/**
* Adds a single hastable entry to the specified hashtable.
* If key already contains a value it tries to probe again
* to antoher key until it finds a free on. 
* Returns number of collisions until value got placed. 
*/
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
    if (colls == table->capacity)
    {
        printf("Table full, laddies/lassies!\n");
    }

    return colls;
}
/**
* Creates a hashtable with a specified capacity and probe function.
* The hashtable capacity is set to the closest power two, larger than 
* the specified min_capacity.
*/
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

/**
* Frees the hastable allocated memory to avoid memory leaks. 
*/
void hash_table_free(hash_table *table)
{
    free(table->values);
    free(table);
}

/**
* Adds all values from an array to the specified hashtable.
* Returns the amount of collisions occuring while adding values.
*/
size_t hash_table_add_all(hash_table *table, int *values, size_t values_length)
{
    size_t col = 0;
    for (size_t i = 0; i < values_length; i++)
    {
        col += hash_table_add(table, values[i]);
    }
    return col;
}

int main(int argc, char *argv[])
{
    int column_size = 11;

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
        printf("Creating tables (load 50%%-100%%) for %s\n", probe_types[i].name);
        printf(
            "%*s | %*s | %*s | %*s | %*s\n",
            column_size, "Load-factor",
            column_size, "Capacity",
            column_size, "Entries",
            column_size, "Collisions",
            column_size, "Time (ms)");
        for (int j = 0; j < fill_ratios_length; j++)
        {
            struct timespec start, end;
            float fill_ratio = fill_ratios[j];
            hash_table *table = hash_table_create(table_bound, probe_types[i].probe);

            if (clock_gettime(CLOCK_REALTIME, &start))
            {
                printf("Time failure");
                return -1;
            }
            size_t col = hash_table_add_all(table, rand_array, table_size * fill_ratio);

            if (clock_gettime(CLOCK_REALTIME, &end))
            {
                printf("Time failure");
                return -1;
            }
            printf("%*.0f%% | %*lu | %*lu | %*lu | %*.3f\n",
                   column_size - 1, get_load_factor(table),
                   column_size, table->capacity,
                   column_size, table->entries,
                   column_size, col,
                   column_size, (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0);

            hash_table_free(table);
        }
        printf("\n");
    }
    return 0;
}