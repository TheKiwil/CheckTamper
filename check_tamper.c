#include "check_tamper_i.h"
#include <furi_hal_nfc.h>
#include <dolphin/dolphin.h>

bool check_tamper_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    CheckTamper* check_tamper = context;
    return scene_manager_handle_custom_event(check_tamper->scene_manager, event);
}

bool check_tamper_back_event_callback(void* context) {
    furi_assert(context);
    CheckTamper* check_tamper = context;
    return scene_manager_handle_back_event(check_tamper->scene_manager);
}

void check_tamper_blink_read_start(CheckTamper* check_tamper) {
    notification_message(check_tamper->notifications, &sequence_blink_start_cyan);
}

void check_tamper_blink_stop(CheckTamper* check_tamper) {
    notification_message(check_tamper->notifications, &sequence_blink_stop);
}

void check_tamper_text_store_set(CheckTamper* check_tamper, const char* text, ...) {
    va_list args;
    va_start(args, text);

    vsnprintf(check_tamper->text_store, sizeof(check_tamper->text_store), text, args);

    va_end(args);
}

void check_tamper_text_store_clear(CheckTamper* check_tamper) {
    memset(check_tamper->text_store, 0, sizeof(check_tamper->text_store));
}

void check_tamper_blink_emulate_start(CheckTamper* check_tamper) {
    notification_message(check_tamper->notifications, &sequence_blink_start_magenta);
}

void check_tamper_blink_detect_start(CheckTamper* check_tamper) {
    notification_message(check_tamper->notifications, &sequence_blink_start_yellow);
}

void check_tamper_show_loading_popup(void* context, bool show) {
    CheckTamper* check_tamper = context;
    TaskHandle_t timer_task = xTaskGetHandle(configTIMER_SERVICE_TASK_NAME);

    if(show) {
        // Raise timer priority so that animations can play
        vTaskPrioritySet(timer_task, configMAX_PRIORITIES - 1);
        view_dispatcher_switch_to_view(check_tamper->view_dispatcher, CheckTamperViewLoading);
    } else {
        // Restore default timer priority
        vTaskPrioritySet(timer_task, configTIMER_TASK_PRIORITY);
    }
}

CheckTamper* check_tamper_alloc() {
    CheckTamper* check_tamper = malloc(sizeof(CheckTamper));

    check_tamper->worker = st25_worker_alloc();
    check_tamper->view_dispatcher = view_dispatcher_alloc();
    check_tamper->scene_manager = scene_manager_alloc(&check_tamper_scene_handlers, check_tamper);
    view_dispatcher_enable_queue(check_tamper->view_dispatcher);
    view_dispatcher_set_event_callback_context(check_tamper->view_dispatcher, check_tamper);
    view_dispatcher_set_custom_event_callback(
        check_tamper->view_dispatcher, check_tamper_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        check_tamper->view_dispatcher, check_tamper_back_event_callback);

    // ST25 data alloc
    ST25Data* st25_data = malloc(sizeof(ST25Data));
    check_tamper->st25_data = st25_data;

    // Open GUI record
    check_tamper->gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(
        check_tamper->view_dispatcher, check_tamper->gui, ViewDispatcherTypeFullscreen);

    // Open Notification record
    check_tamper->notifications = furi_record_open(RECORD_NOTIFICATION);

    // Submenu
    check_tamper->submenu = submenu_alloc();
    view_dispatcher_add_view(
        check_tamper->view_dispatcher,
        CheckTamperViewMenu,
        submenu_get_view(check_tamper->submenu));

    // Popup
    check_tamper->popup = popup_alloc();
    view_dispatcher_add_view(
        check_tamper->view_dispatcher, CheckTamperViewPopup, popup_get_view(check_tamper->popup));

    // Byte input
    check_tamper->byte_input = byte_input_alloc();
    view_dispatcher_add_view(
        check_tamper->view_dispatcher,
        CheckTamperViewByteInput,
        byte_input_get_view(check_tamper->byte_input));

    return check_tamper;
}

void check_tamper_free(CheckTamper* check_tamper) {
    furi_assert(check_tamper);

    // ST25 free
    free(&check_tamper->st25_data);

    // Submenu
    view_dispatcher_remove_view(check_tamper->view_dispatcher, CheckTamperViewMenu);
    submenu_free(check_tamper->submenu);

    // Popup
    view_dispatcher_remove_view(check_tamper->view_dispatcher, CheckTamperViewPopup);
    popup_free(check_tamper->popup);

    // Byte input
    view_dispatcher_remove_view(check_tamper->view_dispatcher, CheckTamperViewByteInput);
    byte_input_free(check_tamper->byte_input);

    // Worker
    st25_worker_stop(check_tamper->worker);
    st25_worker_free(check_tamper->worker);

    // View Dispatcher
    view_dispatcher_free(check_tamper->view_dispatcher);

    // Scene Manager
    scene_manager_free(check_tamper->scene_manager);

    // GUI
    furi_record_close(RECORD_GUI);
    check_tamper->gui = NULL;

    // Notifications
    furi_record_close(RECORD_NOTIFICATION);
    check_tamper->notifications = NULL;

    free(check_tamper);
}

static bool nfc_is_hal_ready() {
    if(!furi_hal_nfc_is_init()) {
        // No connection to the chip, show an error screen
        DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
        DialogMessage* message = dialog_message_alloc();
        dialog_message_set_text(
            message,
            "Error!\nNFC chip failed to start\n\n\nSend a photo of this to:\nsupport@flipperzero.one",
            0,
            0,
            AlignLeft,
            AlignTop);
        dialog_message_show(dialogs, message);
        dialog_message_free(message);
        furi_record_close(RECORD_DIALOGS);
        return false;
    } else {
        return true;
    }
}

int32_t check_tamper_app(void* p) {
    UNUSED(p);
    if(!nfc_is_hal_ready()) return 0;

    CheckTamper* check_tamper = check_tamper_alloc();

    FURI_LOG_T(TAG, "Start app check tamper");

    scene_manager_next_scene(check_tamper->scene_manager, check_tamperSceneStart);
    view_dispatcher_run(check_tamper->view_dispatcher);

    check_tamper_free(check_tamper);

    return 0;
}
