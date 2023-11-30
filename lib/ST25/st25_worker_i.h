#pragma once

#include "st25_worker.h"
#include "st25.h"

#include <furi.h>

struct ST25Worker {
    FuriThread* thread;

    ST25Data* st25_data;

    ST25WorkerCallback callback;
    void* context;
    void* event_data;

    ST25WorkerState state;
};

void st25_worker_change_state(ST25Worker* st25_worker, ST25WorkerState state);

int32_t st25_worker_task(void* context);

void st25_worker_read(ST25Worker* st25_worker);

void st25_worker_read_type(ST25Worker* st25_worker);

void st25_worker_check_tamper(ST25Worker* st25_worker);