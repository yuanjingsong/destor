//
// Created by 袁靖松 on 2019-12-10.
//

#include <destor.h>
#include "ctxtTable.h"
#include "index.h"
#include <time.h>
#include "jcr.h"
#include <storage/containerstore.h>
#include "index_buffer.h"
#include "fingerprint_cache.h"
#include "recipestore.h"
#include "kvstore.h"


// default context table list length

#define DEFAULT_LENGTH 4
#define DEFAULT_EPSILON 0.1

#define EPSILON_GREEDY_POLICY 1
#define RANDOM_POLICY 2



extern GHashTable *ctxtTable;
extern struct {
    struct container *container_buffer;

    GSequenceIter *chunks;
} storage_buffer;
extern struct index_buffer index_buffer;
extern struct index_overhead index_overhead;
extern int ItemId;

// At this function s-> id is always TEMPORARY_ID
// for each chunk in (s -> chunks), chunk id is always TEMPORARY_ID

void lipa_index_lookup(struct segment *s) {
    assert(s->features);
    int policy = EPSILON_GREEDY_POLICY;
    int CTXT_TABLE_ITEM_LENGTH = DEFAULT_LENGTH;


    if (policy == EPSILON_GREEDY_POLICY) {
        choose_champion = epsilon_greedy_policy;
    }else if (policy == RANDOM_POLICY) {
        choose_champion = random_policy;
    }

    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, s->features);

    struct ctxtTableItem* newItem = new_ctxtTableItem(s);
    struct ctxtTableItem* champion;

    while (g_hash_table_iter_next(&iter, &key, &value)) {

         /**
          * s -> features is GHashTable
          * and key => feature
          * value => NULL
          * iter s-> features 's key List
         * for each feature
         * map feature into context table
         * and choose champion
         * and prefetch champion and its followers fingerprint into cache
         */

        //key is fingerprint
        //value is NULL
        //char* feature = key;


        GList* ctxtList = NULL;
        if (g_hash_table_contains(ctxtTable, (char*) key)) {
            // this feature exist a ctxtTable List;
            ctxtList = g_hash_table_lookup(ctxtTable, (char*) key);
            int list_length = g_list_length(ctxtList);
            if (list_length == CTXT_TABLE_ITEM_LENGTH) {
                // correspond arm is full
                // you should remove lowest score ctxtTableItem
                struct ctxtTableItem *miniItem = find_mini_Item(ctxtList);
                ctxtList = g_list_remove(ctxtList, miniItem);
                free_ctxtTableItem(miniItem);
            }
        }

        ctxtList = g_list_append(ctxtList, newItem);
        g_hash_table_replace(ctxtTable, (char*) key, ctxtList);
        champion = choose_champion(ctxtList);

        //prefetch champion and followers fingerprint into cache
        fp_prefetch(ctxtList, champion, (char*) key);

    }

    //check each chunk in segment if it is duplicate
    GSequenceIter *chunkIter = g_sequence_get_begin_iter(s->chunks);
    GSequenceIter *chunkEndIter = g_sequence_get_end_iter(s->chunks);
    for (; chunkIter != chunkEndIter; chunkIter = g_sequence_iter_next(chunkIter)) {
        struct chunk *c = g_sequence_get(chunkIter);

        if (CHECK_CHUNK(c, CHUNK_FILE_START) || CHECK_CHUNK(c, CHUNK_FILE_END))
            continue;

        /**
         * TODO: whether check in the storage buffer and buffered fingerprints is needed?
         */


        /**
         * First check it in the storage buffer */
        if (storage_buffer.container_buffer
            && lookup_fingerprint_in_container(storage_buffer.container_buffer, &c->fp)) {
            c->id = get_container_id(storage_buffer.container_buffer);
            SET_CHUNK(c, CHUNK_DUPLICATE);
            SET_CHUNK(c, CHUNK_REWRITE_DENIED);
        }

        /**
         * First check the buffered fingerprints,
         * recently backup fingerprints.
         */

        GQueue *tq = g_hash_table_lookup(index_buffer.buffered_fingerprints, &c->fp);
        if (!tq) {
            tq = g_queue_new();
        } else if (!CHECK_CHUNK(c, CHUNK_DUPLICATE)) {
            struct indexElem *be = g_queue_peek_head(tq);
            c->id = be->id;
            SET_CHUNK(c, CHUNK_DUPLICATE);
        }

        /**
         * Check the fingerprint on fingerprint cache
         * fingerprint_cache_lookup return the truly id if chunk is duplicate
         * then if a chunk is duplicate, it has the his container id
         * so at filter phase, a chunk with temporary_id will be think it is unique
         P*/

        if (!CHECK_CHUNK(c, CHUNK_DUPLICATE)) {
            int64_t id = fingerprint_cache_lookup(&(c->fp));
            //printf("id is %d\n", id);

            if (id != TEMPORARY_ID) {
                // find item in cache
                c->id = id;
                SET_CHUNK(c, CHUNK_DUPLICATE);
            }else {
                index_overhead.lookup_requests_for_unique++;
            }
        }

        /**
         * After check cache, then write to recipe
         */

        struct indexElem *ne = (struct indexElem *)
                malloc(sizeof(struct indexElem));

        ne->id = c->id;
        memcpy(&ne->fp, &c->fp, sizeof(fingerprint));

        g_queue_push_tail(tq, ne);
        g_hash_table_replace(index_buffer.buffered_fingerprints, &ne->fp, tq);

        index_buffer.chunk_num++;
    }

}




void init_ctxtTable() {
    ctxtTable = g_hash_table_new_full(g_feature_hash, g_feature_equal, NULL, NULL);
    ItemId = 0;
    VERBOSE("initial ctxt table");
}

// use the score as comparable key
struct ctxtTableItem *find_mini_Item(GList *ItemList) {
    struct ctxtTableItem* miniItem = ItemList -> data;
    struct ctxtTableItem* currItem = miniItem;

    ItemList = g_list_next(ItemList);

    while (ItemList){
        currItem = ItemList -> data;
        if (currItem -> score < currItem -> score) {
            miniItem = currItem;
        }
        ItemList = g_list_next(ItemList);
    }

    return miniItem;
}

struct ctxtTableItem *find_max_Item(GList *ItemList) {

    struct ctxtTableItem* maxItem = ItemList -> data;
    struct ctxtTableItem* currItem;
    ItemList = g_list_next(ItemList);
    while(ItemList) {
        currItem = ItemList ->data;
        if (currItem -> score > currItem -> score) {
            maxItem = currItem;
        }
        ItemList = g_list_next(ItemList);
    }

    return maxItem;

}

double randomNumber(double min, double max) {
    double range = (max - min);
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

struct ctxtTableItem* epsilon_greedy_policy (GList *ctxtList) {
    printf("choose champion\n");
    assert(ctxtList);
    srand(time(NULL));
    double rand_num = randomNumber(0, 1.0);
    double threshold = 1 - DEFAULT_EPSILON;

    if (rand_num < threshold) {
        // choose max score segment
        return find_max_Item(ctxtList);
    } else {
        //random choose a segment
        rand_num = randomNumber(0, 1.0);
        int ctxtList_length = g_list_length(ctxtList);
        int index_ = (int) (rand_num * ctxtList_length);
        return g_list_nth_data(ctxtList, index_);
    }
}

struct ctxtTableItem* random_policy(GList* ctxtList) {
    srand(time(NULL));
    assert(ctxtList);
    double rand_num = randomNumber(0, 1.0);
    int ctxtList_length = g_list_length(ctxtList);
    int index_ = (int) (rand_num * ctxtList_length);
    return g_list_nth_data(ctxtList, index_);
}


/**
 * ctxtList is a list consists of ctxtTableItem
 * g_sequence_get(iter) return ctxtTableItem pointer
 * you should prefetch champion and his follower fingerprints into fingerprint cache
 * the number of followers can be determined by champion -> followers
 * @param ctxtList
 * @param champion
 */

void fp_prefetch(GList *ctxtList, struct ctxtTableItem *champion, char* feature) {
    assert(champion);
    GList* iter = g_list_find(ctxtList, champion);

    int iter_time = champion->followers;

    for (int counter = 0; counter <= iter_time && iter != NULL; iter = g_list_next(iter), counter ++) {
        //LIPA_fingerprint_cache_prefetch(champion->id, feature);
        LIPA_fingerprint_cache_prefetch( ((struct ctxtTableItem *)(iter->data))->id,
                feature);
    }

}