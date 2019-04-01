//
// Created by borelset on 2019/3/20.
//

#include <stdint.h>
#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include "md5.h"

#define SymbolCount 256
#define DigistLength 16
#define SeedLength 64
#define MaxChunkSizeOffset 3
#define MinChunkSizeOffset 2

uint64_t g_gear_matrix[SymbolCount];
uint32_t g_min_chunk_size;
uint32_t g_max_chunk_size;
uint32_t g_expect_chunk_size;

enum{
    Mask_64B,
    Mask_128B,
    Mask_256B,
    Mask_512B,
    Mask_1KB,
    Mask_2KB,
    Mask_4KB,
    Mask_8KB,
    Mask_16KB,
    Mask_32KB,
    Mask_64KB,
    Mask_128KB
};

uint64_t g_condition_mask[] = {
        0x00001803110,// 64B
        0x000018035100,// 128B
        0x00001800035300,// 256B
        0x000019000353000,// 512B
        0x0000590003530000,// 1KB
        0x0000d90003530000,// 2KB
        0x0000d90103530000,// 4KB
        0x0000d90303530000,// 8KB
        0x0000d90703530000,// 16KB
        0x0003590703530000,// 32KB
        0x0007590703530000,// 64KB
        0x0007590713530000// 128KB
};

void fastcdc_init(uint32_t expectCS){
    char seed[SeedLength];
    for(int i=0; i<SymbolCount; i++){
        for(int j=0; j<SeedLength; j++){
            seed[j] = i;
        }

        g_gear_matrix[i] = 0;
        char md5_result[DigistLength];
        md5_state_t md5_state;
        md5_init(&md5_state);
        md5_append(&md5_state, seed, SeedLength);
        md5_finish(&md5_state, md5_result);

        memcpy(&g_gear_matrix[i], md5_result, sizeof(uint64_t));
    }

    g_min_chunk_size = expectCS >> MinChunkSizeOffset;
    g_max_chunk_size = expectCS << MaxChunkSizeOffset;
    g_expect_chunk_size = expectCS;
}


int fastcdc_chunk_data(unsigned char *p, int n){
    assert(p != NULL);
    uint64_t fp = 0;
    if(n < g_max_chunk_size){
        if(n <= g_min_chunk_size){
            return n;
        }else if (n < g_expect_chunk_size){
            for(int i=g_min_chunk_size; i<n; i++){
                fp = ( fp <<1 ) + g_gear_matrix[ p[i] ];
                if(!(fp & g_condition_mask[Mask_64KB])){
                    return i;
                }
            }
            return n;
        }else {
            for(int i=g_min_chunk_size; i<g_expect_chunk_size; i++){
                fp = ( fp <<1 ) + g_gear_matrix[ p[i] ];
                if(!(fp & g_condition_mask[Mask_64KB])){
                    return i;
                }
            }
            for(int i=g_expect_chunk_size; i<n; i++){
                fp = ( fp <<1 ) + g_gear_matrix[ p[i] ];
                if(!(fp & g_condition_mask[Mask_1KB])){
                    return i;
                }
            }
            return n;
        }
    }
    for(int i=g_min_chunk_size; i<g_expect_chunk_size; i++){
        fp = ( fp <<1 ) + g_gear_matrix[ p[i] ];
        if(!(fp & g_condition_mask[Mask_64KB])){
            return i;
        }
    }
    for(int i=g_expect_chunk_size; i<g_max_chunk_size; i++){
        fp = ( fp <<1 ) + g_gear_matrix[ p[i] ];
        if(!(fp & g_condition_mask[Mask_1KB])){
            return i;
        }
    }
    return g_max_chunk_size;
}