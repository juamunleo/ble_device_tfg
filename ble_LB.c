/**
 * Copyright (c) 2013 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#include "sdk_common.h"
#if NRF_MODULE_ENABLED(BLE_LBS)
#include "ble_LB.h"
#include "ble_srv_common.h"
#include <string.h>
#include "ble_conn_state.h"
#include "boards.h"

#define LEDBUTTON_LED                   BSP_BOARD_LED_1

#define NRF_LOG_MODULE_NAME ble_LB
#if BLE_LB_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL       BLE_LB_CONFIG_LOG_LEVEL
#define NRF_LOG_INFO_COLOR  BLE_LB_CONFIG_INFO_COLOR
#define NRF_LOG_DEBUG_COLOR BLE_LB_CONFIG_DEBUG_COLOR
#else // BLE_LB_CONFIG_LOG_ENABLED
#define NRF_LOG_LEVEL       0
#endif // BLE_LB_CONFIG_LOG_ENABLED

#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_LB       LB Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_LB_t * p_LB, ble_evt_t const * p_ble_evt)
{
    p_LB->conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;
}

/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_poxy       LB Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_LB_t * p_LB, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_LB->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_LB       LB Service structure.
 * @param[in]   p_ble_evt			  Event received from the BLE stack.
 */
static void on_write(ble_LB_t * p_LB, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

	/** TODO On write events. Whenever the BLE Chars are updated this function is triggered.
	  * p_evt_write->handle: Contains the updated field info.
      * p_ble_evt->evt.gatts_evt.params.write.data[0]: Contains the updated data.
	*/

	if(p_evt_write->handle == p_LB->LED_handles.value_handle)
	{
                if(p_ble_evt->evt.gatts_evt.params.write.data[0] > 0){
                  bsp_board_led_on(LEDBUTTON_LED);
                }else{
                  bsp_board_led_off(LEDBUTTON_LED);
                }
	}
	else if(p_evt_write->handle == p_LB->Button_handles.value_handle)
	{

	}
	else if(p_evt_write->handle == p_LB->battery_level_handles.value_handle)
	{

	}
}

void ble_LB_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    ble_LB_t * p_LB = (ble_LB_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_LB, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_LB, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_LB, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}


uint32_t ble_LB_LED_update(ble_LB_t * p_LB, uint8_t LED)
{
    if (p_LB == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = sizeof(uint8_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = &LED;

    // Update database.
    err_code = sd_ble_gatts_value_set(p_LB->conn_handle,p_LB->LED_handles.value_handle,&gatts_value);
    if (err_code == NRF_SUCCESS)
    {
        memcpy(&p_LB->LED, &LED, sizeof(uint8_t));
    }
    else
    {
        return err_code;
    }

    return err_code;
}


uint32_t ble_LB_Button_update(ble_LB_t * p_LB, uint8_t Button)
{
    if (p_LB == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = sizeof(uint8_t);
    gatts_value.offset  = 0;
    gatts_value.p_value = &Button;

    // Update database.
    err_code = sd_ble_gatts_value_set(p_LB->conn_handle,p_LB->Button_handles.value_handle,&gatts_value);
    if (err_code == NRF_SUCCESS)
    {
        memcpy(&p_LB->Button, &Button, sizeof(uint8_t));
    }
    else
    {
        return err_code;
    }

    return err_code;
}

uint32_t ble_LB_button_notify(ble_LB_t * p_LB, uint8_t button_state)
{
    ble_gatts_hvx_params_t params;
    uint16_t len = sizeof(button_state);

    memset(&params, 0, sizeof(params));
    params.type   = BLE_GATT_HVX_NOTIFICATION;
    params.handle = p_LB->Button_handles.value_handle;
    params.p_data = &button_state;
    params.p_len  = &len;

    return sd_ble_gatts_hvx(p_LB->conn_handle, &params);
}

uint32_t ble_LB_battery_level_update(ble_LB_t * p_LB, uint8_t battery_level)
{
    if (p_LB == NULL)
    {
        return NRF_ERROR_NULL;
    }

    uint32_t err_code = NRF_SUCCESS;
    ble_gatts_value_t gatts_value;

    // Initialize value struct.
    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = sizeof(uint8_t);
    gatts_value.offset  = 0;
    //TODO Check that datatype is consistent.
    gatts_value.p_value = &battery_level;

    // Update database.
    err_code = sd_ble_gatts_value_set(p_LB->conn_handle,p_LB->battery_level_handles.value_handle,&gatts_value);
    if (err_code == NRF_SUCCESS)
    {
        // TODO Store value in the p_LB->battery_level (May use '=', maybe & or * depending on data).
        memcpy(&p_LB->battery_level, &battery_level, sizeof(uint8_t));
    }
    else
    {
        return err_code;
    }

    return err_code;
}



/**@brief Function for adding the LB BATTERY_LEVEL characteristic.
 *
 * @param[in]   p_LB        LB Service structure.
 * @param[in]   p_LB_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t battery_level_char_add(ble_LB_t * p_LB, const ble_LB_init_t * p_LB_init)
{
    ret_code_t             err_code;
    ble_add_char_params_t  add_char_params;
    ble_add_descr_params_t add_descr_params;
    uint8_t                default_battery_level;
    uint8_t                init_len;
    uint8_t                encoded_report_ref[BLE_SRV_ENCODED_REPORT_REF_LEN];

    // Add battery_level characteristic
    default_battery_level = p_LB_init->default_battery_level;

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = BLE_UUID_BATTERY_LEVEL;
    add_char_params.max_len           = sizeof(uint8_t);
    add_char_params.init_len          = sizeof(uint8_t);
    add_char_params.p_init_value      = &default_battery_level;

add_char_params.char_props.read = 1;	add_char_params.read_access = p_LB_init->battery_level_rd_sec;
add_char_params.char_props.write = 1;	add_char_params.cccd_write_access = p_LB_init->battery_level_cccd_wr_sec;
	add_char_params.write_access = p_LB_init->battery_level_wr_sec;
    err_code = characteristic_add(p_LB->service_handle,&add_char_params,&(p_LB->battery_level_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    return NRF_SUCCESS;
}


/**@brief Function for adding the LB LED characteristic.
 *
 * @param[in]   p_LB        LB Service structure.
 * @param[in]   p_LB_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t LED_char_add(ble_LB_t * p_LB, const ble_LB_init_t * p_LB_init)
{
    ret_code_t             err_code;
    ble_add_char_params_t  add_char_params;
    ble_add_descr_params_t add_descr_params;
    uint8_t                default_LED;
    uint8_t                init_len;
    uint8_t                encoded_report_ref[BLE_SRV_ENCODED_REPORT_REF_LEN];

    // Add LED characteristic
    default_LED = p_LB_init->default_LED;

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = BLE_UUID_LED;
    add_char_params.max_len           = sizeof(uint8_t);
    add_char_params.init_len          = sizeof(uint8_t);
    add_char_params.p_init_value      = &default_LED;

add_char_params.char_props.read = 1;	add_char_params.read_access = p_LB_init->LED_rd_sec;
add_char_params.char_props.write = 1;	add_char_params.cccd_write_access = p_LB_init->LED_cccd_wr_sec;
	add_char_params.write_access = p_LB_init->LED_wr_sec;
    err_code = characteristic_add(p_LB->service_handle,&add_char_params,&(p_LB->LED_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    return NRF_SUCCESS;
}

/**@brief Function for adding the LB BUTTON characteristic.
 *
 * @param[in]   p_LB        LB Service structure.
 * @param[in]   p_LB_init   Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
static ret_code_t Button_char_add(ble_LB_t * p_LB, const ble_LB_init_t * p_LB_init)
{
    ret_code_t             err_code;
    ble_add_char_params_t  add_char_params;
    ble_add_descr_params_t add_descr_params;
    uint8_t                  default_Button;
    uint8_t                init_len;
    uint8_t                encoded_report_ref[BLE_SRV_ENCODED_REPORT_REF_LEN];

    // Add Button characteristic
    default_Button = p_LB_init->default_Button;

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = BLE_UUID_BUTTON;
    add_char_params.max_len           = sizeof(uint8_t);
    add_char_params.init_len          = sizeof(uint8_t);
    add_char_params.p_init_value      = &default_Button;
    add_char_params.char_props.read = 1;
    add_char_params.char_props.notify = 1;	
    add_char_params.read_access = p_LB_init->Button_rd_sec;
    add_char_params.cccd_write_access = SEC_OPEN;
    
    err_code = characteristic_add(p_LB->service_handle,&add_char_params,&(p_LB->Button_handles));
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    return NRF_SUCCESS;
}

ret_code_t ble_LB_init(ble_LB_t * p_LB, const ble_LB_init_t * p_LB_init)
{
	if (p_LB == NULL || p_LB_init == NULL)
	{
		return NRF_ERROR_NULL;
	}
	ret_code_t err_code;
	ble_uuid_t ble_uuid;

	// Initialize service structure
	p_LB->evt_handler = p_LB_init->evt_handler;
	p_LB->LED = 0;
	p_LB->Button = 0;
	p_LB->battery_level = 0;

	// Add service
	BLE_UUID_BLE_ASSIGN(ble_uuid, 0xAF00);

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, & ble_uuid, & p_LB->service_handle);
	VERIFY_SUCCESS(err_code);

	err_code = LED_char_add(p_LB, p_LB_init);
	err_code = Button_char_add(p_LB, p_LB_init);
	err_code = battery_level_char_add(p_LB, p_LB_init);
	return err_code;
}


#endif // NRF_MODULE_ENABLED(BLE_LBS)
