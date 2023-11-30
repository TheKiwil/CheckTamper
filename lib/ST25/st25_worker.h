#pragma once

#include "st25.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ST25Worker ST25Worker;

typedef enum {
    // Init states
    ST25WorkerStateNone,
    ST25WorkerStateReady,

    // Main worker states
    ST25WorkerStateRead,
    ST25WorkerStateCheckTamper,

    // Transition
    ST25WorkerStateStop,
} ST25WorkerState;

typedef enum {
    // Reserve first 50 events for application events
    ST25WorkerEventReserved = 50,

    // Nfc read events
    ST25WorkerEventReadUidST25,
    ST25WorkerEventReadST25,
    ST25WorkerEventCheckTamperST25,

    // Nfc worker common events
    ST25WorkerEventSuccess,
    ST25WorkerEventFail,
    ST25WorkerEventAborted,
    ST25WorkerEventCardDetected,
    ST25WorkerEventNoCardDetected,
    ST25WorkerEventWrongCardDetected,

    // Detect Reader events
    ST25WorkerEventDetectReaderDetected,
    ST25WorkerEventDetectReaderLost,
    ST25WorkerEventDetectReaderMfkeyCollected,

    ST25WorkerEventExit,
} ST25WorkerEvent;

typedef bool (*ST25WorkerCallback)(ST25WorkerEvent event, void* context);

ST25Worker* st25_worker_alloc();

ST25WorkerState st25_worker_get_state(ST25Worker* st25_worker);

void* st25_worker_get_event_data(ST25Worker* st25_worker);

void st25_worker_free(ST25Worker* st25_worker);

void st25_worker_start(
    ST25Worker* st25_worker,
    ST25WorkerState state,
    ST25Data* st25_data,
    ST25WorkerCallback callback,
    void* context);

void st25_worker_stop(ST25Worker* st25_worker);

#ifdef __cplusplus
}
#endif