#include "../check_tamper_i.h"
#include <dolphin/dolphin.h>
#include <check_tamper_icons.h>

typedef enum {
    check_tamperSceneMainStateIdle,
    check_tamperSceneMainStateDetecting,
    check_tamperSceneMainStateEndReading,
    check_tamperSceneMainStateStartReading,
    check_tamperSceneMainStateClose,
    check_tamperSceneMainStateNotSupportedCard,
    check_tamperSceneMainStateCheckTamper,
} check_tamperSceneMainState;

static bool check_tamper_scene_main_worker_callback(ST25WorkerEvent event, void* context) {
    CheckTamper* check_tamper = context;
    FURI_LOG_T(TAG, "event = %d", check_tamper->st25_data->tamper_state);

    FURI_LOG_E(TAG, "event");
    //if(event == check_tamperSceneMainStateEndReading) {
    //popup_set_text(check_tamper->popup, "End", 97, 24, AlignCenter, AlignTop);
    //memcpy(data->key_privacy, check_tamper->byte_input_store, 4);
    //} else {
    view_dispatcher_send_custom_event(check_tamper->view_dispatcher, event);
    //}
    return true;
}

void check_tamper_scene_main_popup_callback(void* context) {
    CheckTamper* check_tamper = context;
    view_dispatcher_send_custom_event(check_tamper->view_dispatcher, ST25WorkerEventExit);
}

void check_tamper_scene_main_set_state(CheckTamper* check_tamper, check_tamperSceneMainState state) {
    //FuriHalNfcDevData* nfc_data = check_tamper->st25_data->nfc_data;
    ST25Data* st25_data = check_tamper->st25_data;

    uint32_t curr_state =
        scene_manager_get_scene_state(check_tamper->scene_manager, check_tamperSceneMain);
    if(curr_state != state) {
        Popup* popup = check_tamper->popup;
        if(state == check_tamperSceneMainStateDetecting) {
            popup_reset(popup);
            popup_set_text(
                popup, "Put figurine on\nFlipper's back", 97, 24, AlignCenter, AlignTop);
            popup_set_icon(popup, 0, 8, &I_NFC_manual_60x50);
        } else if(state == check_tamperSceneMainStateStartReading) {
            popup_reset(popup);
            popup_set_header(popup, "Start\nreading", 94, 3, AlignCenter, AlignTop);
            notification_message(check_tamper->notifications, &sequence_single_vibro);

            //popup_set_icon(popup, 0, 6, &I_RFIDDolphinSuccess_108x57);
            popup_set_context(popup, check_tamper);
            popup_set_callback(popup, check_tamper_scene_main_popup_callback);
            popup_set_timeout(popup, 1500);

        } else if(state == check_tamperSceneMainStateEndReading) {
            popup_reset(popup);
            popup_set_header(popup, "End of\nreading", 94, 3, AlignCenter, AlignTop);
            notification_message(check_tamper->notifications, &sequence_single_vibro);
            popup_set_context(popup, check_tamper);
            popup_set_callback(popup, check_tamper_scene_main_popup_callback);
            popup_set_timeout(popup, 1500);

        } else if(state == check_tamperSceneMainStateCheckTamper) {
            popup_reset(popup);

            if(st25_data->tamper_state) {
                popup_set_header(popup, "Open", 94, 15, AlignCenter, AlignTop);
                popup_set_icon(popup, 0, 2, &I_Tamper_Open);
            } else {
                popup_set_header(popup, "Close", 100, 15, AlignCenter, AlignTop);
                popup_set_icon(popup, 0, 2, &I_Tamper_Close);
            }

            notification_message(check_tamper->notifications, &sequence_single_vibro);
            popup_set_context(popup, check_tamper);
            popup_set_callback(popup, check_tamper_scene_main_popup_callback);
            popup_set_timeout(popup, 1500);

            view_dispatcher_switch_to_view(check_tamper->view_dispatcher, CheckTamperViewPopup);
            dolphin_deed(DolphinDeedNfcReadSuccess);
        } else if(state == check_tamperSceneMainStateNotSupportedCard) {
            popup_reset(popup);
            popup_set_header(popup, "Wrong Type Of Card!", 64, 3, AlignCenter, AlignTop);
            popup_set_text(popup, "Wrong Type Of Card!", 4, 22, AlignLeft, AlignTop);
            popup_set_icon(popup, 73, 20, &I_DolphinCommon_56x48);
        }
        scene_manager_set_scene_state(check_tamper->scene_manager, check_tamperSceneMain, state);
    }
}

void check_tamper_scene_main_on_enter(void* context) {
    CheckTamper* check_tamper = context;

    // Setup view
    check_tamper_scene_main_set_state(check_tamper, check_tamperSceneMainStateDetecting);
    view_dispatcher_switch_to_view(check_tamper->view_dispatcher, CheckTamperViewPopup);

    // Start worker
    st25_worker_start(
        check_tamper->worker,
        ST25WorkerStateCheckTamper,
        check_tamper->st25_data,
        check_tamper_scene_main_worker_callback,
        check_tamper);

    check_tamper_blink_read_start(check_tamper);
}

bool check_tamper_scene_main_on_event(void* context, SceneManagerEvent event) {
    CheckTamper* check_tamper = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        FURI_LOG_T(TAG, "Event");
        if(event.event == ST25WorkerEventCardDetected) {
            check_tamper_scene_main_set_state(
                check_tamper, check_tamperSceneMainStateStartReading);
            consumed = true;
        } else if(event.event == ST25WorkerEventReadST25) {
            FURI_LOG_T(TAG, "E endReading");
            check_tamper_scene_main_set_state(check_tamper, check_tamperSceneMainStateEndReading);
            consumed = true;
        } else if(event.event == ST25WorkerEventCheckTamperST25) {
            check_tamper_scene_main_set_state(check_tamper, check_tamperSceneMainStateCheckTamper);
            consumed = true;
        } else if(event.event == ST25WorkerEventAborted) {
            check_tamper_scene_main_set_state(check_tamper, check_tamperSceneMainStateClose);
            consumed = true;
        } else if(event.event == ST25WorkerEventNoCardDetected) {
            check_tamper_scene_main_set_state(check_tamper, check_tamperSceneMainStateDetecting);
            consumed = true;
        } else if(event.event == ST25WorkerEventWrongCardDetected) {
            check_tamper_scene_main_set_state(
                check_tamper, check_tamperSceneMainStateNotSupportedCard);
        } else if(event.event == ST25WorkerEventExit) {
            consumed = scene_manager_search_and_switch_to_previous_scene(
                check_tamper->scene_manager, check_tamperSceneStart);
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            check_tamper->scene_manager, check_tamperSceneStart);
    }
    return consumed;
}

void check_tamper_scene_main_on_exit(void* context) {
    CheckTamper* check_tamper = context;

    // Stop worker
    st25_worker_stop(check_tamper->worker);

    // Clear view
    popup_reset(check_tamper->popup);
    check_tamper_blink_stop(check_tamper);
    scene_manager_set_scene_state(
        check_tamper->scene_manager, check_tamperSceneMain, check_tamperSceneMainStateIdle);
}
