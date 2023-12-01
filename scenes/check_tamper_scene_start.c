#include "../check_tamper_i.h"
//#include <lib/nfc/nfc_worker_i.h>
#include <dolphin/dolphin.h>

enum SubmenuIndex {
    SubmenuIndexCheckTamper,
    SubmenuIndexPassword,
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
        submenu,
        "Check tamper",
        SubmenuIndexCheckTamper,
        check_tamper_scene_start_submenu_callback,
        check_tamper);

    submenu_add_item(
        submenu,
        "Password (02KC)",
        SubmenuIndexPassword,
        check_tamper_scene_start_submenu_callback,
        check_tamper);

    //st25_device_clear(check_tamper->dev);
    view_dispatcher_switch_to_view(check_tamper->view_dispatcher, CheckTamperViewMenu);
}

bool check_tamper_scene_start_on_event(void* context, SceneManagerEvent event) {
    CheckTamper* check_tamper = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubmenuIndexCheckTamper) {
            scene_manager_set_scene_state(
                check_tamper->scene_manager, check_tamperSceneStart, SubmenuIndexCheckTamper);
            scene_manager_next_scene(check_tamper->scene_manager, check_tamperSceneMain);
            dolphin_deed(DolphinDeedNfcRead);
            consumed = true;
        } else if(event.event == SubmenuIndexPassword) {
            scene_manager_set_scene_state(
                check_tamper->scene_manager, check_tamperSceneStart, SubmenuIndexCheckTamper);
            scene_manager_next_scene(check_tamper->scene_manager, check_tamperScenePassword);
            consumed = true;
        }
    }

    return consumed;
}

void check_tamper_scene_start_on_exit(void* context) {
    CheckTamper* check_tamper = context;

    submenu_reset(check_tamper->submenu);
}
