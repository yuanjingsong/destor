//
// Created by 袁靖松 on 2020/3/11.
//

#include "priority_cache.h"
#include "destor.h"

struct priorityCache* new_priority_cache(int size, void(* free_elem)(void*),
        int (* hit_elem)(void* elem, void* user_data)) {
    struct priorityCache* cache = (struct priorityCache* )malloc(sizeof (struct priorityCache));

    cache ->elem_queue = NULL;

    cache ->max_size = size;
    cache ->size = 0;
    cache ->hit_count = 0;
    cache ->miss_count = 0;

    cache ->free_elem = free_elem;
    cache ->hit_elem = hit_elem;

    return cache;
}

void free_priority_cache(struct priorityCache* c) {
    g_list_free_full(c->elem_queue, c->free_elem) ;
    free(c);
}

void free_cache_item(struct cache_item* c) {
    free_chunk(c->data) ;
    free(c);
}

void* priority_cache_lookup(struct priorityCache* cache, void* user_data) {
    GList* elem = g_list_first(cache->elem_queue)  ;
    while (elem) {
        if (cache->hit_elem(((struct cache_item*)(elem->data))->data, user_data)) {
            break;
        }
        elem = g_list_next(elem);
    }
    if (elem) {
        cache ->hit_count ++;
        return elem->data;
    } else {
        cache ->miss_count ++;
        return NULL;
    }
}

void priority_cache_insert(struct priorityCache* c, void* data, int priority,
        void (* func)(void*, void*), void* user_data) {

    void *victim = 0;
    if (c->max_size > 0 && c->size == c->max_size) {
        GList* last = g_list_last(c->elem_queue);
        c->elem_queue = g_list_remove_link(c->elem_queue, last);
        victim = last->data;
        g_list_free_1(last);
        c->size--;
    }

    struct cache_item* new_item = (struct cache_item*)malloc(sizeof(struct cache_item));
    new_item ->data = data;
    new_item -> priority = priority;
    GList* elem = g_list_first(c ->elem_queue);
    int pos = 0;
    while (elem) {
        if (((struct cache_item*)(elem->data))->priority > priority) {
            elem = g_list_next(elem);
            pos ++;
        }else {
            g_list_insert(c->elem_queue, new_item, pos);
        }
    }

    if (victim) {
        if (func) {
            void* victim_data = ((struct cache_item*)(victim))->data;
            func(victim_data, user_data);
        }
        c->free_elem(victim);
    }
}

