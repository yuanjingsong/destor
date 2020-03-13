//
// Created by 袁靖松 on 2020/3/11.
//

#include <glib.h>
#include <utils/lru_cache.h>
#include <utils/priority_cache.h>
#include <utils/sync_queue.h>
#include <storage/containerstore.h>
#include "destor.h"
#include "restore.h"
#include "jcr.h"
#include "index.c"

struct {
    // A list of container
    GSequence* faa_area;
    // faa_area_size mean how many active container in faa_area
    int64_t faa_area_size;
    // faa_area_max_size means how many max container in faa_area
    int64_t faa_area_max_size;
    int faa_area_full;

    // A list of chunk
    GSequence* informed_chunk_cache;
    int64_t informed_chunk_cache_size;
    int64_t informed_chunk_cache_max_size;

    // A list of
    struct lruCache* p_cache;
    struct priorityCache* f_cache;

} law_area;

struct fab_container {
    int32_t data_size;
    GSequence* chunk_lst;

    //record chunks with map
    GHashTable* chunk_map;
} ;

struct fab_container* create_fab_container() {
    struct fab_container* container = (struct fab_container*) (malloc(sizeof(struct fab_container)));
    container-> data_size = 0;
    container-> chunk_lst = g_sequence_new(free_chunk);
    container-> chunk_map = g_hash_table_new_full(g_feature_hash, g_feature_equal, free, NULL) ;
}

void free_fab_container(struct fab_container* c) {
    g_sequence_free(c->chunk_lst);
    free(c);
}

void add_chunk_to_fab_container(struct fab_container* c, struct chunk* ck) {
    g_sequence_append(c->chunk_lst, ck);
    c->data_size += ck->size;
    g_hash_table_insert(c->chunk_map, &ck->fp, NULL);
}

void update_chunk_in_fab_container(struct fab_container* c, fingerprint* fp) {
    // update fab container corresponding chunk id status
    // TODO: copy data
    GSequenceIter* iter = g_sequence_get_begin_iter(c->chunk_lst);
    GSequenceIter* end = g_sequence_get_end_iter(c->chunk_lst);
    struct chunk* ck = NULL;
    for (; iter != end ; iter = g_sequence_iter_next(iter)) {
        ck = g_sequence_get(iter);
        if (g_fingerprint_equal(&ck->fp, fp)) {
            SET_CHUNK(ck, CHUNK_READY);
        }
    }
}

void update_chunk_in_fab_container_with_con (struct fab_container* fab_c, struct container* con) {
    GSequenceIter* iter = g_sequence_get_begin_iter(fab_c ->chunk_lst) ;
    GSequenceIter* end = g_sequence_get_end_iter(fab_c->chunk_lst);
    struct chunk* ck = NULL ;
    for (; iter != end; iter = g_sequence_iter_next(iter)) {
        ck = g_sequence_get(iter);
        if (g_hash_table_contains(con->meta.map, ck->fp)) {
            SET_CHUNK(ck, CHUNK_READY);
        }
    }
}

int fab_container_overflow(struct fab_container* c, int32_t size) {
    if (c->data_size + size > CONTAINER_SIZE)
        return 1;
    return 0;
}

int lookup_chunk_in_chunk_cache(struct chunk *data, fingerprint* fp) {
    return memcmp(data->fp, fp, sizeof(fingerprint)) ;
}


int fp_in_info_chunk_cache(fingerprint* fp) {
    int priority = law_area.informed_chunk_cache_max_size;
    GSequenceIter* iter = g_sequence_get_begin_iter(law_area.informed_chunk_cache);
    GSequenceIter* end = g_sequence_get_end_iter(law_area.informed_chunk_cache);
    for (; iter != end; iter = g_sequence_iter_next(iter), priority--) {
        if (g_fingerprint_equal(fp, &((struct chunk*)g_sequence_get(iter))->fp)) {
            return priority;
        }
    }
    return 0;
}

int fp_in_faa_area(fingerprint* fp) {
    GSequenceIter* iter = g_sequence_get_begin_iter(law_area.faa_area);
    GSequenceIter* end = g_sequence_get_end_iter(law_area.faa_area);
    struct fab_container* c = NULL;
    for (; iter != end; iter = g_sequence_iter_next(iter)) {
        c = g_sequence_get(iter) ;
        if (g_hash_table_contains(c->chunk_map, fp)) {
            return 1;
        }
    }
    return 0;
}


void insert_fab(struct container* con) {
    GHashTableIter iter ;
    g_hash_table_iter_init(&iter, con->meta.map);
    gpointer key, value;
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        // for each feature in container, update them in info chunk
        // if pri is 0 , means not find
        int pri = fp_in_info_chunk_cache((fingerprint*)key);
        if (pri) {
            // F-chunk
             struct chunk* new_ck = new_chunk(0);
             memcpy(new_ck->fp, key, destor.index_key_size);
             new_ck->id = con->meta.id;
             priority_cache_insert(law_area.f_cache, new_ck, pri, NULL, NULL);
        }else if (fp_in_faa_area((fingerprint*)key)) {
            // P chunk
            struct chunk* new_ck = new_chunk(0);
            memcpy(new_ck -> fp, key, destor.index_key_size);
            new_ck-> id = con->meta.id;
            lru_cache_insert(law_area.p_cache, new_ck, NULL, NULL) ;
        }
    }
}



static void init_law_are() {
    law_area.faa_area = g_sequence_new(free_fab_container);
    law_area.faa_area_size = 0 ;
    law_area.faa_area_max_size =  2;
    law_area.faa_area_full = 0;
    for (int i = 0 ; i< law_area.faa_area_max_size; i++ ) {
        struct fab_container* new_con = create_fab_container();
        g_sequence_append(law_area.faa_area, new_con);
    }

    law_area.informed_chunk_cache = g_sequence_new(free_chunk);
    law_area.informed_chunk_cache_size = 0;
    law_area.informed_chunk_cache_max_size = 1024;

    law_area.p_cache = new_lru_cache(1024, free_chunk, lookup_chunk_in_chunk_cache);
    law_area.f_cache = new_priority_cache(1024, free_cache_item, lookup_chunk_in_chunk_cache);
}

static GQueue* faa_area() {
    // Get FAA first container
    if (g_sequence_get_length(law_area.faa_area) == 0)
        return NULL;

    GQueue* q = g_queue_new();
    GSequenceIter* first_fab = g_sequence_get_begin_iter(law_area.faa_area);
    struct fab_container* first_con = g_sequence_get(first_fab);
    struct chunk* c = NULL;
    GSequenceIter* iter = g_sequence_get_begin_iter(first_con->chunk_lst);
    GSequenceIter* end = g_sequence_get_end_iter(first_con->chunk_lst) ;
    for (; iter != end; iter = g_sequence_iter_next(iter)) {
        c = g_sequence_get(iter);
        if (CHECK_CHUNK(c, CHUNK_FILE_START) || CHECK_CHUNK(c, CHUNK_FILE_END)) {
            g_queue_push_tail(q, c);
            c = NULL;
        }else if (CHECK_CHUNK(c, CHUNK_READY)) {
            g_queue_push_tail(q, c);
            continue;
        }else {
            //c needs to be restored
            //Find chunk cache
            containerid id = c->id;
            struct chunk* target_chunk = NULL;
            target_chunk = priority_cache_lookup(law_area.f_cache, &c->fp);
            if (target_chunk == NULL) {
                target_chunk = lru_cache_lookup(law_area.p_cache, &c->fp) ;
            }
            if (target_chunk == NULL) {
                // Not find in chunk cache
                // Try read a container
                struct container* con = NULL;
                jcr.read_container_num ++;
                VERBOSE("Restore cache: container %lld is missed", id);
                con = retrieve_container_by_id(id);

                // restore container data to existing all fab
                GSequenceIter* _iter =  g_sequence_get_begin_iter(law_area.faa_area);
                GSequenceIter* _end = g_sequence_get_end_iter(law_area.faa_area);
                for(; _iter != _end; _iter = g_sequence_iter_next(_iter)) {
                    update_chunk_in_fab_container_with_con(g_sequence_get(_iter), con);
                }

                // Insert chunk to chunk cache
                insert_fab(con);

            }else {
                // find data in chunk cache
                // copy to all fab

                GSequenceIter* _iter = g_sequence_get_begin_iter(law_area.faa_area);
                GSequenceIter* _end = g_sequence_get_end_iter(law_area.faa_area);
                for (; _iter != _end; _iter = g_sequence_iter_next(_iter)) {
                    update_chunk_in_fab_container(g_sequence_get(_iter), &target_chunk->fp);
                }

                // TODO adjust target_chunk priority

            }
            assert(CHECK_CHUNK(c, CHUNK_READY));
            g_queue_push_tail(q, c);
        }
    }

    // remove first fab
    g_sequence_remove(first_fab);

    // add a new fab from info chunk cache
    GSequenceIter* iter_chunk = g_sequence_get_begin_iter(law_area.informed_chunk_cache);
    GSequenceIter* end_chunk = g_sequence_get_end_iter(law_area.informed_chunk_cache);
    struct fab_container* new_con = create_fab_container();
    struct chunk* ck;
    for (; iter_chunk != end_chunk; iter_chunk = g_sequence_get_begin_iter(law_area.informed_chunk_cache)) {
        ck = g_sequence_get(iter_chunk) ;
        if (!fab_container_overflow(new_con, ck->size)) {
            add_chunk_to_fab_container(new_con, ck);
            g_sequence_remove(iter_chunk) ;
        }else {
            g_sequence_append(law_area.faa_area, new_con);
        }
    }

    return q;
}

static int law_area_push(struct chunk* c) {
    if (c == NULL)
        return 1;

    // Try to add chunk to faa area
    if (!law_area.faa_area_full) {
        GSequenceIter* iter = g_sequence_get_begin_iter(law_area.faa_area);
        GSequenceIter* end = g_sequence_get_end_iter(law_area.faa_area);
        struct fab_container* now_con;
        for (; iter != end; iter = g_sequence_iter_next(iter)) {
            now_con = g_sequence_get(iter);
            if (!fab_container_overflow(now_con, c->size)) {
                add_chunk_to_fab_container(now_con, c);
                // add chunk successfully return 0
                return 0;
            }
        }
        if (iter == end) {
            // FAA area is full
            law_area.faa_area_full = 1;
        }
    }

    // Try to add chunk to info chunk

    if (law_area.informed_chunk_cache_size < law_area.informed_chunk_cache_max_size) {
        g_sequence_append(law_area.informed_chunk_cache, c);
        law_area.informed_chunk_cache_size ++ ;
    }

    // info chunk is full
    if (law_area.informed_chunk_cache_size == law_area.informed_chunk_cache_max_size)
        return 1;

    return 0;

}

void* law_restore_thread(void* arg) {
    init_law_are();
    struct chunk* c;
    while ((c = sync_queue_pop(restore_recipe_queue))) {
        TIMER_DECLARE(1);
        TIMER_BEGIN(1);

        if (law_area_push(c)) {
            GQueue* q = faa_area();
            TIMER_END(1, jcr.read_chunk_time);
            struct chunk* rc ;
            while ((rc = g_queue_pop_head(q))) {
                if (CHECK_CHUNK(c, CHUNK_FILE_START)
                    || CHECK_CHUNK(c, CHUNK_FILE_END)) {
                    sync_queue_push(restore_chunk_queue, rc);
                    continue;
                }
                jcr.data_size += rc->size;
                jcr.chunk_num ++;
                if (destor.simulation_level >= SIMULATION_RESTORE) {
                    free_chunk(rc);
                } else {
                    sync_queue_push(restore_chunk_queue, rc);
                }
            }

            g_queue_free(q);
        }else {
            TIMER_END(1, jcr.read_chunk_time);
        }
    }

}
