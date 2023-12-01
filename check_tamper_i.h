#pragma once

#include "check_tamper.h"

#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <notification/notification_messages.h>
#include <dialogs/dialogs.h>

#include <gui/modules/submenu.h>
#include <gui/modules/popup.h>
#include <gui/modules/byte_input.h>

#include "lib/ST25/st25_worker.h"
#include "lib/ST25/st25.h"

#include "./scenes/check_tamper_scene.h"

#include <m-list.h>
#include <m-array.h>

#define check_tamper_TEXT_STORE_SIZE 128
#define check_tamper_APP_FOLDER ANY_PATH("nfc")

struct CheckTamper {
    ST25Worker* worker;
    ViewDispatcher* view_dispatcher;
    Gui* gui;
    NotificationApp* notifications;
    SceneManager* scene_manager;
    ST25Data* st25_data;

    char text_store[check_tamper_TEXT_STORE_SIZE + 1];
    FuriString* text_box_store;
    uint8_t byte_input_store[6];

    // Common Views
    Submenu* submenu;
    Popup* popup;
    ByteInput* byte_input;
};

typedef enum {
    CheckTamperViewMenu,
    CheckTamperViewDialogEx,
    CheckTamperViewPopup,
    CheckTamperViewLoading,
    CheckTamperViewTextInput,
    CheckTamperViewByteInput,
    CheckTamperViewTextBox,
    CheckTamperViewVarItemList,
    CheckTamperViewWidget,
    CheckTamperViewDictAttack,
    CheckTamperViewDetectReader,
} CheckTamperView;

typedef enum { CheckTamperCustomEventByteInputDone } CheckTamperCustomEvent;

CheckTamper* check_tamper_alloc();

int32_t check_tamper_task(void* p);

void check_tamper_blink_read_start(CheckTamper* check_tamper);

void check_tamper_blink_stop(CheckTamper* check_tamper);

void check_tamper_show_loading_popup(void* context, bool show);
