#include "../check_tamper_i.h"
#include <dolphin/dolphin.h>

typedef enum {
    check_tamperSceneMainStateIdle,
    check_tamperSceneMainStateDetecting,
    check_tamperSceneMainStateOpen,
    check_tamperSceneMainStateClose,
    check_tamperSceneMainStateNotSupportedCard,
} check_tamperSceneMainState;

static bool check_tamper_scene_main_worker_callback(NfcWorkerEvent event, void* context) {
    CheckTamper* check_tamper = context;
    NfcVSlixData* data = &check_tamper->dev->dev_data.nfcv_data.sub_data.slix;

    if(event == NfcWorkerEventNfcVPassKey) {
        memcpy(data->key_privacy, check_tamper->byte_input_store, 4);
    } else {
        view_dispatcher_send_custom_event(check_tamper->view_dispatcher, event);
    }
    return true;
}

void check_tamper_scene_main_popup_callback(void* context) {
    CheckTamper* check_tamper = context;
    view_dispatcher_send_custom_event(check_tamper->view_dispatcher, NfcCustomEventViewExit);
}

void check_tamper_scene_main_set_state(CheckTamper* check_tamper, check_tamperSceneMainState state) {
    FuriHalNfcDevData* nfc_data = &(check_tamper->dev->dev_data.nfc_data);
    NfcVData* nfcv_data = &(check_tamper->dev->dev_data.nfcv_data);

    uint32_t curr_state =
        scene_manager_get_scene_state(check_tamper->scene_manager, check_tamperSceneMain);
    if(curr_state != state) {
        Popup* popup = check_tamper->popup;
        if(state == check_tamperSceneMainStateDetecting) {
            popup_reset(popup);
            popup_set_text(
                popup, "Put figurine on\nFlipper's back", 97, 24, AlignCenter, AlignTop);
            //popup_set_icon(popup, 0, 8, &I_NFC_manual_60x50);
        } else if(state == check_tamperSceneMainStateOpen) {
            popup_reset(popup);

            if(nfc_worker_get_state(check_tamper->worker) == NfcWorkerStateNfcVUnlockAndSave) {
                snprintf(
                    check_tamper->dev->dev_name,
                    sizeof(check_tamper->dev->dev_name),
                    "SLIX_%02X%02X%02X%02X%02X%02X%02X%02X",
                    nfc_data->uid[0],
                    nfc_data->uid[1],
                    nfc_data->uid[2],
                    nfc_data->uid[3],
                    nfc_data->uid[4],
                    nfc_data->uid[5],
                    nfc_data->uid[6],
                    nfc_data->uid[7]);

                check_tamper->dev->format = NfcDeviceSaveFormatNfcV;

                /*
                if(nfc_save_file(check_tamper)) {
                    popup_set_header(popup, "Successfully\nsaved", 94, 3, AlignCenter, AlignTop);
                } else {
                    popup_set_header(
                        popup, "Unlocked but\nsave failed!", 94, 3, AlignCenter, AlignTop);
                }
                */
            } else {
                popup_set_header(popup, "Successfully\nunlocked", 94, 3, AlignCenter, AlignTop);
            }

            notification_message(check_tamper->notifications, &sequence_single_vibro);
            //notification_message(check_tamper->notifications, &sequence_success);

            //popup_set_icon(popup, 0, 6, &I_RFIDDolphinSuccess_108x57);
            popup_set_context(popup, check_tamper);
            popup_set_callback(popup, check_tamper_scene_main_popup_callback);
            popup_set_timeout(popup, 1500);

            view_dispatcher_switch_to_view(check_tamper->view_dispatcher, CheckTamperViewPopup);
            dolphin_deed(DolphinDeedNfcReadSuccess);

        } else if(state == check_tamperSceneMainStateClose) {
            popup_reset(popup);

            popup_set_header(popup, "Already\nUnlocked!", 94, 3, AlignCenter, AlignTop);
            //popup_set_icon(popup, 0, 6, &I_RFIDDolphinSuccess_108x57);
            popup_set_context(popup, check_tamper);
            popup_set_callback(popup, check_tamper_scene_main_popup_callback);
            popup_set_timeout(popup, 1500);

            view_dispatcher_switch_to_view(check_tamper->view_dispatcher, CheckTamperViewPopup);
        } else if(state == check_tamperSceneMainStateNotSupportedCard) {
            popup_reset(popup);
            popup_set_header(popup, "Wrong Type Of Card!", 64, 3, AlignCenter, AlignTop);
            popup_set_text(popup, nfcv_data->error, 4, 22, AlignLeft, AlignTop);
            //popup_set_icon(popup, 73, 20, &I_DolphinCommon_56x48);
        }
        scene_manager_set_scene_state(check_tamper->scene_manager, check_tamperSceneMain, state);
    }
}

void check_tamper_scene_main_on_enter(void* context) {
    CheckTamper* check_tamper = context;

    nfc_device_clear(check_tamper->dev);
    // Setup view
    check_tamper_scene_main_set_state(check_tamper, check_tamperSceneMainStateDetecting);
    view_dispatcher_switch_to_view(check_tamper->view_dispatcher, CheckTamperViewPopup);

    // Start worker
    nfc_worker_start(
        check_tamper->worker,
        NfcWorkerStateNfcVUnlockAndSave,
        &check_tamper->dev->dev_data,
        check_tamper_scene_main_worker_callback,
        check_tamper);

    check_tamper_blink_read_start(check_tamper);
}

bool check_tamper_scene_main_on_event(void* context, SceneManagerEvent event) {
    CheckTamper* check_tamper = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcWorkerEventCardDetected) {
            check_tamper_scene_main_set_state(check_tamper, check_tamperSceneMainStateOpen);
            consumed = true;
        } else if(event.event == NfcWorkerEventAborted) {
            check_tamper_scene_main_set_state(check_tamper, check_tamperSceneMainStateClose);
            consumed = true;
        } else if(event.event == NfcWorkerEventNoCardDetected) {
            check_tamper_scene_main_set_state(check_tamper, check_tamperSceneMainStateDetecting);
            consumed = true;
        } else if(event.event == NfcWorkerEventWrongCardDetected) {
            check_tamper_scene_main_set_state(
                check_tamper, check_tamperSceneMainStateNotSupportedCard);
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
    nfc_worker_stop(check_tamper->worker);
    // Clear view
    popup_reset(check_tamper->popup);
    check_tamper_blink_stop(check_tamper);
    scene_manager_set_scene_state(
        check_tamper->scene_manager, check_tamperSceneMain, check_tamperSceneMainStateIdle);
}
