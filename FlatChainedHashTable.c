
#include "FlatChainedHashTable.h"

#define LIST_HEAD        13835058055282163711ul
#define HEAD_BIT_MASK    9223372036854775808ul
#define EMPTY_BIT_MASK   4611686018427387904ul
#define PROBE_BITS_MASK  4611686018427387903ul
#define NO_MORE_PROBES   4611686018427387903ul
#define MAX_CAPACITY     576460752303423488ul  // 2^59
#define MIN_CAPACITY     8ul

/*
 * The correct multiplier constant for the hash function is based on the golden ratio.
 * The golden ratio can be calculated with python3 using the following statements:
 *     >>> from decimal import Decimal
 *     >>> golden_ratio = Decimal((Decimal(1.0) + Decimal.sqrt(Decimal(5.0)))/ Decimal(2.0))
 *     >>> golden_ratio
 *     >>> 1.618033988749894848204586834
 * For 64-bit values use 2^64 / golden_ratio = (uint64_t)(11400714819323198486)
 * For 32-bit values use 2^32 / golden_ratio = (uint64_t)(2654435769)
 */
static inline uint64_t hash(const hashtable_t *hashtable, const uint64_t key){
    return (key * 11400714819323198486ul) >> hashtable->shift;
}

/* Is the load factor greater than or equal to 0.9375? */
static inline bool hashtable_should_grow(const hashtable_t *hashtable){
    return hashtable->count >= (hashtable->capacity - (hashtable->capacity >> 4));
}

/* Is the load factor is less than 0.375? */
static inline bool hashtable_should_shrink(const hashtable_t *hashtable){
    return hashtable->count <= (hashtable->capacity >> 2) + (hashtable->capacity >> 3);
}

float hashtable_load_factor(const hashtable_t *hashtable){
    return hashtable->capacity == 0 ? 0.0f : (float)hashtable->count / (float)hashtable->capacity;
}

static inline uint64_t hashtable_total_size(const uint64_t capacity, const uint64_t bucket_size){
    return (capacity * bucket_size) + sizeof(hashtable_t);
}

hashtable_t *hashtable_create(uint64_t initial_capacity, const uint64_t bucket_size){
    if(initial_capacity < MIN_CAPACITY) initial_capacity = MIN_CAPACITY;
    if(initial_capacity > MAX_CAPACITY) initial_capacity = MAX_CAPACITY;
    uint64_t bits = (uint64_t)ceil(log2(initial_capacity));
    initial_capacity = 1ul << bits;
    hashtable_t *table = (hashtable_t*)malloc(hashtable_total_size(initial_capacity, bucket_size));
    if(table){
        table->shift = 64ul - bits;
        table->count = 0ul;
        table->capacity = initial_capacity;
        table->mask = initial_capacity - 1ul;
    }
    return table;
}

map_t *map_create(uint64_t initial_capacity){
    map_t *map = (map_t*)hashtable_create(initial_capacity, sizeof(map_bucket_t));
    if(map){
        for(uint64_t i = 0; i < map->hashtable.capacity; i++){
            map->buckets[i].meta = EMPTY_BIT_MASK;
        }
    }
    return map;
}

void hashtable_destroy(hashtable_t **hasttable) {
    if(hasttable && *hasttable){
        free(*hasttable);
        *hasttable = NULL;
    }
}

element_t *map_get(map_t *map, const uint64_t key) {
    
    uint64_t h = hash((hashtable_t*)map, key);
    map_bucket_t *bucket = &map->buckets[h];
    uint64_t meta = bucket->meta;

    if(meta & HEAD_BIT_MASK){
        for(;;){
            if(bucket->key == key) return &bucket->value;
            meta &= PROBE_BITS_MASK;
            if(meta == NO_MORE_PROBES) break;
            bucket = &map->buckets[meta];
            meta = bucket->meta;
        }
    }
    return NULL;
}

static void swap(uint64_t *key, element_t *value, map_bucket_t *bucket){

    element_t temp;
    
    temp.u64 = *key;
    *key = bucket->key;
    bucket->key = temp.u64;

    temp = *value;
    *value = bucket->value;
    bucket->value = temp;
}

static bool map_resize(map_t **fmap, bool action){
    map_t *map = *fmap;
    uint64_t new_capacity;
    if(action){
        if(map->hashtable.capacity >= MAX_CAPACITY) return false;
        new_capacity = map->hashtable.capacity * 2;
    }
    else{
        if(map->hashtable.capacity <= MIN_CAPACITY) return false;
        new_capacity = map->hashtable.capacity / 2;
    }
    map = map_create(new_capacity);
    if(!map) return false;
    for(uint64_t i = 0; i < (*fmap)->hashtable.capacity; i++){
        map_bucket_t *b = &(*fmap)->buckets[i];
        if((b->meta & EMPTY_BIT_MASK) == 0ul){
            if(!map_put(&map, b->key, b->value)){
                map_destroy(&map);
                return false;
            }
        }
    }
    map_destroy(fmap);
    *fmap = map;
    return true;
}

bool map_put(map_t **fmap, uint64_t key, element_t value){

    if(hashtable_should_grow((hashtable_t*)*fmap)){
        if(!map_resize(fmap, true)) return false;
    }

    map_t *map = *fmap;
    uint64_t *p,n,c,i,h = hash((hashtable_t*)map, key);
    map_bucket_t *bucket = &map->buckets[h];
    uint64_t meta = bucket->meta;

    if(meta & EMPTY_BIT_MASK){
        bucket->meta = LIST_HEAD;
        bucket->key = key;
        bucket->value = value;
        map->hashtable.count++;
        return true;
    }

    if(meta & HEAD_BIT_MASK){
        meta ^= HEAD_BIT_MASK;
        for(;;){
            if(bucket->key == key){
                bucket->value = value;
                return true;
            }
            if(meta == NO_MORE_PROBES) break;
            h = meta;
            bucket = &map->buckets[h];
            meta = bucket->meta;
        }
    }
    else{
        bucket->meta = LIST_HEAD;
        swap(&key, &value, bucket);

        // find the previous index that points to h
        i = hash((hashtable_t*)map, key);
        for(;;){
            n = map->buckets[i].meta & PROBE_BITS_MASK;
            if(n == h) break;
            i = n;
        }
        h = i;

        bucket = &map->buckets[h];
        bucket->meta = (bucket->meta & HEAD_BIT_MASK) | meta;
        while(meta != NO_MORE_PROBES){
            h = meta;
            bucket = &map->buckets[h];
            meta = bucket->meta;            
        }
    }
    
    // Append the key-value pair to the end of the list by
    // locating the next emtpy bucket using a quadratic search.
    for(p = &bucket->meta, n = 1, c = 1; n < map->hashtable.capacity; n++, c+=n){
        i = (h + c) & map->hashtable.mask;
        bucket = &map->buckets[i];
        if(bucket->meta & EMPTY_BIT_MASK){
            bucket->meta = NO_MORE_PROBES;
            bucket->key = key;
            bucket->value = value;
            *p = (*p & HEAD_BIT_MASK) | i;
            map->hashtable.count++;
            return true;
        }
    }
    
    return false;
}

void map_del(map_t **fmap, uint64_t key) {
    
    map_t *map = *fmap;
    uint64_t h = hash((hashtable_t*)map, key);
    map_bucket_t  *erase = NULL, *last = &map->buckets[h];
    uint64_t  *prev_meta = NULL, meta = last->meta;
    
    if(meta & HEAD_BIT_MASK){
        meta ^= HEAD_BIT_MASK;
        for(;;){
            if(last->key == key) erase = last;
            if(meta == NO_MORE_PROBES) break;
            prev_meta = &last->meta;
            last = &map->buckets[meta];
            meta = last->meta;
        }
        if(erase){
            if(prev_meta)
                *prev_meta = (*prev_meta & HEAD_BIT_MASK) | NO_MORE_PROBES;
            erase->key = last->key;
            erase->value = last->value;
            last->meta = EMPTY_BIT_MASK;
            map->hashtable.count--;
            if(hashtable_should_shrink((hashtable_t*)map))
                map_resize(fmap, false);
        }
    }
}
