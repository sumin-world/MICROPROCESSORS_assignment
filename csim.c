
/*
* 2022122051 김수민
* @file csim.c
* @brief This program simulates a cache with given parameters and a trace file.
* The purpose of this program is to simulate how a cache works in terms of loading, storing, and evicting data.
* The cache is represented by a structure consisting of sets and lines. Each set contains several lines, and each line has a valid bit, a tag, and a timestamp.
* The timestamp is used to implement the LRU (Least Recently Used) eviction policy. When the cache is full and a new line is accessed, the line with the oldest timestamp is evicted.
* The cache parameters including the number of sets, the number of lines per set, and the number of block bits are provided by command line arguments.
* The program reads a trace file, simulates cache access for each memory access in the trace, and outputs the number of hits, misses, and evictions.
*/


#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#include "cachelab.h"

/* Define the line structure in cache */
typedef struct {
    int valid;
    int tag;
    int timestamp;
} line_t;

/* Define the set structure in cache */
typedef struct {
    line_t *lines;
} set_t;

/* Define the cache structure */
typedef struct {
    set_t *sets;
    size_t set_num;
    size_t line_num;
} cache_t;

cache_t cache = {};
int set_bits = 0, block_bits = 0;
size_t hits = 0, misses = 0, evictions = 0;
int current_time = 0;

/*
* @brief - Simulate the cache access with the given address
* @params[in] - addr: The address to be accessed in the cache
*/
void simulate(int addr) {
    size_t set_index = (addr >> block_bits) & ((1 << set_bits) - 1);
    int tag = addr >> (set_bits + block_bits);
    set_t *set = &cache.sets[set_index];

    for (size_t i = 0; i < cache.line_num; ++i) {
        if (!set->lines[i].valid || set->lines[i].tag != tag) continue;

        ++hits;
        set->lines[i].timestamp = ++current_time;
        return;
    }

    ++misses;
    int min_time_index = 0;
    for (size_t i = 0; i < cache.line_num; ++i) {
        if (!set->lines[i].valid) {
            set->lines[i].valid = 1;
            set->lines[i].tag = tag;
            set->lines[i].timestamp = ++current_time;
            return;
        }
        if (set->lines[i].timestamp < set->lines[min_time_index].timestamp)
            min_time_index = i;
    }

    ++evictions;
    set->lines[min_time_index].tag = tag;
    set->lines[min_time_index].timestamp = ++current_time;
}

/*
* @brief - Main function to simulate the cache with the given trace file
* @params[in] - argc: The number of command line arguments
* @params[in] - argv: The command line arguments
*/
int main(int argc, char *argv[]) {
    FILE *file = NULL;
    for (int opt; (opt = getopt(argc, argv, "s:E:b:t:")) != -1;) {
        switch (opt) {
            case 's':
                set_bits = atoi(optarg);
                cache.set_num = 1 << set_bits;
                break;
            case 'E':
                cache.line_num = atoi(optarg);
                break;
            case 'b':
                block_bits = atoi(optarg);
                break;
            case 't':
                file = fopen(optarg, "r");
                break;
            default:
                return 1;
        }
    }

    cache.sets = malloc(sizeof(set_t) * cache.set_num);
    for (int i = 0; i < cache.set_num; ++i) {
        cache.sets[i].lines = calloc(sizeof(line_t), cache.line_num);
    }

    char operation;
    int address;
    while (fscanf(file, " %c %x%*c%*d", &operation, &address) != EOF) {
        if (operation == 'I') continue;
        simulate(address);
        if (operation == 'M') simulate(address);
    }

    printSummary(hits, misses, evictions);
    
    for (size_t i = 0; i < cache.set_num; ++i) {
        free(cache.sets[i].lines);
    }
    free(cache.sets);
    fclose(file);
    return 0;
}
