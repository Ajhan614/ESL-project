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

static ret_code_t ble_char_add_helper(ble_estc_service_t * service,
                                     uint16_t            char_uuid,
                                     uint16_t            len,
                                     uint8_t           * p_init_value,
                                     char              * char_name,
                                     ble_gatts_char_handles_t * p_handles)
{
    ble_uuid_t          uuid;
    ble_gatts_char_md_t char_md = {0};
    ble_gatts_attr_md_t cccd_md = {0};
    ble_gatts_attr_t    attr_char_value = {0};
    ble_gatts_attr_md_t attr_md = {0};

    uuid.uuid = char_uuid;
    uuid.type = service->uuid_type;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

    char_md.char_props.read     = 1;
    char_md.char_props.write    = 1;
    char_md.char_props.notify   = 1;
    char_md.p_cccd_md           = &cccd_md;

    char_md.p_char_user_desc    = (uint8_t *)char_name;
    char_md.char_user_desc_size  = strlen(char_name);
    char_md.char_user_desc_max_size = strlen(char_name);

    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    attr_char_value.p_uuid    = &uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.max_len   = len;
    attr_char_value.init_len  = len;
    attr_char_value.p_value   = p_init_value;

    return sd_ble_gatts_characteristic_add(service->service_handle,
                                           &char_md,
                                           &attr_char_value,
                                           p_handles);
}

static ret_code_t estc_ble_add_characteristics(ble_estc_service_t *service,rgb_color *p_init_data)
{
    ret_code_t error_code = NRF_SUCCESS;

    error_code = ble_char_add_helper(service, 
                                  ESTC_GATT_CHAR_STATE_UUID,
                                  sizeof(uint8_t), (uint8_t*)&p_init_data->state,
                                  "LED On/Off",
                                  &service->char_state_handles);
    APP_ERROR_CHECK(error_code);

    error_code = ble_char_add_helper(service, 
                                  ESTC_GATT_CHAR_R_UUID,
                                  sizeof(uint8_t), &p_init_data->r,
                                  "Red Channel",
                                  &service->char_r_handles);
    APP_ERROR_CHECK(error_code);

    error_code = ble_char_add_helper(service, 
                                  ESTC_GATT_CHAR_G_UUID,
                                  sizeof(uint8_t), &p_init_data->g,
                                  "Green Channel",
                                  &service->char_g_handles);
    APP_ERROR_CHECK(error_code);

    error_code = ble_char_add_helper(service, 
                                  ESTC_GATT_CHAR_B_UUID,
                                  sizeof(uint8_t), &p_init_data->b,
                                  "Blue Channel",
                                  &service->char_b_handles);
    APP_ERROR_CHECK(error_code);

    return NRF_SUCCESS;
}

ret_code_t estc_ble_service_init(ble_estc_service_t *service, rgb_color *p_init_data)
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
    return estc_ble_add_characteristics(service, p_init_data);
}
void estc_update_characteristic(ble_estc_service_t *service, uint8_t *value, ble_gatts_char_handles_t * p_handles){
    if (service->connection_handle != BLE_CONN_HANDLE_INVALID)
		{
			uint16_t               len = sizeof(uint8_t);
			ble_gatts_hvx_params_t hvx_params;
			memset(&hvx_params, 0, sizeof(hvx_params));

			hvx_params.handle = p_handles->value_handle;
			hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
			hvx_params.offset = 0;
			hvx_params.p_len  = &len;
			hvx_params.p_data = value; 

			sd_ble_gatts_hvx(service->connection_handle, &hvx_params);
		}
}