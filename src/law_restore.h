//
// Created by 袁靖松 on 2020/3/13.
//

#ifndef DESTOR_LAW_RESTORE_H
#define DESTOR_LAW_RESTORE_H

#include "utils/sync_queue.h"
SyncQueue* restore_chunk_queue;
SyncQueue* restore_recipe_queue;

void* law_restore_thread(void* arg);

#endif //DESTOR_LAW_RESTORE_H
