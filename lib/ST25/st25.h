#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <furi_hal.h>
#include <furi_hal_nfc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ST25_UID_LENGTH 8
#define ST25_COMMAND_RETRIES 10

typedef enum {
    ST25TypePlain = 0,
    ST25Type20KC = 1,
    ST25Type512 = 2,
} ST25Subtype;

/* ST25 command codes */
typedef enum {
    /* mandatory command codes */
    ST25_CMD_PASSWORD = 0x01,
    ST25_CMD_TAMPER = 0x02,
} St25Commands;

/* ISO15693 Response error codes */
typedef enum {
    ST25_NOERROR = 0x00,
    ST25_ERROR_CMD_PASSWORD = 0x01, // Command not supported
    ST25_ERROR_CMD_TAMPER = 0x02,
} ST25VErrorcodes;

typedef enum {
    ST25TV02KC,
    ST25TV512,
} ST25Type;

typedef struct {
    FuriHalNfcDevData nfc_data;

    /* common ISO15693 fields, being specified in ISO15693-3 */
    uint8_t dsfid;
    uint8_t afi;
    uint8_t ic_ref;
    uint16_t block_num;
    uint8_t block_size;
    uint8_t data[512];
    uint8_t security_status[1 + 512];
    bool selected;
    bool quiet;

    bool modified;
    bool ready;
    bool echo_mode;
    bool tamper_state;

    ST25Subtype sub_type;
    uint8_t rand[2];
    uint8_t pwd_config[4];

    uint8_t* frame; /* [NFCV_FRAMESIZE_MAX] ISO15693-2 incoming raw data from air layer */
    uint8_t frame_length; /* ISO15693-2 length of incoming data */
    uint32_t eof_timestamp; /* ISO15693-2 EOF timestamp, read from DWT->CYCCNT */
} ST25Data;

bool st25_read_blocks(ST25Data* data);
bool st25_read_sysinfo(FuriHalNfcDevData* nfc_data, ST25Data* data);
bool st25_read_card(FuriHalNfcDevData* nfc_data, ST25Data* data);
bool st25_read_tamper(FuriHalNfcDevData* nfc_data, ST25Data* data);

#ifdef __cplusplus
}
#endif
