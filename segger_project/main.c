/**
 * Copyright (c) 2015 - 2021, Nordic Semiconductor ASA
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
/**
 * @brief Blinky Sample Application main file.
 *
 * This file contains the source code for a sample server application using the LED Button service.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_power.h"
#include "app_error.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_conn_state.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "boards.h"
#include "app_timer.h"
#include "app_button.h"
#include "ble_LB.h"
#include "ble_dis.h"
#include "nrf_gpiote.h"
#include "nrf_ble_bms.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_log.h"
#include "nrf_delay.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "fds.h"
#include "bsp_btn_ble.h"
#include "nrf_drv_gpiote.h"

#define SYSTEMOFF_LED                   BSP_BOARD_LED_0
#define PCB_LED                         26                                      /**< LED to be toggled with the help of the LED Button Service. */
#define ADVERTISING_LED                 BSP_BOARD_LED_2                         /**< Is on when device is advertising. */
#define CONNECTED_LED                   BSP_BOARD_LED_3                         /**< Is on when device has connected. */

#define BUTTON                          30                                      /**< Button that will trigger the notification event with the LED Button Service */
#define BUTTON_DEL_BONDS                BSP_BUTTON_2                            /**< Button that will trigger the notification event with the LED Button Service */

#define DEVICE_NAME                     "Nordic_JM"                             /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME               "JM"

#define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define APP_ADV_FAST_INTERVAL           0x0028                                  //!< Fast advertising interval (in units of 0.625 ms. This value corresponds to 25 ms.).
#define APP_ADV_INTERVAL                64                                      /**< The advertising interval (in units of 0.625 ms; this value corresponds to 40 ms). */
#define APP_ADV_DURATION                1000                                    /**< The advertising time-out (in units of seconds). When set to 0, we will never time out. */


#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(200, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                   0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory time-out (4 seconds). */

#define SEC_PARAM_BOND                  1                                       //!< Perform bonding.
#define SEC_PARAM_MITM                  0                                       //!< Man In The Middle protection not required.
#define SEC_PARAM_LESC                  0                                       //!< LE Secure Connections not enabled.
#define SEC_PARAM_KEYPRESS              0                                       //!< Keypress notifications not enabled.
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                    //!< No I/O capabilities.
#define SEC_PARAM_OOB                   0                                       //!< Out Of Band data not available.
#define SEC_PARAM_MIN_KEY_SIZE          7                                       //!< Minimum encryption key size.
#define SEC_PARAM_MAX_KEY_SIZE          16                                      //!< Maximum encryption key size.

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(20000)                  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (15 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000)                   /**< Time between each call to sd_ble_gap_conn_param_update after the first call (5 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50)                     /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define MEM_BUFF_SIZE                   512
#define DEAD_BEEF                       0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define SYSTEM_FROM_STARTUP 0x00000000 //Primer encendido
#define SYSTEM_FROM_RESETBTN 0x00000001 //Reset por botón
#define SYSTEM_FROM_OFF 0x00010000 //Reset desde System OFF

#define SHOW_CONSOLE_OUTPUT //Activa la información del programa por la consola

#define USE_AUTHORIZATION_CODE 1

#ifdef USE_AUTHORIZATION_CODE
static uint8_t m_auth_code[] = {'A', 'B', 'C', 'D'}; //0x41, 0x42, 0x43, 0x44
static int m_auth_code_len = sizeof(m_auth_code);
#endif

bool advertising_active = false, device_connected = false;

BLE_ADVERTISING_DEF(m_advertising);                                             //!< Advertising module instance.
NRF_BLE_BMS_DEF(m_bms); 
BLE_LB_DEF(m_LB);
NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                         /**< Context for the Queued Write module.*/

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                        /**< Handle of the current connection. */
static uint8_t m_qwr_mem[MEM_BUFF_SIZE];                                        //!< Write buffer for the Queued Write module.
static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;                   /**< Advertising handle used to identify an advertising set. */
static uint8_t m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];                    /**< Buffer for storing an encoded advertising set. */
static uint8_t m_enc_scan_response_data[BLE_GAP_ADV_SET_DATA_SIZE_MAX];         /**< Buffer for storing an encoded scan data. */
static ble_conn_state_user_flag_id_t m_bms_bonds_to_delete;                     //!< Flags used to identify bonds that should be deleted.

uint32_t reset_reason;

static ble_uuid_t m_adv_uuids[] = /** < Universally unique service identifiers. */
{
	{BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE},
	{0xAF00, BLE_UUID_TYPE_BLE},
};

/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = m_enc_scan_response_data,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX

    }
};

//Declaración de las funciones
int main(void);
void dormir(void);
static void buttons_init(void);
static void button_event_handler(uint8_t, uint8_t);
void leds_init(void);
void assert_nrf_callback(uint16_t, const uint8_t *);
void timers_init(void);
static void gap_params_init(void);
static void gatt_init(void);
static void advertising_init(void);
static void nrf_qwr_error_handler(uint32_t);
void services_init(void);
static void on_conn_params_evt(ble_conn_params_evt_t *);
static void conn_params_error_handler(uint32_t);
static void conn_params_init(void);
static void advertising_start(void);
static void ble_evt_handler(ble_evt_t const *, void *);
static void ble_stack_init(void);
static void log_init(void);
static void power_management_init(void);
static void idle_state_handle(void);
void bms_evt_handler(nrf_ble_bms_t * p_ess, nrf_ble_bms_evt_t * p_evt);
static void bond_delete(uint16_t conn_handle, void * p_context);
static void delete_disconnected_bonds(void);
static void delete_bonds(void);
static void delete_requesting_bond(nrf_ble_bms_t const * p_bms);
static void delete_all_bonds(nrf_ble_bms_t const * p_bms);
static void delete_all_except_requesting_bond(nrf_ble_bms_t const * p_bms);
static void peer_manager_init(void);
static void pm_evt_handler(pm_evt_t const * p_evt);
uint16_t qwr_evt_handler(nrf_ble_qwr_t * p_qwr, nrf_ble_qwr_evt_t * p_evt);
static void on_adv_evt(ble_adv_evt_t ble_adv_evt);
static void ble_advertising_error_handler(uint32_t nrf_error);
static void gpiote_init(void);
//Fin de las declaraciones

/**@brief Función inicial (el programa comienza aquí)
 */
int main(void){
    reset_reason = NRF_POWER->RESETREAS; //Guardamos el estado del registro RESETREAS para saber el motivo del reset
    NRF_POWER->RESETREAS = 0xFFFFFFFF; //Reiniciamos el valor del registro RESETREAS
    //Inicialización de componentes
    log_init();
    buttons_init();
    leds_init();
    timers_init();
    power_management_init();
    ble_stack_init();
    gap_params_init();
    gatt_init();
    services_init();
    advertising_init();
    conn_params_init();
    peer_manager_init();
    gpiote_init();
    //Fin de inicialización

    //Una vez finalizada la inicialización de BLE, se inicia el advertising
    #ifdef SHOW_CONSOLE_OUTPUT
      NRF_LOG_INFO("BLE started.");
    #endif

    if(reset_reason == SYSTEM_FROM_OFF){
        #ifdef SHOW_CONSOLE_OUTPUT
          NRF_LOG_INFO("Reset desde System OFF, iniciando advertising");
        #endif
        advertising_start();
    }
    #ifdef SHOW_CONSOLE_OUTPUT
    else{
        NRF_LOG_INFO("Reset no es desde System OFF, no se inicia advertising");
    }
    #endif

    //Entra en el bucle principal
    for (;;){
        idle_state_handle(); //Se realizarán acciones dependiendo del estado del dispositivo.
    }
}

static void gpiote_init()
{
    //VCE: This block is a one time configuration
    ret_code_t err_code;
    if(!nrf_drv_gpiote_is_init())
    {
        err_code = nrf_drv_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }

    //VCE: The below block needs to be called for each pin
    nrf_drv_gpiote_in_config_t in_config_1;
    in_config_1.pull = NRF_GPIO_PIN_PULLUP; //User defined
    in_config_1.sense = GPIOTE_CONFIG_POLARITY_Toggle; //User defined
    in_config_1.hi_accuracy = true; //User defined
    in_config_1.is_watcher = false; //Don't change this
    in_config_1.skip_gpio_setup = false; //Don't change this

    //VCE: Configuring 
    //err_code = nrf_drv_gpiote_in_init(15, &in_config_1, delete_bonds);
    //APP_ERROR_CHECK(err_code);

    //nrf_drv_gpiote_in_event_enable(15, true);
}

/**@brief Función que inicializa los servicios BLE usados en el programa
 *
 * @details Se inicializa la información del dispositivo y los servicios para LED, botón y batería
 */
void services_init(void){
	ret_code_t         err_code;
        nrf_ble_bms_init_t   bms_init;
	nrf_ble_qwr_init_t qwr_init;
	ble_dis_init_t     dis_init;
	ble_LB_init_t     LB_init;

        // Initialize Queued Write Module
        memset(&qwr_init, 0, sizeof(qwr_init));
        qwr_init.mem_buffer.len   = MEM_BUFF_SIZE;
        qwr_init.mem_buffer.p_mem = m_qwr_mem;
        qwr_init.callback         = qwr_evt_handler;
        qwr_init.error_handler    = nrf_qwr_error_handler;
        err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
        APP_ERROR_CHECK(err_code);

	// Initialize Device Information Service.
	memset(&dis_init, 0, sizeof(dis_init));
	ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, (char *)MANUFACTURER_NAME);
	dis_init.dis_char_rd_sec = SEC_OPEN;
	err_code = ble_dis_init(&dis_init);
	APP_ERROR_CHECK(err_code);
       
        // Initialize Bond Management Service
        memset(&bms_init, 0, sizeof(bms_init));
        m_bms_bonds_to_delete        = ble_conn_state_user_flag_acquire();
        bms_init.evt_handler         = bms_evt_handler;
        bms_init.error_handler       = nrf_qwr_error_handler;
    #if USE_AUTHORIZATION_CODE
        bms_init.feature.delete_requesting_auth         = true;
        bms_init.feature.delete_all_auth                = true;
        bms_init.feature.delete_all_but_requesting_auth = true;
    #else
        bms_init.feature.delete_requesting              = true;
        bms_init.feature.delete_all                     = true;
        bms_init.feature.delete_all_but_requesting      = true;
    #endif
        bms_init.bms_feature_sec_req = SEC_JUST_WORKS;
        bms_init.bms_ctrlpt_sec_req  = SEC_JUST_WORKS;

        bms_init.p_qwr                                       = &m_qwr;
        bms_init.bond_callbacks.delete_requesting            = delete_requesting_bond;
        bms_init.bond_callbacks.delete_all                   = delete_all_bonds;
        bms_init.bond_callbacks.delete_all_except_requesting = delete_all_except_requesting_bond;

	memset( & LB_init, 0, sizeof(LB_init));
	LB_init.evt_handler = NULL;
	LB_init.default_LED = 0;
	LB_init.LED_rd_sec = SEC_OPEN;
	LB_init.LED_cccd_wr_sec = SEC_OPEN;
	LB_init.LED_wr_sec = SEC_OPEN;

	LB_init.default_Button = 0;
	LB_init.Button_rd_sec = SEC_OPEN;

	LB_init.default_battery_level = 0.0;
	LB_init.battery_level_rd_sec = SEC_OPEN;
	LB_init.battery_level_cccd_wr_sec = SEC_OPEN;
	LB_init.battery_level_wr_sec = SEC_OPEN;

	err_code = ble_LB_init( & m_LB, &LB_init);
	APP_ERROR_CHECK(err_code);
        err_code = nrf_ble_bms_init(&m_bms, &bms_init);
        APP_ERROR_CHECK(err_code);
}


/**@brief Función de inicialización de LEDS.
 */
void leds_init(void){
    //bsp_board_init(BSP_INIT_LEDS);
    nrf_gpio_cfg_output(PCB_LED);
}


/**@brief Función para inicializar el manejador del botón
 */
static void buttons_init(void){
    //bsp_board_init(BSP_INIT_BUTTONS);
    ret_code_t err_code;

    //The array must be static because a pointer to it will be saved in the button handler module.
    static app_button_cfg_t buttons[] =
    {
        {BUTTON, false, BUTTON_PULL, button_event_handler}
    };

    err_code = app_button_init(buttons, ARRAY_SIZE(buttons),
                               BUTTON_DETECTION_DELAY);

    //Hacemos que el botón despierte al dispositivo en System Off
    nrf_gpio_cfg_sense_input(BUTTON, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling events from the button handler module.
 *
 * @param[in] pin_no        The pin that the event applies to.
 * @param[in] button_action The button action (press/release).
 */
static void button_event_handler(uint8_t pin_no, uint8_t button_action){
    ret_code_t err_code;

    switch (pin_no)
    {
        case BUTTON:
            #ifdef SHOW_CONSOLE_OUTPUT
              NRF_LOG_INFO("Send button state change:%d", button_action);
            #endif
            err_code = ble_LB_button_notify(&m_LB, button_action);
            if (err_code != NRF_SUCCESS &&
                err_code != BLE_ERROR_INVALID_CONN_HANDLE &&
                err_code != NRF_ERROR_INVALID_STATE &&
                err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        default:
            APP_ERROR_HANDLER(pin_no);
            break;
    }
}

/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for deleting all bet requesting device bonds
*/
static void delete_all_except_requesting_bond(nrf_ble_bms_t const * p_bms)
{
    ret_code_t err_code;
    uint16_t conn_handle;

    NRF_LOG_INFO("Client requested that all bonds except current bond be deleted");

    pm_peer_id_t peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
    while (peer_id != PM_PEER_ID_INVALID)
    {
        err_code = pm_conn_handle_get(peer_id, &conn_handle);
        APP_ERROR_CHECK(err_code);

        /* Do nothing if this is our own bond. */
        if (conn_handle != p_bms->conn_handle)
        {
            bond_delete(conn_handle, NULL);
        }

        peer_id = pm_next_peer_id_get(peer_id);
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context){
    ble_LB_on_ble_evt(p_ble_evt, p_context);
    ret_code_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            device_connected = true;
            advertising_active = false;
            //bsp_board_led_off(ADVERTISING_LED);
            #ifdef SHOW_CONSOLE_OUTPUT
              NRF_LOG_INFO("Connected");
            #endif
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            err_code = ble_LB_button_notify(&m_LB, 1);
            if (err_code != NRF_SUCCESS && err_code != BLE_ERROR_INVALID_CONN_HANDLE && err_code != NRF_ERROR_INVALID_STATE && err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING) {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BLE_GAP_EVT_DISCONNECTED:

            //bsp_board_led_off(PCB_LED);
            //bsp_board_led_on(ADVERTISING_LED);
            #ifdef SHOW_CONSOLE_OUTPUT
              NRF_LOG_INFO("Disconnected");
            #endif
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            //err_code = app_button_disable();
            //APP_ERROR_CHECK(err_code);
            advertising_active = false;
            device_connected = false;
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            #ifdef SHOW_CONSOLE_OUTPUT
              NRF_LOG_DEBUG("PHY update request.");
            #endif
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            #ifdef SHOW_CONSOLE_OUTPUT
              NRF_LOG_DEBUG("GATT Client Timeout.");
            #endif
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            #ifdef SHOW_CONSOLE_OUTPUT
              NRF_LOG_DEBUG("GATT Server Timeout.");
            #endif
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}

//--------------------------Funciones necesarias del SDK para que BLE funcione--------------------------

/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num    Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name){
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
void timers_init(void)
{
    // Initialize timer module, making it use the scheduler
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void){
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void){
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void){
    ret_code_t    err_code;
    ble_advdata_t advdata;
    ble_advdata_t srdata;
    ble_advertising_init_t init;
    uint8_t        adv_flags;
    //ble_uuid_t adv_uuids[] = {{LBS_UUID_SERVICE, m_lbs.uuid_type}};

    memset(&init, 0, sizeof(init));
    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));


    advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance = true;
    advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;


    memset(&srdata, 0, sizeof(srdata));
    srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    srdata.uuids_complete.p_uuids  = m_adv_uuids;

    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

   /*err_code = ble_advdata_encode(&srdata, m_adv_data.scan_rsp_data.p_data, &m_adv_data.scan_rsp_data.len);
    APP_ERROR_CHECK(err_code);*/

    ble_gap_adv_params_t adv_params;

    // Set advertising parameters.
    memset(&adv_params, 0, sizeof(adv_params));

    adv_params.primary_phy     = BLE_GAP_PHY_1MBPS;
    adv_params.duration        = APP_ADV_DURATION;
    adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    adv_params.p_peer_addr     = NULL;
    adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
    adv_params.interval        = APP_ADV_INTERVAL;

    
    adv_flags                             = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    init.advdata.name_type                = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance       = true;
    init.advdata.flags                    = adv_flags;
    init.advdata.uuids_complete.uuid_cnt  = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.advdata.uuids_complete.p_uuids   = m_adv_uuids;

    init.config.ble_adv_whitelist_enabled = false;
    init.config.ble_adv_fast_enabled      = true;
    init.config.ble_adv_fast_interval     = APP_ADV_FAST_INTERVAL;
    init.config.ble_adv_fast_timeout      = APP_ADV_DURATION;
    init.config.ble_adv_slow_enabled      = false;

    init.evt_handler   = on_adv_evt;
    init.error_handler = ble_advertising_error_handler;
    /*
    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &adv_params);
    APP_ERROR_CHECK(err_code);
    */
    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);
    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error){
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module that
 *          are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply
 *       setting the disconnect_on_fail config parameter, but instead we use the event
 *       handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt){
    ret_code_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error){
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void){
    ret_code_t             err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
        uint32_t ret;

        ret = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
        APP_ERROR_CHECK(ret);
        //bsp_board_led_on(ADVERTISING_LED);
        advertising_active = true;
}

/**@brief Function for handling events from bond management service.
 */
void bms_evt_handler(nrf_ble_bms_t * p_ess, nrf_ble_bms_evt_t * p_evt)
{
    ret_code_t err_code;
    bool is_authorized = true;

    switch (p_evt->evt_type)
    {
        case NRF_BLE_BMS_EVT_AUTH:
            NRF_LOG_DEBUG("Authorization request.");
#if USE_AUTHORIZATION_CODE
            if ((p_evt->auth_code.len != m_auth_code_len) ||
                (memcmp(m_auth_code, p_evt->auth_code.code, m_auth_code_len) != 0))
            {
                is_authorized = false;
            }
#endif
            err_code = nrf_ble_bms_auth_response(&m_bms, is_authorized);
            APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for deleting a single bond if it does not belong to a connected peer.
 *
 * This will mark the bond for deferred deletion if the peer is connected.
 */
static void bond_delete(uint16_t conn_handle, void * p_context)
{
    UNUSED_PARAMETER(p_context);
    ret_code_t   err_code;
    pm_peer_id_t peer_id;

    if (ble_conn_state_status(conn_handle) == BLE_CONN_STATUS_CONNECTED)
    {
        ble_conn_state_user_flag_set(conn_handle, m_bms_bonds_to_delete, true);
    }
    else
    {
        NRF_LOG_DEBUG("Attempting to delete bond.");
        err_code = pm_peer_id_get(conn_handle, &peer_id);
        APP_ERROR_CHECK(err_code);
        if (peer_id != PM_PEER_ID_INVALID)
        {
            err_code = pm_peer_delete(peer_id);
            APP_ERROR_CHECK(err_code);
            ble_conn_state_user_flag_set(conn_handle, m_bms_bonds_to_delete, false);
        }
    }
}


/**@brief Function for performing deferred deletions.
*/
static void delete_disconnected_bonds(void)
{
    uint32_t n_calls = ble_conn_state_for_each_set_user_flag(m_bms_bonds_to_delete, bond_delete, NULL);
    UNUSED_RETURN_VALUE(n_calls);
}


/**@brief Function for marking the requester's bond for deletion.
*/
static void delete_requesting_bond(nrf_ble_bms_t const * p_bms)
{
    NRF_LOG_INFO("Client requested that bond to current device deleted");
    ble_conn_state_user_flag_set(p_bms->conn_handle, m_bms_bonds_to_delete, true);
}


/**@brief Function for deleting all bonds
*/
static void delete_all_bonds(nrf_ble_bms_t const * p_bms)
{
    ret_code_t err_code;
    uint16_t conn_handle;

    NRF_LOG_INFO("Client requested that all bonds be deleted");

    pm_peer_id_t peer_id = pm_next_peer_id_get(PM_PEER_ID_INVALID);
    while (peer_id != PM_PEER_ID_INVALID)
    {
        err_code = pm_conn_handle_get(peer_id, &conn_handle);
        APP_ERROR_CHECK(err_code);

        bond_delete(conn_handle, NULL);

        peer_id = pm_next_peer_id_get(peer_id);
    }
}



/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void){
    ret_code_t err_code;
    
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);
    
    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


static void log_init(void){
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void){
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void){
    if (NRF_LOG_PROCESS() == false && advertising_active == false && device_connected == false)
    {
        //nrf_pwr_mgmt_run();
        dormir();
    }
    if(advertising_active){
      nrf_gpio_pin_set(PCB_LED);
      nrf_delay_ms(100);
      nrf_gpio_pin_set(PCB_LED);
      nrf_delay_ms(100);
    }
}

/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    //pm_handler_disconnect_on_sec_failure(p_evt);
    //pm_handler_flash_clean(p_evt);
    //pm_handler_flash_clean_on_return();

    switch (p_evt->evt_id)
    {
        case PM_EVT_CONN_SEC_FAILED: //Si el bond ya existe, se elimina.
            delete_bonds();
            break;
        default:
            break;
    }
}

uint16_t qwr_evt_handler(nrf_ble_qwr_t * p_qwr, nrf_ble_qwr_evt_t * p_evt)
{
    return nrf_ble_bms_on_qwr_evt(&m_bms, p_qwr, p_evt);
}
/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast adverstising.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            //bsp_board_led_off(ADVERTISING_LED);
            advertising_active = false;
            break;
        default:
            break;
    }
}
    
/**@brief Function for handling advertising errors.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void ble_advertising_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

void dormir(void){
    NRF_LOG_INFO("DORMIR");
    //bsp_board_led_off(PCB_LED);
    nrf_power_system_off();
}


/**
 * @}
 */
