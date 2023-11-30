#include "st25_worker_i.h"
#include <furi_hal.h>

#include <platform.h>
//#include <lib/nfc/parsers/nfc_supported_card.h>
//#include "reader_analyzer.h"

#define TAG "ST25Worker"

/***************************** NFC Worker API *******************************/

ST25Worker* st25_worker_alloc() {
    ST25Worker* st25_worker = malloc(sizeof(ST25Worker));

    // Worker thread attributes
    st25_worker->thread = furi_thread_alloc_ex("ST25Worker", 8192, st25_worker_task, st25_worker);

    st25_worker->callback = NULL;
    st25_worker->context = NULL;
    st25_worker->event_data = NULL;
    //st25_worker->storage = furi_record_open(RECORD_STORAGE);

    // Initialize rfal
    while(furi_hal_nfc_is_busy()) {
        furi_delay_ms(10);
    }
    st25_worker_change_state(st25_worker, ST25WorkerStateReady);

    //st25_worker->reader_analyzer = reader_analyzer_alloc(st25_worker->storage);

    return st25_worker;
}

void st25_worker_free(ST25Worker* st25_worker) {
    furi_assert(st25_worker);

    furi_thread_free(st25_worker->thread);

    free(st25_worker);
}

ST25WorkerState st25_worker_get_state(ST25Worker* st25_worker) {
    return st25_worker->state;
}

void* st25_worker_get_event_data(ST25Worker* st25_worker) {
    return st25_worker->event_data;
}

void st25_worker_start(
    ST25Worker* st25_worker,
    ST25WorkerState state,
    ST25Data* st25_data,
    ST25WorkerCallback callback,
    void* context) {
    furi_assert(st25_worker);
    furi_assert(st25_data);
    while(furi_hal_nfc_is_busy()) {
        furi_delay_ms(10);
    }
    furi_hal_nfc_deinit();
    furi_hal_nfc_init();

    st25_worker->callback = callback;
    st25_worker->context = context;
    st25_worker->st25_data = st25_data;
    st25_worker_change_state(st25_worker, state);
    furi_thread_start(st25_worker->thread);
}

void st25_worker_stop(ST25Worker* st25_worker) {
    furi_assert(st25_worker);
    furi_assert(st25_worker->thread);
    if(furi_thread_get_state(st25_worker->thread) != FuriThreadStateStopped) {
        furi_hal_nfc_stop();
        st25_worker_change_state(st25_worker, ST25WorkerStateStop);
        furi_thread_join(st25_worker->thread);
    }
}

void st25_worker_change_state(ST25Worker* st25_worker, ST25WorkerState state) {
    st25_worker->state = state;
}

/***************************** NFC Worker Thread *******************************/

int32_t st25_worker_task(void* context) {
    ST25Worker* st25_worker = context;

    furi_hal_nfc_exit_sleep();

    if(st25_worker->state == ST25WorkerStateRead) {
        st25_worker_read(st25_worker);
    } else if(st25_worker->state == ST25WorkerStateCheckTamper) {
        st25_worker_check_tamper(st25_worker);
    }
    furi_hal_nfc_sleep();
    st25_worker_change_state(st25_worker, ST25WorkerStateReady);

    return 0;
}

static bool st25_worker_read_nfcv(ST25Worker* st25_worker, FuriHalNfcTxRxContext* tx_rx) {
    bool read_success = false;
    FuriHalNfcDevData* nfc_data = &st25_worker->st25_data->nfc_data;
    ST25Data* st25_data = st25_worker->st25_data;

    furi_hal_nfc_sleep();

    do {
        if(!furi_hal_nfc_detect(nfc_data, 200)) break;
        if(!st25_read_card(nfc_data, st25_data)) break;

        read_success = true;
    } while(false);

    return read_success;
}

void st25_worker_read(ST25Worker* st25_worker) {
    furi_assert(st25_worker);
    furi_assert(st25_worker->callback);

    FuriHalNfcDevData* nfc_data = &st25_worker->st25_data->nfc_data;
    FuriHalNfcTxRxContext tx_rx = {};
    ST25WorkerEvent event = 0;
    bool card_not_detected_notified = false;

    while(st25_worker->state == ST25WorkerStateRead) {
        if(furi_hal_nfc_detect(nfc_data, 300)) {
            // Process first found device
            st25_worker->callback(ST25WorkerEventCardDetected, st25_worker->context);
            card_not_detected_notified = false;
            if(nfc_data->type == FuriHalNfcTypeV) {
                FURI_LOG_I(TAG, "NfcV detected");
                //st25_worker->protocol = NfcDeviceProtocolNfcV;
                if(st25_worker_read_nfcv(st25_worker, &tx_rx)) {
                    FURI_LOG_I(TAG, "st25_worker_read_nfcv success");
                }
                event = ST25WorkerEventReadST25;
                break;
            } else {
                FURI_LOG_I(TAG, "Uncompatible tag");
                //st25_worker->protocol = NfcDeviceProtocolUnknown;
                event = ST25WorkerEventWrongCardDetected;
            }
        } else {
            if(!card_not_detected_notified) {
                st25_worker->callback(ST25WorkerEventNoCardDetected, st25_worker->context);
                card_not_detected_notified = true;
            }
        }
        furi_hal_nfc_sleep();
        furi_delay_ms(100);
    }

    // Notify caller and exit
    if(event > ST25WorkerEventReserved) {
        FURI_LOG_I(TAG, "st25_worker_read_nfcv success2");
        st25_worker->callback(event, st25_worker->context);
    }
}

static bool st25_worker_read_tamper(ST25Worker* st25_worker) {
    bool read_success = false;
    FuriHalNfcDevData* nfc_data = &st25_worker->st25_data->nfc_data;
    ST25Data* st25_data = st25_worker->st25_data;

    furi_hal_nfc_sleep();

    do {
        if(!furi_hal_nfc_detect(&st25_worker->st25_data->nfc_data, 200)) break;
        if(!st25_read_tamper(nfc_data, st25_data)) break;

        read_success = true;
    } while(false);

    return read_success;
}

void st25_worker_check_tamper(ST25Worker* st25_worker) {
    furi_assert(st25_worker);
    furi_assert(st25_worker->callback);

    FuriHalNfcDevData* nfc_data = &st25_worker->st25_data->nfc_data;
    ST25WorkerEvent event = 0;
    bool card_not_detected_notified = false;

    while(st25_worker->state == ST25WorkerStateCheckTamper) {
        if(furi_hal_nfc_detect(nfc_data, 300)) {
            // Process first found device
            st25_worker->callback(ST25WorkerEventCardDetected, st25_worker->context);
            card_not_detected_notified = false;
            if(nfc_data->type == FuriHalNfcTypeV) {
                FURI_LOG_I(TAG, "NfcV detected");
                if(st25_worker_read_tamper(st25_worker)) {
                    FURI_LOG_I(TAG, "st25_worker_check_tamper success");
                }

                event = ST25WorkerEventCheckTamperST25;
                break;
            } else {
                FURI_LOG_I(TAG, "Uncompatible tag");
                event = ST25WorkerEventWrongCardDetected;
            }
        } else {
            if(!card_not_detected_notified) {
                st25_worker->callback(ST25WorkerEventNoCardDetected, st25_worker->context);
                card_not_detected_notified = true;
            }
        }
        furi_hal_nfc_sleep();
        furi_delay_ms(100);
    }

    // Notify caller and exit
    if(event > ST25WorkerEventReserved) {
        st25_worker->callback(event, st25_worker->context);
    }
}