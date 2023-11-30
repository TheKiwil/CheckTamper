#include <limits.h>
#include <furi.h>

#include "st25.h"

#define TAG "ST25"

/* macros to map "modulate field" flag to GPIO level */
#define GPIO_LEVEL_MODULATED NFCV_LOAD_MODULATION_POLARITY
#define GPIO_LEVEL_UNMODULATED (!GPIO_LEVEL_MODULATED)

/* timing macros */
#define DIGITAL_SIGNAL_UNIT_S (100000000000.0f)
#define DIGITAL_SIGNAL_UNIT_US (100000.0f)

bool st25_command(FuriHalNfcTxRxContext* tx_rx) {
    char cDebug[128] = {0};
    bool bRet = false;

    memset(tx_rx->rx_data, 0, sizeof(tx_rx->rx_data));

    // Debug transeive
    snprintf(cDebug, sizeof(cDebug) - strlen(cDebug), "Transeive : ");
    for(size_t i = 0; i < (tx_rx->tx_bits / 8); i++) {
        snprintf(
            cDebug + strlen(cDebug), sizeof(cDebug) - strlen(cDebug), "%02X-", tx_rx->tx_data[i]);
    }
    cDebug[strlen(cDebug) - 1] = 0x00;
    FURI_LOG_T(TAG, cDebug);

    tx_rx->tx_rx_type = FuriHalNfcTxRxTypeRxNoCrc;
    if(furi_hal_nfc_tx_rx(tx_rx, 50)) {
        // Debug receive
        memset(cDebug, 0, sizeof(cDebug));

        snprintf(cDebug, sizeof(cDebug) - strlen(cDebug), "Receive : ");
        for(size_t i = 0; i < (tx_rx->rx_bits / 8); i++) {
            snprintf(
                cDebug + strlen(cDebug),
                sizeof(cDebug) - strlen(cDebug),
                "%02X-",
                tx_rx->rx_data[i]);
        }
        cDebug[strlen(cDebug) - 1] = 0x00;
        FURI_LOG_T(TAG, cDebug);

        bRet = true;
    }

    return bRet;
}

bool st25_get_random(FuriHalNfcDevData* dev_data, ST25Data* st25_data) {
    uint8_t uid[ST25_UID_LENGTH] = {};
    FuriHalNfcTxRxContext cmd = {};
    bool ret = false;

    /* UID is stored reversed in requests */
    for(int pos = 0; pos < dev_data->uid_len; pos++) {
        uid[pos] = dev_data->uid[dev_data->uid_len - 1 - pos];
    }

    cmd.tx_data[0] = 0x22U;
    cmd.tx_data[1] = 0xB4U;
    cmd.tx_data[2] = 0x02U;

    memcpy(&(cmd.tx_data[3]), &uid[0], ST25_UID_LENGTH);

    cmd.tx_bits = (3 * 8) + (ST25_UID_LENGTH * 8);

    ret = st25_command(&cmd);

    if(ret) {
        if(cmd.rx_bits < 5 * 8) return false;

        if(st25_data != NULL) {
            st25_data->rand[0] = cmd.rx_data[1];
            st25_data->rand[1] = cmd.rx_data[2];
        }
    }

    return ret;
}

bool st25_present_password(FuriHalNfcDevData* dev_data, ST25Data* st25_data, uint8_t password_id) {
    uint8_t uid[ST25_UID_LENGTH] = {};
    FuriHalNfcTxRxContext cmd = {};
    bool ret = false;
    uint8_t cmd_set_pass[] = {
        password_id,
        st25_data->rand[0],
        st25_data->rand[1],
        st25_data->rand[0],
        st25_data->rand[1]};

    // XOR
    for(int pos = 0; pos < 4; pos++) {
        cmd_set_pass[1 + pos] ^= st25_data->pwd_config[3 - pos];
    }

    // UID is stored reversed in requests
    for(int pos = 0; pos < dev_data->uid_len; pos++) {
        uid[pos] = dev_data->uid[dev_data->uid_len - 1 - pos];
    }

    cmd.tx_data[0] = 0x22U;
    cmd.tx_data[1] = 0xB3U;
    cmd.tx_data[2] = 0x02U;

    memcpy(&(cmd.tx_data[3]), &uid[0], ST25_UID_LENGTH);
    memcpy(&(cmd.tx_data[11]), &cmd_set_pass[0], 1);
    memcpy(&(cmd.tx_data[12]), &cmd_set_pass[1], 4);

    cmd.tx_bits = (3 * 8) + (ST25_UID_LENGTH * 8) + (5 * 8);

    ret = st25_command(&cmd);

    if(ret) {
        if(cmd.rx_bits < 3 * 8) return false;
    }

    return ret;
}

bool st25_read_config(
    FuriHalNfcDevData* dev_data,
    ST25Data* st25_data,
    uint8_t* data,
    uint16_t data_bits) {
    uint8_t uid[ST25_UID_LENGTH] = {};
    FuriHalNfcTxRxContext cmd = {};
    bool ret = false;

    // UID is stored reversed in requests */
    for(int pos = 0; pos < dev_data->uid_len; pos++) {
        uid[pos] = dev_data->uid[dev_data->uid_len - 1 - pos];
    }
    cmd.tx_data[0] = 0x22U;
    cmd.tx_data[1] = 0xA0U;
    cmd.tx_data[2] = 0x02U;

    memcpy(&(cmd.tx_data[3]), &uid[0], ST25_UID_LENGTH);
    memcpy(&(cmd.tx_data[11]), &data[0], data_bits / 8);

    cmd.tx_bits = (3 * 8) + (ST25_UID_LENGTH * 8) + data_bits;

    ret = st25_command(&cmd);
    if(ret) {
        //FURI_LOG_D(TAG, "bits=%d, val=%04x", cmd.rx_bits, cmd.rx_data[0]);
        if(cmd.rx_bits < 4 * 8) return false;
        if(cmd.rx_data[0] != 0x00) return false;
        FURI_LOG_D(TAG, "OK command");
        memcpy(&st25_data->data, &cmd.rx_data[1], (cmd.rx_bits / 8) - 3);
    }

    return ret;
}
bool st25_read_blocks(ST25Data* st25_data) {
    FuriHalNfcTxRxContext cmd = {};

    for(size_t block = 0; block < st25_data->block_num; block++) {
        FURI_LOG_D(TAG, "Reading block %d/%d", block, (st25_data->block_num - 1));

        bool ret = false;
        for(int tries = 0; tries < ST25_COMMAND_RETRIES; tries++) {
            cmd.tx_data[0] = 0x02U;
            cmd.tx_data[1] = 0x20U;
            cmd.tx_data[2] = (uint8_t)block;
            cmd.tx_bits = 3 * 8;
            ret = st25_command(&cmd);

            if(ret) break;
        }
        if(!ret) {
            FURI_LOG_D(TAG, "failed to read");
            return ret;
        }
        memcpy(
            &(st25_data->data[block * st25_data->block_size]),
            &cmd.rx_data[1],
            st25_data->block_size);
        FURI_LOG_D(
            TAG,
            "  %02X %02X %02X %02X",
            st25_data->data[block * st25_data->block_size + 0],
            st25_data->data[block * st25_data->block_size + 1],
            st25_data->data[block * st25_data->block_size + 2],
            st25_data->data[block * st25_data->block_size + 3]);
    }

    return true;
}

bool st25_read_sysinfo(FuriHalNfcDevData* nfc_data, ST25Data* st25_data) {
    //uint8_t cmd.rx_data[32];
    bool ret = false;
    FuriHalNfcTxRxContext cmd = {};
    FURI_LOG_D(TAG, "Read SYSTEM INFORMATION...");

    for(int tries = 0; tries < ST25_COMMAND_RETRIES; tries++) {
        cmd.tx_data[0] = 0x02U;
        cmd.tx_data[1] = 0x2BU;
        cmd.tx_bits = 2 * 8;
        ret = st25_command(&cmd);
        if(ret) break;
    }

    if(ret) {
        nfc_data->type = FuriHalNfcTypeV;
        nfc_data->uid_len = ST25_UID_LENGTH;
        memset(nfc_data->uid, 0, sizeof(nfc_data->uid));

        /* UID is stored reversed in this response */
        for(int pos = 0; pos < nfc_data->uid_len; pos++) {
            nfc_data->uid[pos] = cmd.rx_data[2 + (ST25_UID_LENGTH - 1 - pos)];
        }

        st25_data->dsfid = cmd.rx_data[ST25_UID_LENGTH + 2];
        st25_data->afi = cmd.rx_data[ST25_UID_LENGTH + 3];
        st25_data->block_num = cmd.rx_data[ST25_UID_LENGTH + 4] + 1;
        st25_data->block_size = cmd.rx_data[ST25_UID_LENGTH + 5] + 1;
        st25_data->ic_ref = cmd.rx_data[ST25_UID_LENGTH + 6];
        FURI_LOG_D(
            TAG,
            "  UID:          %02X %02X %02X %02X %02X %02X %02X %02X",
            nfc_data->uid[0],
            nfc_data->uid[1],
            nfc_data->uid[2],
            nfc_data->uid[3],
            nfc_data->uid[4],
            nfc_data->uid[5],
            nfc_data->uid[6],
            nfc_data->uid[7]);
        FURI_LOG_D(
            TAG,
            "  DSFID %d, AFI %d, Blocks %d, Size %d, IC Ref %d",
            st25_data->dsfid,
            st25_data->afi,
            st25_data->block_num,
            st25_data->block_size,
            st25_data->ic_ref);
        return ret;
    }
    FURI_LOG_D(TAG, "Failed");

    return ret;
}

bool ST25TV02KC_check_card_type(FuriHalNfcDevData* nfc_data) {
    if((nfc_data->uid[0] == 0xE0) && (nfc_data->uid[1] == 0x02) && (nfc_data->uid[2] == 0x08)) {
        return true;
    }
    return false;
}

bool ST25TV512_check_card_type(FuriHalNfcDevData* nfc_data) {
    if((nfc_data->uid[0] == 0xE0) && (nfc_data->uid[1] == 0x02) && (nfc_data->uid[2] == 0x23)) {
        return true;
    }
    return false;
}

bool st25_read_card(FuriHalNfcDevData* nfc_data, ST25Data* st25_data) {
    furi_assert(nfc_data);
    furi_assert(st25_data);

    if(!st25_read_sysinfo(nfc_data, st25_data)) return false;

    if(!st25_read_blocks(st25_data)) return false;

    /* clear all know sub type data before reading them */
    //memset(&st25_data->sub_data, 0x00, sizeof(st25_data->sub_data));

    if(ST25TV02KC_check_card_type(nfc_data)) {
        FURI_LOG_I(TAG, "ST ST25TV02KC detected");
        st25_data->sub_type = ST25Type20KC;
    } else if(ST25TV512_check_card_type(nfc_data)) {
        FURI_LOG_I(TAG, "ST ST25TV512 detected");
        st25_data->sub_type = ST25Type512;
    } else {
        st25_data->sub_type = ST25TypePlain;
    }

    return true;
}

bool st25_read_tamper(FuriHalNfcDevData* nfc_data, ST25Data* st25_data) {
    furi_assert(nfc_data);
    furi_assert(st25_data);
    uint8_t data[12] = {0};
    uint8_t _uid[ST25_UID_LENGTH] = {0};

    FURI_LOG_I(TAG, "Read tamper");

    /* UID is stored reversed in this response */
    memcpy(_uid, nfc_data->uid, ST25_UID_LENGTH);
    for(int pos = 0; pos < nfc_data->uid_len; pos++) {
        nfc_data->uid[pos] = _uid[(ST25_UID_LENGTH - 1 - pos)];
    }

    if(ST25TV02KC_check_card_type(nfc_data)) {
        FURI_LOG_I(TAG, "ST ST25TV02KC detected");
        st25_data->sub_type = ST25Type20KC;
        data[0] = 0x03;
        data[1] = 0x06;
        if(!st25_get_random(nfc_data, st25_data)) return false;
        if(!st25_present_password(nfc_data, st25_data, 0)) return false;
        if(!st25_read_config(nfc_data, st25_data, data, 16)) return false;
        if(st25_data->data[2] == 0x63) {
            FURI_LOG_I(TAG, "ST ST25TV02KC tamper close");
            st25_data->tamper_state = false;
        } else if(st25_data->data[2] == 0x6F) {
            FURI_LOG_I(TAG, "ST ST25TV02KC tamper open");
            st25_data->tamper_state = true;
        }
    } else if(ST25TV512_check_card_type(nfc_data)) {
        FURI_LOG_I(TAG, "ST ST25TV512 detected");
        st25_data->sub_type = ST25Type512;
        data[0] = 0x05;

        if(!st25_read_config(nfc_data, st25_data, data, 8)) return false;
        if(st25_data->data[0] == 0x01) {
            FURI_LOG_I(TAG, "ST ST25TV512 tamper close");
            st25_data->tamper_state = false;
        } else if(st25_data->data[0] == 0x00) {
            FURI_LOG_I(TAG, "ST ST25TV512 tamper open");
            st25_data->tamper_state = true;
        }
    } else {
        st25_data->sub_type = ST25TypePlain;
        return false;
    }

    return true;
}