//
// Created by 袁靖松 on 2019-12-10.
//

#ifndef DESTOR_CTXTTABLE_H
#define DESTOR_CTXTTABLE_H

void lipa_index_lookup(struct segment *);

void init_ctxtTable();


struct ctxtTableItem *find_mini_Item(GList *);

struct ctxtTableItem *find_max_Item(GList *);

struct ctxtTableItem *(*choose_champion)(GList *);

struct ctxtTableItem *epsilon_greedy_policy(GList *);

struct ctxtTableItem* random_policy(GList*);

void fp_prefetch(GList *, struct ctxtTableItem *, char* feature);


#endif //DESTOR_CTXTTABLE_H
