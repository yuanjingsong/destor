//
// Created by 袁靖松 on 2019-12-11.
//

#include "destor.h"
#ifndef DESTOR_LIPA_CACHE_H
#define DESTOR_LIPA_CACHE_H


struct LIPA_cacheItem* new_lipa_cache_item(struct ctxtTableItem* ctxtTableItem);

void free_lipa_cache(struct LIPA_cacheItem* cache);


int lookup_fingerprint_in_lipa_cache(struct LIPA_cacheItem* cacheItem, fingerprint* fp) {
    return g_hash_table_contains(cacheItem->kvpairs,fp) ;
}

int lipa_cache_check_id(struct LIPA_cacheItem* cache, int id) {
    return cache ->id == id;
}

// feedback the chosen arm
void feedback(struct LIPA_cacheItem* cacheItem, char* feature);


#endif //DESTOR_LIPA_CACHE_H
