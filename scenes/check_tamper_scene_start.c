#include "../check_tamper_i.h"
#include <lib/nfc/nfc_worker_i.h>
#include <dolphin/dolphin.h>

enum SubmenuIndex {
    SubmenuIndexRead,
    SubmenuIndexDetectReader,
    SubmenuIndexSaved,
    SubmenuIndexExtraAction,
    SubmenuIndexAddManually,
    SubmenuIndexDebug,
};

void check_tamper_scene_start_submenu_callback(void* context, uint32_t index) {
    CheckTamper* check_tamper = context;

    view_dispatcher_send_custom_event(check_tamper->view_dispatcher, index);
}

void check_tamper_scene_start_on_enter(void* context) {
    CheckTamper* check_tamper = context;
    Submenu* submenu = check_tamper->submenu;

    submenu_add_item(
        submenu, "Read", SubmenuIndexRead, check_tamper_scene_start_submenu_callback, check_tamper);

    nfc_device_clear(check_tamper->dev);
    view_dispatcher_switch_to_view(check_tamper->view_dispatcher, CheckTamperViewMenu);
}

bool check_tamper_scene_start_on_event(void* context, SceneManagerEvent event) {
    CheckTamper* check_tamper = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubmenuIndexRead) {
            FURI_LOG_E(TAG, "S1");
            scene_manager_set_scene_state(
                check_tamper->scene_manager, check_tamperSceneStart, SubmenuIndexRead);
            check_tamper->dev->dev_data.read_mode = NfcReadModeAuto;
            FURI_LOG_E(TAG, "S2");
            scene_manager_next_scene(check_tamper->scene_manager, check_tamperSceneMain);
            dolphin_deed(DolphinDeedNfcRead);
            FURI_LOG_E(TAG, "S3");
            consumed = true;
        }
    }

    return consumed;
}

void check_tamper_scene_start_on_exit(void* context) {
    CheckTamper* check_tamper = context;

    submenu_reset(check_tamper->submenu);
}
