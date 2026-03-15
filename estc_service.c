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

#include "estc_service.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;

	ble_uuid_t char_uuid;
    char_uuid.uuid = ESTC_GATT_CHAR_1_UUID;
    char_uuid.type = service->uuid_type;

    ble_gatts_char_md_t char_md = { 0 };
    char_md.char_props.read = 1;
	char_md.char_props.write = 1;

    ble_gatts_attr_md_t attr_md = { 0 };
    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    ble_gatts_attr_t attr_char_value = { 0 };
    attr_char_value.p_uuid = &char_uuid;
    attr_char_value.p_attr_md   = &attr_md;
    attr_char_value.max_len  = sizeof(uint8_t);
    attr_char_value.init_len = sizeof(uint8_t);
    uint8_t value            = 0x12;
	attr_char_value.p_value  = &value;

    error_code = sd_ble_gatts_characteristic_add(service->service_handle,
                                   &char_md,
                                   &attr_char_value,
                                   &service->char_handles);

    APP_ERROR_CHECK(error_code);

    ble_uuid_t char_notify_uuid;
    char_notify_uuid.uuid = ESTC_GATT_CHAR_NOTIFY_UUID;
    char_notify_uuid.type = service->uuid_type;

    ble_gatts_attr_md_t cccd_notify_md={0};
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_notify_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_notify_md.write_perm);
    cccd_notify_md.vloc = BLE_GATTS_VLOC_STACK;

    ble_gatts_char_md_t char_notify_md={0};
    char_notify_md.char_props.read   = 1;
    char_notify_md.char_props.notify = 1; 
    char_notify_md.p_cccd_md         = &cccd_notify_md;

    ble_gatts_attr_md_t attr_notify_md ={0};
    attr_notify_md.vloc = BLE_GATTS_VLOC_STACK;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_notify_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_notify_md.write_perm);

    ble_gatts_attr_t attr_notify_char_value={0};
    attr_notify_char_value.p_uuid    = &char_notify_uuid;
    attr_notify_char_value.p_attr_md = &attr_notify_md;
    attr_notify_char_value.max_len   = sizeof(uint8_t);
    attr_notify_char_value.init_len  = sizeof(uint8_t);
    uint8_t notify_init_value        = 0x00;
    attr_notify_char_value.p_value   = &notify_init_value;

    error_code = sd_ble_gatts_characteristic_add(service->service_handle,
                                                 &char_notify_md,
                                                 &attr_notify_char_value,
                                                 &service->char_notify_handles);
    APP_ERROR_CHECK(error_code);
    
    ble_uuid_t char_indicate_uuid;
    char_indicate_uuid.uuid = ESTC_GATT_CHAR_INDICATE_UUID;
    char_indicate_uuid.type = service->uuid_type;

    ble_gatts_attr_md_t cccd_indicate_md={0};
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_indicate_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_indicate_md.write_perm);
    cccd_indicate_md.vloc = BLE_GATTS_VLOC_STACK;

    ble_gatts_char_md_t char_indicate_md={0};
    char_indicate_md.char_props.read     = 1;
    char_indicate_md.char_props.indicate = 1; 
    char_indicate_md.p_cccd_md           = &cccd_indicate_md;

    ble_gatts_attr_md_t attr_indicate_md={0};
    attr_indicate_md.vloc = BLE_GATTS_VLOC_STACK;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_indicate_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_indicate_md.write_perm);

    ble_gatts_attr_t attr_indicate_char_value;
    memset(&attr_indicate_char_value, 0, sizeof(attr_indicate_char_value));
    attr_indicate_char_value.p_uuid    = &char_indicate_uuid;
    attr_indicate_char_value.p_attr_md = &attr_indicate_md;
    attr_indicate_char_value.max_len   = sizeof(uint8_t);
    attr_indicate_char_value.init_len  = sizeof(uint8_t);
    uint8_t indicate_init_value        = 0x00;
    attr_indicate_char_value.p_value   = &indicate_init_value;

    error_code = sd_ble_gatts_characteristic_add(service->service_handle,
                                                 &char_indicate_md,
                                                 &attr_indicate_char_value,
                                                 &service->char_indicate_handles);
    APP_ERROR_CHECK(error_code);

    return NRF_SUCCESS;
}

ret_code_t estc_ble_service_init(ble_estc_service_t *service)
{
    ret_code_t error_code = NRF_SUCCESS;

    ble_uuid_t service_uuid;

    // TODO: 3. Add service UUIDs to the BLE stack table using `sd_ble_uuid_vs_add`
    ble_uuid128_t base_uuid = {ESTC_BASE_UUID};

    error_code = sd_ble_uuid_vs_add(&base_uuid, &service->uuid_type);
    APP_ERROR_CHECK(error_code);

    service_uuid.uuid = ESTC_SERVICE_UUID; 
    service_uuid.type = service->uuid_type;

    // TODO: 4. Add service to the BLE stack using `sd_ble_gatts_service_add`
    error_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, 
                                          &service_uuid, 
                                          &service->service_handle);
    APP_ERROR_CHECK(error_code);

    //NRF_LOG_DEBUG("%s:%d | Service UUID: 0x%04x", __FUNCTION__, __LINE__, service_uuid.uuid);
    //NRF_LOG_DEBUG("%s:%d | Service UUID type: 0x%02x", __FUNCTION__, __LINE__, service_uuid.type);
    //NRF_LOG_DEBUG("%s:%d | Service handle: 0x%04x", __FUNCTION__, __LINE__, service->service_handle);
    return estc_ble_add_characteristics(service);
}
void estc_update_notify_characteristic(ble_estc_service_t *service, uint8_t *value){
    if (service->connection_handle != BLE_CONN_HANDLE_INVALID)
		{
			uint16_t               len = sizeof(uint8_t);
			ble_gatts_hvx_params_t hvx_params;
			memset(&hvx_params, 0, sizeof(hvx_params));

			hvx_params.handle = service->char_notify_handles.value_handle;
			hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
			hvx_params.offset = 0;
			hvx_params.p_len  = &len;
			hvx_params.p_data = value;  

			sd_ble_gatts_hvx(service->connection_handle, &hvx_params);
		}
}
void estc_update_indicate_characteristic(ble_estc_service_t *service, uint8_t *value){
    if (service->connection_handle != BLE_CONN_HANDLE_INVALID)
		{
			uint16_t               len = sizeof(uint8_t);
			ble_gatts_hvx_params_t hvx_params;
			memset(&hvx_params, 0, sizeof(hvx_params));

			hvx_params.handle = service->char_indicate_handles.value_handle;
			hvx_params.type   = BLE_GATT_HVX_INDICATION;
			hvx_params.offset = 0;
			hvx_params.p_len  = &len;
			hvx_params.p_data = value;  

			sd_ble_gatts_hvx(service->connection_handle, &hvx_params);
		}
}