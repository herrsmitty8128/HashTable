
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "FlatChainedHashTable.h"

#define SAMPLE_SIZE 10000
uint64_t samples[SAMPLE_SIZE];


int main(){

    int i,j;
    //kvpair_t *ppair;
    //element_t v;
    int r = EXIT_SUCCESS;
    map_t *map = map_create(0);

    time_t s = 1666869947; //time(0);
    srand(s);
    printf("Seed = %ld\n",s);
    //srand(0);

    fprintf(stdout, "\n               Buckets\tCount\tLoad Factor\n");
    fprintf(stdout, "               -------\t-----\t-----------");

    for(int n = 1; n <= 3; n++){

        fprintf(stdout, "\nIteration #%d:\n",n);
        fprintf(stdout, "-------------\n");

        for(i = 0; i < SAMPLE_SIZE; i++){
            do{
                samples[i] = (uint32_t)rand();
                for(j = 0; samples[j] != samples[i]; j++);
            }while(j < i);
        }

        fprintf(stdout, "Initial State: %lu\t%lu,\t%f\n", map->hashtable.capacity, map->hashtable.count, map_load_factor(map));
        //fprintf(stdout, "Initial State: %lu\t%lu,\t%f\n", 1ul << map->bits, map->count, map_load_factor(map));
        
        for(i = 0; i < SAMPLE_SIZE; i++){
            //v = map_put(&map, samples[i], (element_t)samples[i]);
            if(!map_put(&map, samples[i], (element_t)samples[i])){ //, samples[i])){
                fprintf(stderr, "Insertion failed [%d] %lu\n", i, samples[i]);
                r = EXIT_FAILURE;
                goto end_test;
            }
            //v.u64 = samples[i];
            if(map->hashtable.count != (uint64_t)i+1){
                fprintf(stderr, "Size is incorrect [%d] [%lu]\n", i+1, map->hashtable.count);
                r = EXIT_FAILURE;
                goto end_test;
            }
            for(j = 0; j < i; j++){
                //printf("i = [%d] j = [%d]\n",i,j);
                element_t *x = map_get(map, samples[j]);
                if(!x){
                    fprintf(stderr, "Lookup failed! [%d] [%d] %lu\n", i, j, samples[j]);
                    goto end_test;
                }
                if(x->u64 != samples[j]){
                    fprintf(stderr, "Lookup value value not correct [%d] %lu [%d] %lu\n", i, x->u64, j, samples[j]);
                    goto end_test;
                }
            }
        }

        fprintf(stdout, "After Insert:  %lu,\t%lu,\t%f\n", map->hashtable.capacity, map->hashtable.count, map_load_factor(map));
        //fprintf(stdout, "After Insert:  %lu,\t%lu,\t%f\n", 1ul << map->bits, map->count, map_load_factor(map));
        
        for(i = 0; i < SAMPLE_SIZE; i++){
            map_del(&map, samples[i]);
            /*if(!buff){
                fprintf(stderr, "Removal failed [%lu] %lu\n", i, samples[i]);
                r = EXIT_FAILURE;
                goto end_test;
            }
            if(buff != samples[i]){
                fprintf(stderr, "Removal value incorrect %lu [%lu] %lu\n", buff, i, samples[i]);
                r = EXIT_FAILURE;
                goto end_test;
            }*/
            
            if(map->hashtable.count != (uint64_t)SAMPLE_SIZE - (i+1)){
                //fprintf(stderr, "Size incorrect after removal %lu [%lu] %lu\n", *buff, i, samples[i]);
                fprintf(stderr, "Size incorrect after removal [%d] %lu\n", i, samples[i]);
                r = EXIT_FAILURE;
                goto end_test;
            }
            element_t *x = map_get(map, samples[i]);
            if(x != NULL){
                fprintf(stderr, "Failed to remove [%d] [%d] %lu\n", i, j, samples[j]);
                r = EXIT_FAILURE;
                goto end_test;
            }
            for(j = i + 1; j < SAMPLE_SIZE; j++){
                x = map_get(map, samples[j]);
                if(!x){
                    fprintf(stderr, "Lookup failed after removal [%d] [%d] %lu\n", i, j, samples[j]);
                    r = EXIT_FAILURE;
                    goto end_test;
                }
                if(x->u64 != samples[j]){
                    fprintf(stderr, "Lookup value incorrect after removal %lu [%d] %lu\n", x->u64, j, samples[j]);
                    r = EXIT_FAILURE;
                    goto end_test;
                }
            }
            
        }

        fprintf(stdout, "After Remove:  %lu,\t%lu,\t%f\n", map->hashtable.capacity, map->hashtable.count, map_load_factor(map));
        //fprintf(stdout, "After Remove:  %lu,\t%lu,\t%f\n", 1ul << map->bits, map->count, map_load_factor(map));

    }
    
    end_test:

    fprintf(stdout, "\nEnd State:     %lu,\t%lu,\t%f\n\n", map->hashtable.capacity, map->hashtable.count, map_load_factor(map));
    //fprintf(stdout, "\nEnd State:     %lu,\t%lu,\t%f\n\n", 1ul << map->bits, map->count, map_load_factor(map));

    map_destroy(&map);
    
    return r;
}