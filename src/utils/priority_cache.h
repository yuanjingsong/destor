//
// Created by 袁靖松 on 2020/3/11.
//

#ifndef DESTOR_PRIORITY_CACHE_H
#define DESTOR_PRIORITY_CACHE_H

#include <glib.h>

struct priorityCache {
    GList* elem_queue;

    int max_size;
    int size;

    double hit_count;
    double miss_count;

    void (*free_elem) (void*);
    int (* hit_elem) (void* elem, void* user_data);
};

struct cache_item {
    void* data;
    int priority;
};

struct priorityCache* new_priority_cache(int size, void(* free_elem)(void*),
        int (*hit_elem)(void* elem, void* user_data));

void free_cache_item(struct cache_item*);

void free_priority_cache(struct priorityCache* );
void* priority_cache_lookup(struct priorityCache*, void* user_data);
void* priority_cache_lookup_without_update(struct priorityCache*, void* user_data);
void* priority_cache_hits(struct priorityCache* cache, void* user_data,
            int(* hit)(void* elem, void* user_data));
void priority_cache_kicks(struct priorityCache* , void* user_data,
            int (*func)(void* elem, void* user_data));
void priority_cache_insert(struct priorityCache*, void* data, int priority,
            void (* victim)(void*, void*), void* user_data);
int priority_cache_is_full(struct priorityCache*);

#endif //DESTOR_PRIORITY_CACHE_H
