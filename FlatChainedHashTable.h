
#ifndef _FLAT_CHAINED_HASH_TABLE_
#define _FLAT_CHAINED_HASH_TABLE_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIST_HEAD          13835058055282163711ul
#define HEAD_BIT_MASK      9223372036854775808ul
#define EMPTY_BIT_MASK     4611686018427387904ul
#define PROBE_BITS_MASK    4611686018427387903ul
#define NO_MORE_PROBES     4611686018427387903ul

typedef union {
    double   dbl;
    int64_t  i64;
    uint64_t u64;
    void*    ptr;
}element_t;

typedef struct {
    uint64_t meta;
    uint64_t key;
    element_t value;
}map_bucket_t;

typedef struct {
    uint64_t shift;
    uint64_t mask;
    uint64_t count;
    uint64_t capacity;
}hashtable_t;

typedef struct {
    hashtable_t hashtable;
    map_bucket_t buckets[];
}map_t;

typedef struct {
    uint64_t meta;
    uint64_t key;
}set_bucket_t;

typedef struct {
    hashtable_t hashtable;
    set_bucket_t buckets[];
}set_t;

float hashtable_load_factor(const hashtable_t* hashtable);
hashtable_t *hashtable_create(uint64_t initial_capacity, const uint64_t bucket_size);
void hashtable_destroy(hashtable_t **hashtable);

map_t *map_create(uint64_t initial_capacity);
element_t *map_get(map_t *map, const uint64_t key);
bool map_put(map_t **map, uint64_t key, element_t value);
void map_del(map_t **map, uint64_t key);

bool set_has(set_t *set, const uint64_t key);
bool set_put(set_t **set, const uint64_t key);
bool set_del(set_t **set, const uint64_t key);

#ifdef __cplusplus
};
#endif

static inline set_t *set_create(uint64_t initial_capacity){return (set_t*)hashtable_create(initial_capacity, sizeof(set_bucket_t));}
static inline void map_destroy(map_t **map){hashtable_destroy((hashtable_t**)map);}
static inline void set_destroy(set_t **set){hashtable_destroy((hashtable_t**)set);}
static inline float map_load_factor(const map_t *map){return hashtable_load_factor((hashtable_t*)map);}
static inline float set_load_factor(const set_t *set){return hashtable_load_factor((hashtable_t*)set);}

#endif
