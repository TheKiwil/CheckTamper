#include "../check_tamper_i.h"
#include <dolphin/dolphin.h>

void check_tamper_byte_input_callback(void* context) {
    CheckTamper* check_tamper = context;
    ST25Data* data = check_tamper->st25_data;

    memcpy(data->pwd_config, check_tamper->byte_input_store, 4);

    FURI_LOG_T(
        TAG,
        "new password : %02X-%02X-%02X-%02X",
        data->pwd_config[0],
        data->pwd_config[1],
        data->pwd_config[2],
        data->pwd_config[3]);

    view_dispatcher_send_custom_event(
        check_tamper->view_dispatcher, CheckTamperCustomEventByteInputDone);
}

void check_tamper_scene_password_on_enter(void* context) {
    CheckTamper* check_tamper = context;

    // Setup view
    ByteInput* byte_input = check_tamper->byte_input;
    byte_input_set_header_text(byte_input, "Enter The Password In Hex");
    byte_input_set_result_callback(
        byte_input,
        check_tamper_byte_input_callback,
        NULL,
        check_tamper,
        check_tamper->byte_input_store,
        4);
    view_dispatcher_switch_to_view(check_tamper->view_dispatcher, CheckTamperViewByteInput);
}

bool check_tamper_scene_password_on_event(void* context, SceneManagerEvent event) {
    CheckTamper* check_tamper = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == CheckTamperCustomEventByteInputDone) {
            consumed = scene_manager_search_and_switch_to_previous_scene(
                check_tamper->scene_manager, check_tamperSceneStart);
        }
    }
    return consumed;
}

void check_tamper_scene_password_on_exit(void* context) {
    CheckTamper* check_tamper = context;

    // Clear view
    byte_input_set_result_callback(check_tamper->byte_input, NULL, NULL, NULL, NULL, 0);
    byte_input_set_header_text(check_tamper->byte_input, "");
}
