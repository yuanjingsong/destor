/*
 * fingerprint_cache.c
 *
 *  Created on: Mar 24, 2014
 *      Author: fumin
 */
#include "../destor.h"
#include "index.h"
#include "../storage/containerstore.h"
#include "../recipe/recipestore.h"
#include "../utils/lru_cache.h"
#include "fingerprint_cache.h"
#include "lipa_cache.h"
#include "kvstore.h"

static struct lruCache* lru_queue;

extern GHashTable* ctxtTable;

/* defined in index.c */
extern struct {
	/* Requests to the key-value store */
	int lookup_requests;
	int update_requests;
	int lookup_requests_for_unique;
	/* Overheads of prefetching module */
	int read_prefetching_units;
}index_overhead;

void init_fingerprint_cache(){
	switch(destor.index_category[1]){
	case INDEX_CATEGORY_PHYSICAL_LOCALITY:
		lru_queue = new_lru_cache(destor.index_cache_size,
				free_container_meta, lookup_fingerprint_in_container_meta);
		break;
	case INDEX_CATEGORY_LOGICAL_LOCALITY:
		// use my own fingerprint cache
	    if (destor.index_specific == INDEX_SPECIFIC_LIPA) {
	    	lru_queue = new_lru_cache(destor.index_cache_size,
	    			free_lipa_cache, lookup_fingerprint_in_lipa_cache);
			break;
		}

		lru_queue = new_lru_cache(destor.index_cache_size,
				free_segment_recipe, lookup_fingerprint_in_segment_recipe);

		break;
	default:
		WARNING("Invalid index category!");
		exit(1);
	}
}

/**
 *
 * @param fp
 * @return
 * if fingerprint are found in cache return the coreesponding
 * else return TEMPORARY_ID which means it is not in cache
 * always get id from htable
 */

int64_t fingerprint_cache_lookup(fingerprint *fp){
	switch(destor.index_category[1]){
		case INDEX_CATEGORY_PHYSICAL_LOCALITY:{
			struct containerMeta* cm = lru_cache_lookup(lru_queue, fp);
			if (cm)
				return cm->id;
			break;
		}
		case INDEX_CATEGORY_LOGICAL_LOCALITY:{
			if (destor.index_specific == INDEX_SPECIFIC_LIPA) {
                struct LIPA_cacheItem* cacheItem = lru_cache_lookup(lru_queue, fp);
                if (cacheItem) {
                    //update cache item hit
                    containerid id = g_hash_table_lookup(cacheItem ->kvpairs, fp);
                    if (id != TEMPORARY_ID) {
                        cacheItem ->hit_score ++;
                        return id;
                    }
                }
				break;
			}


			struct segmentRecipe* sr = lru_cache_lookup(lru_queue, fp);
			if(sr){
				struct chunkPointer* cp = g_hash_table_lookup(sr->kvpairs, fp);
				if(cp->id <= TEMPORARY_ID){
					WARNING("expect > TEMPORARY_ID, but being %lld", cp->id);
					assert(cp->id > TEMPORARY_ID);
				}
				return cp->id;
			}
			break;
		}
	}

	return TEMPORARY_ID;
}

void fingerprint_cache_prefetch(int64_t id){
	switch(destor.index_category[1]){
		case INDEX_CATEGORY_PHYSICAL_LOCALITY:{
			struct containerMeta * cm = retrieve_container_meta_by_id(id);
			index_overhead.read_prefetching_units++;
			if (cm) {
				lru_cache_insert(lru_queue, cm, NULL, NULL);
			} else{
				WARNING("Error! The container %lld has not been written!", id);
				exit(1);
			}
			break;
		}
		case INDEX_CATEGORY_LOGICAL_LOCALITY:{
			if (!lru_cache_hits(lru_queue, &id,
					segment_recipe_check_id)){
				/*
				 * If the segment we need is already in cache,
				 * we do not need to read it.
				 */
				// segements is a list of segmentsRecipe
				// use segment id to find segmentsRecipe
				GQueue* segments = prefetch_segments(id,
						destor.index_segment_prefech);
				index_overhead.read_prefetching_units++;
				VERBOSE("Dedup phase: prefetch %d segments into %d cache",
						g_queue_get_length(segments),
						destor.index_cache_size);
				struct segmentRecipe* sr;
				while ((sr = g_queue_pop_tail(segments))) {
					/* From tail to head */
					if (!lru_cache_hits(lru_queue, &sr->id,
							segment_recipe_check_id)) {
						lru_cache_insert(lru_queue, sr, NULL, NULL);
					} else {
						/* Already in cache */
						free_segment_recipe(sr);
					}
				}
				g_queue_free(segments);
			}
			break;
		}
	}
}

/**
 *
 * @param id is ctxtTableItem id
 * prefetch the corresponding segment item
 * warp it into LIPA_cache_Item and prefetch into cache
 * don't prefetch any more
 */
void LIPA_fingerprint_cache_prefetch(int64_t id, char* feature) {
    // check the whether segment is in cache
	if (!lru_cache_hits(lru_queue, id, lipa_cache_check_id)) {
		index_overhead.read_prefetching_units ++;
		struct ctxtTableItem* ctxtTableItem = LIPA_prefetch_item(feature, id);
		assert(ctxtTableItem);
		struct LIPA_cacheItem* cacheItem = new_lipa_cache_item(ctxtTableItem);
		lru_cache_insert(lru_queue, cacheItem, feedback, NULL);
	}
}


struct ctxtTableItem* LIPA_prefetch_item (char* feature, int64_t id) {
    GList* ctxtList = g_hash_table_lookup(ctxtTable, feature);
   	assert(ctxtList) ;
   	while (ctxtList) {
		if (((struct ctxtTableItem*) ctxtList->data) ->id == id) {
			return ctxtList->data;
		}
   		ctxtList = g_list_next(ctxtList);
   	}
   	return NULL;
}

void LIPA_cache_update(fingerprint *fp, containerid id) {

	GList* elem =  g_list_first(lru_queue->elem_queue);
	while (elem) {
	    if (lru_queue->hit_elem((struct LIPA_cacheItem*) (elem->data), fp)) {
	    	g_hash_table_replace(((struct LIPA_cacheItem*)(elem->data))->kvpairs,
	    			fp, id);
	    }
	    elem = g_list_next(elem);
	}

}
