
#include <benchmark/benchmark.h>
#include "FlatChainedHashTable.h"

#define MAX_COUNT 2000000
uint64_t myarray[MAX_COUNT];



static void map_insert(benchmark::State& state) {
    size_t range = state.range(0);
    map_t *map = map_create(1);
    for (auto _ : state){
        for(size_t i = 0; i < range; i++){
            //pair.key = myarray[i];
            //pair.value = myarray[i];
            map_put(&map, myarray[i], (element_t&)myarray[i]);
        }
    }
    state.counters["load_factor"] = map_load_factor(map);
    state.counters["ns_per_entry"] = benchmark::Counter((double)(range * state.iterations()) / (double)1000000000.0, benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
    map_destroy(&map);
}

BENCHMARK(map_insert)->Name("map_insert")->DenseRange(10000, MAX_COUNT, 25000)->Unit(benchmark::kNanosecond);



static void map_lookup(benchmark::State& state) {
    size_t range = state.range(0);
    map_t *map = map_create(1);
    for(size_t i = 0; i < range; i++){
        map_put(&map, myarray[i], (element_t&)myarray[i]);
    }
    for (auto _ : state){
        for(size_t i = 0; i < range; i++){
            benchmark::DoNotOptimize(map_get(map, myarray[i]));
        }
    }
    state.counters["load_factor"] = map_load_factor(map);
    state.counters["ns_per_entry"] = benchmark::Counter((double)(range * state.iterations()) / (double)1000000000.0, benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
    map_destroy(&map);
}

BENCHMARK(map_lookup)->Name("map_get")->DenseRange(10000, MAX_COUNT, 24000)->Unit(benchmark::kNanosecond);


static void map_erase(benchmark::State& state) {
    size_t range = state.range(0);
    map_t* map = map_create(0);
    for(size_t i = 0; i < range; i++){
        map_put(&map, myarray[i], (element_t&)myarray[i]);
    }
    state.counters["load_factor"] = map_load_factor(map);
    for (auto _ : state){
        for(size_t i = 0; i < range; i++)
            map_del(&map, myarray[i]);
    }
    state.counters["ns_per_entry"] = benchmark::Counter((double)(range * state.iterations()) / (double)1000000000.0, benchmark::Counter::kIsRate | benchmark::Counter::kInvert);
    map_destroy(&map);
}

BENCHMARK(map_erase)->Name("map_erase")->DenseRange(10000, MAX_COUNT, 25000)->Unit(benchmark::kNanosecond);



//BENCHMARK_MAIN();
int main(int argc, char** argv) {
    // other initialization code goes here
    srand(time(0));
    for(size_t i = 0; i < MAX_COUNT; i++) myarray[i] = (uint32_t)rand(); //(uint64_t)rand();

    ::benchmark::Initialize(&argc, argv); 
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1; 
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}

