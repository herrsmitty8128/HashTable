
#include "FlatChainedHashTable.h"

#define HEAD_BIT_MASK    9223372036854775808ul
#define EMPTY_BIT_MASK   4611686018427387904ul
#define PROBE_BITS_MASK  4611686018427387903ul
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

void hashtable_destroy(hashtable_t **hashtable) {
    if(hashtable && *hashtable){
        free(*hashtable);
        *hashtable = NULL;
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
            if(meta == h) break;
            bucket = &map->buckets[meta];
            meta = bucket->meta;
        }
    }
    return NULL;
}

/*
static void map_rehash(map_t *map, map_bucket_t *old_bucket){
    
    old_bucket->meta = EMPTY_BIT_MASK;
    uint64_t h = hash((hashtable_t*)map, old_bucket->key);
    uint64_t *p,n,c,i;
    map_bucket_t *bucket = &map->buckets[h];
    uint64_t meta = bucket->meta;
    
    if(meta & EMPTY_BIT_MASK){
        bucket->meta = LIST_HEAD;
        bucket->key = old_bucket->key;
        bucket->value = old_bucket->value;
    }
    else{
        if(meta & HEAD_BIT_MASK){
            meta ^= HEAD_BIT_MASK;
        }
        else{
            
            i = hash((hashtable_t*)map, old_bucket->key);
            if(!(map->buckets[i].meta & HEAD_BIT_MASK)){
                i >>= 1;
                if(!(map->buckets[i].meta & HEAD_BIT_MASK)){
                    
                }
            }
            bucket->meta = LIST_HEAD;
            swap(&old_bucket->key, &old_bucket->value, bucket);
            for(;;){
                n = map->buckets[i].meta & PROBE_BITS_MASK;
                if(n == h) break;
                i = n;
            }
            h = i;
            bucket = &map->buckets[h];
            bucket->meta = (bucket->meta & HEAD_BIT_MASK) | meta;
        }

        while(meta != NO_MORE_PROBES){
            h = meta;
            bucket = &map->buckets[h];
            meta = bucket->meta;
        }
        
        for(p = &bucket->meta, n = 1, c = 1; n < map->hashtable.capacity; n++, c+=n){
            i = (h + c) & map->hashtable.mask;
            bucket = &map->buckets[i];
            if(bucket->meta & EMPTY_BIT_MASK){
                bucket->meta = NO_MORE_PROBES;
                bucket->key = old_bucket->key;
                bucket->value = old_bucket->value;
                *p = (*p & HEAD_BIT_MASK) | i;
                break;
            }
        }
    }
}


static bool map_grow(map_t **fmap){
    
    uint64_t i,old_capacity,meta;
    map_bucket_t *bucket;
    map_t *map = *fmap;

    // can't exceed the maximum capacity
    if(map->hashtable.capacity >= MAX_CAPACITY) return false;

    // ask the system to add more memory to our table
    map = (map_t*)realloc(map, hashtable_total_size(map->hashtable.capacity * 2, sizeof(map_bucket_t)));
    if(!map) return false;
    *fmap = map;
    
    // remember the old capacity for later
    old_capacity = map->hashtable.capacity;
    
    // adjust the hash table values
    map->hashtable.shift -= 1;
    map->hashtable.capacity <<= 1;
    map->hashtable.mask = map->hashtable.capacity - 1ul;
    
    // initialize the new memory
    for(i = old_capacity; i < map->hashtable.capacity; i++){
        map->buckets[i].meta = EMPTY_BIT_MASK;
    }
    
    // rehash the old key-value pairs
    i = old_capacity;
    do{
        i--;
        bucket = &map->buckets[i];
        meta = bucket->meta;
        if(meta & HEAD_BIT_MASK){
            meta ^= HEAD_BIT_MASK;
            for(;;){
                map_rehash(map, bucket);
                //bucket->meta = EMPTY_BIT_MASK;
                if(meta == NO_MORE_PROBES) break;
                bucket = &map->buckets[meta];
                meta = bucket->meta;
            }
        }
    }while(i != 0);

    return true;
}
*/

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

/*
static inline uint64_t find_empty_bucket(map_t *map, uint64_t i){
    for(uint64_t n = 1; n < map->hashtable.capacity; n++){
        i += n;
        i &= map->hashtable.mask;
        if(map->buckets[i].meta & EMPTY_BIT_MASK) break;
    }
    return i;
}
*/

bool map_put(map_t **fmap, uint64_t key, element_t value){

    if(hashtable_should_grow((hashtable_t*)*fmap)){
        if(!map_resize(fmap, true)) return false;
        //if(!map_grow(fmap)) return false;
    }
    
    map_t *map = *fmap;
    uint64_t h = hash((hashtable_t*)map, key);
    map_bucket_t *bucket = &map->buckets[h];
    uint64_t n,e,i,meta = bucket->meta;

    if(meta & EMPTY_BIT_MASK){
        bucket->meta = HEAD_BIT_MASK | h;
        bucket->key = key;
        bucket->value = value;
        map->hashtable.count++;
        return true;
    }

    i = h;
    
    if(meta & HEAD_BIT_MASK){
        
        meta ^= HEAD_BIT_MASK;
        
        for(;;){
            if(bucket->key == key){
                bucket->value = value;
                return true;
            }
            if(meta == h) break;
            i = meta;
            bucket = &map->buckets[i];
            meta = bucket->meta;
        }

        for(n = 1, e = i; n < map->hashtable.capacity; n++){
            e += n;
            e &= map->hashtable.mask;
            if(map->buckets[e].meta & EMPTY_BIT_MASK){
                bucket->meta = (bucket->meta & HEAD_BIT_MASK) | e;
                map->buckets[e] = (map_bucket_t){h,key,value};
                map->hashtable.count++;
                return true;
            }
        }

        return false;
    
    }

    do{
        e = meta; //i = meta;
        bucket = &map->buckets[e]; //bucket = &map->buckets[i];
        meta = bucket->meta & PROBE_BITS_MASK;
    }while(meta != h);

    //for(n = 1, e = i; n < map->hashtable.capacity; n++){
    for(n = 1; n < map->hashtable.capacity; n++){
        e += n;
        e &= map->hashtable.mask;
        if(map->buckets[e].meta & EMPTY_BIT_MASK){
            
            // move h to the empty bucket
            map->buckets[e].meta = map->buckets[h].meta;
            map->buckets[e].key = map->buckets[h].key;
            map->buckets[e].value = map->buckets[h].value;
            
            // point the current bucket to the empty bucket to remove h
            bucket->meta = (bucket->meta & HEAD_BIT_MASK) | e;
            
            // h is now ready to receive the original key-value pair
            map->buckets[h] = (map_bucket_t){(HEAD_BIT_MASK | h),key,value};
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
            if(meta == h) break;
            prev_meta = &last->meta;
            last = &map->buckets[meta];
            meta = last->meta;
        }
        if(erase){
            if(prev_meta)
                *prev_meta = (*prev_meta & HEAD_BIT_MASK) | h;
            erase->key = last->key;
            erase->value = last->value;
            last->meta = EMPTY_BIT_MASK;
            map->hashtable.count--;
            if(hashtable_should_shrink((hashtable_t*)map))
                map_resize(fmap, false);
        }
    }
}
