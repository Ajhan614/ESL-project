/**
 * Copyright 2022 Evgeniy Morozov
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE
*/

#ifndef ESTC_SERVICE_H__
#define ESTC_SERVICE_H__

#include <stdint.h>

#include "ble.h"
#include "sdk_errors.h"

// TODO: 1. Generate random BLE UUID (Version 4 UUID) and define it in the following format:
//UUID : 755071f4-5460-4c14-82fe-77851cdbc397
#define ESTC_BASE_UUID { 0x97, 0xc3, 0xdb, 0x1c, 0x85, 0x77, /* - */ 0xfe, 0x82, /* - */ 0x14, 0x4c, /* - */ 0x60, 0x54, /* - */ 0x00, 0x00, 0x50, 0x75 }

// TODO: 2. Pick a random service 16-bit UUID and define it:

#define ESTC_SERVICE_UUID 0xAEE1
// TODO: 3. Pick a characteristic UUID and define it:
#define ESTC_GATT_CHAR_1_UUID 0xBEE1
#define ESTC_GATT_CHAR_NOTIFY_UUID 0xCEE1
#define ESTC_GATT_CHAR_INDICATE_UUID 0xDEE1

typedef struct
{
    uint16_t service_handle;
    uint16_t connection_handle;
    uint8_t  uuid_type;
    ble_gatts_char_handles_t char_notify_handles;
    ble_gatts_char_handles_t char_indicate_handles;
    ble_gatts_char_handles_t char_handles;
} ble_estc_service_t;

ret_code_t estc_ble_service_init(ble_estc_service_t *service);

void estc_ble_service_on_ble_event(const ble_evt_t *ble_evt, void *ctx);

void estc_update_notify_characteristic(ble_estc_service_t *service, uint8_t *value);
void estc_update_indicate_characteristic(ble_estc_service_t *service, uint8_t *value);

#endif /* ESTC_SERVICE_H__ */