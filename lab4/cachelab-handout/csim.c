#include "cachelab.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <getopt.h>

int E, s, b;
int verbose;
FILE *fp = NULL;

typedef struct {
    int valid;
    uint64_t tag;
    int time;
}cacheLine;

typedef cacheLine* cacheSet;
typedef cacheSet* cache;

cache my_cache;

int hits, misses, evictions;

void simulate();
int findCache(uint64_t address);

void preprocess(int argc, char *argv[]) {
    int op;
    while ((op = getopt(argc, argv, "vs:E:b:t:")) != -1) {
        switch(op) {
            case 'v':
                verbose = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                fp = fopen(optarg, "r");
                break;
            default:
                break;
        }
    }
    // check whether the argument is complete
    if (fp == NULL || E == 0 || s == 0 || b == 0) {
        printf("argument is not complete\n");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    preprocess(argc, argv);
    simulate();
    printSummary(hits, misses, evictions);
    return 0;
}

void simulate() {
    int set_num = pow(2, s);
    int line_num = E;
    my_cache = (cache) malloc(sizeof(cacheSet) * set_num);
    for (int i = 0; i < set_num; i++) 
        my_cache[i] = (cacheSet) malloc(sizeof(cacheLine) * line_num);
    
    char buf[20];
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (buf[0] == 'I')
            continue;
        
        char operation;
        int size;
        uint64_t address;

        sscanf(buf, " %c %lx,%d", &operation, &address, &size);

        switch(operation) {
            case 'S':
                break;
            case 'L':
                break;
            case 'M':
                hits++;
                break;
            default:
                break;
        }
        findCache(address);

    }

    for(int i = 0; i < set_num; i++)
        free(my_cache[i]);
    free(my_cache);
    fclose(fp);
}

int findCache(uint64_t address) {
    uint32_t setID = (address >> b) & ((1 << s) - 1);
    uint64_t tag = (address >> (b + s));
    cacheSet current_set = my_cache[setID];

    int evicit = -1, invalid = -1, hit = 0;

    for (int i = 0; i < E; i++) {
        if (current_set[i].valid) {
            current_set[i].time++;
            if (current_set[i].tag == tag) {
                hits++;
                hit = 1;
                current_set[i].time = 1;
            }
            else if ((evicit == -1) || (current_set[i].time > current_set[evicit].time))
                evicit = i;
        } else {
            invalid = i;
        }
    }
    if (hit)
        return 0;
    misses++;
    if (invalid != -1) {
        current_set[invalid].valid = 1;
        current_set[invalid].tag = tag;
        current_set[invalid].time = 1;
        return 1;
    } else {
        evictions++;
        current_set[evicit].tag = tag;
        current_set[evicit].time = 1;
        return 2;
    }
}