//
// Created by 袁靖松 on 2019-12-12.
//
#include <utils/lru_cache.h>
#include <recipe/recipestore.h>
#include "destor.h"
#include "index.h"
#include "kvstore.h"
#include "fingerprint_cache.h"

extern GHashTable* ctxtTable;

struct LIPA_cacheItem* new_lipa_cache_item(struct ctxtTableItem* ctxtTableItem, struct segmentRecipe* sr) {
    struct LIPA_cacheItem* new_cacheItem = (struct LIPA_cacheItem*)(malloc(sizeof(struct LIPA_cacheItem)));

    new_cacheItem ->id = ctxtTableItem ->id;
    new_cacheItem ->hit_score = 0;
    new_cacheItem ->flag = 0;
    new_cacheItem ->tableItemPtr = ctxtTableItem;

    new_cacheItem ->kvpairs = g_hash_table_new_full(g_feature_hash, g_feature_equal, NULL, NULL);
    if (sr == NULL) {

        GSequenceIter* chunkIter = g_sequence_get_begin_iter(ctxtTableItem->segment_ptr->chunks);
        GSequenceIter* chunkEnd = g_sequence_get_end_iter(ctxtTableItem->segment_ptr->chunks);

        for(; chunkIter != chunkEnd; chunkIter = g_sequence_iter_next(chunkIter)){
            struct chunk* c = g_sequence_get(chunkIter);
            g_hash_table_insert(new_cacheItem->kvpairs, &c->fp, TEMPORARY_ID):
        }

    }else {
        GHashTableIter tableIter;
        gpointer key, value;
        g_hash_table_iter_init(&tableIter, sr->kvpairs);
        while (g_hash_table_iter_next(&tableIter, &key, &value)) {
            g_hash_table_insert(new_cacheItem->kvpairs, &key, value);
        }

    }



    return new_cacheItem;
}

// if a cache item is free
// then the info it records will destroyed,
// and when a new lipa cache create
// it will lose the information
// so it will lower the ratio

void free_lipa_cache(struct LIPA_cacheItem* cache) {
    g_hash_table_destroy(cache->kvpairs);
    free(cache);
}

void feedback(struct LIPA_cacheItem* cacheItem, char* feature) {
    struct ctxtTableItem* target_ctxtTableItem = cacheItem ->tableItemPtr;
    assert(target_ctxtTableItem);
    target_ctxtTableItem ->update_time ++;
    target_ctxtTableItem -> score = target_ctxtTableItem -> score +
            ((double)(cacheItem->hit_score) - target_ctxtTableItem -> score) * (1.0 / target_ctxtTableItem -> update_time);
}
