#pragma once

#include "check_tamper.h"

#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <notification/notification_messages.h>

#include <gui/modules/submenu.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/popup.h>
#include <gui/modules/loading.h>
#include <gui/modules/text_input.h>
#include <gui/modules/byte_input.h>
#include <gui/modules/text_box.h>
#include <gui/modules/widget.h>
#include <gui/modules/variable_item_list.h>

#include <lib/nfc/nfc_types.h>
#include <lib/nfc/nfc_worker.h>
#include <lib/nfc/nfc_device.h>
#include <lib/nfc/parsers/nfc_supported_card.h>
#include <lib/nfc/helpers/nfc_generators.h>

#include "./scenes/check_tamper_scene.h"
#include "./helpers/nfc_custom_event.h"

#include <dialogs/dialogs.h>
#include <m-list.h>
#include <m-array.h>

#define check_tamper_TEXT_STORE_SIZE 128
#define check_tamper_APP_FOLDER ANY_PATH("nfc")

struct CheckTamper {
    NfcWorker* worker;
    ViewDispatcher* view_dispatcher;
    Gui* gui;
    NotificationApp* notifications;
    SceneManager* scene_manager;
    NfcDevice* dev;
    FuriHalNfcDevData dev_edit_data;

    char text_store[check_tamper_TEXT_STORE_SIZE + 1];
    FuriString* text_box_store;
    uint8_t byte_input_store[6];

    // Common Views
    Submenu* submenu;
    DialogEx* dialog_ex;
    Popup* popup;
    Loading* loading;
    TextInput* text_input;
    ByteInput* byte_input;
    TextBox* text_box;
    VariableItemList* variable_item_list;
    Widget* widget;

    const NfcGenerator* generator;
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

CheckTamper* check_tamper_alloc();

int32_t check_tamper_task(void* p);

void check_tamper_text_store_set(CheckTamper* check_tamper, const char* text, ...);

void check_tamper_text_store_clear(CheckTamper* check_tamper);

void check_tamper_blink_read_start(CheckTamper* check_tamper);

void check_tamper_blink_emulate_start(CheckTamper* check_tamper);

void check_tamper_blink_detect_start(CheckTamper* check_tamper);

void check_tamper_blink_stop(CheckTamper* check_tamper);

bool check_tamper_save_file(CheckTamper* check_tamper);

void check_tamper_show_loading_popup(void* context, bool show);
