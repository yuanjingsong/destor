//
// Created by 袁靖松 on 2019-12-12.
//
#include <utils/lru_cache.h>
#include "destor.h"
#include "index.h"

extern GHashTable* ctxtTable;

struct LIPA_cacheItem* new_lipa_cache_item(struct ctxtTableItem* ctxtTableItem) {
    struct LIPA_cacheItem* new_cacheItem = (struct LIPA_cacheItem*)(malloc(sizeof(struct LIPA_cacheItem)));

    new_cacheItem ->id = ctxtTableItem ->id;
    new_cacheItem ->hit_score = 0;
    new_cacheItem ->flag = 0;
    new_cacheItem ->tableItemPtr = ctxtTableItem;

    new_cacheItem ->kvpairs = g_hash_table_new_full(g_feature_hash, g_feature_equal, NULL, NULL);

    GSequenceIter* chunkIter = g_sequence_get_begin_iter(ctxtTableItem ->segment_ptr->chunks);
    GSequenceIter* chunkIterEnd = g_sequence_get_end_iter(ctxtTableItem->segment_ptr->chunks);
    for (int count = 0; chunkIter != chunkIterEnd; chunkIter = g_sequence_iter_next(chunkIter), count ++) {
        struct chunk* c = g_sequence_get(chunkIter);
        g_hash_table_insert(new_cacheItem->kvpairs, &c->fp, TEMPORARY_ID);
    }

    return new_cacheItem;
}


void free_lipa_cache(struct LIPA_cacheItem* cache) {
    printf("free lipa cache item\n");
    g_hash_table_destroy(cache->kvpairs);
    free(cache);
}

void feedback(struct LIPA_cacheItem* cacheItem, char* feature) {
    struct ctxtTableItem* target_ctxtTableItem = cacheItem ->tableItemPtr;
    target_ctxtTableItem ->update_time ++;
    target_ctxtTableItem -> score = target_ctxtTableItem -> score +
            ((double)(cacheItem->hit_score) - target_ctxtTableItem -> score) * (1.0 / target_ctxtTableItem -> update_time);
}
