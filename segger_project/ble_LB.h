#ifndef BLE_LB_H__
#define BLE_LB_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"

/* Characteristics UUID Definitions. */
#define BLE_UUID_BUTTON		0xC001
#define BLE_UUID_BATTERY_LEVEL		0xC002

/* Enable LB Module */
#define BLE_LB_ENABLED 1
#define BLE_LB_BLE_OBSERVER_PRIO 2

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Macro for defining a ble_LB instance. 
* @param _name Name of the instance.
* @hideinitializer        */
#define BLE_LB_DEF(_name) 	static ble_LB_t _name; NRF_SDH_BLE_OBSERVER(_name  ## _obs,BLE_LB_BLE_OBSERVER_PRIO,ble_LB_on_ble_evt, &_name) 

/**@brief LB Service event type. */
typedef enum
{
	BLE_LB_EVT_NOTIFICATION_ENABLED, /**< LB value notification enabled event. */
	BLE_LB_EVT_NOTIFICATION_DISABLED /**< LB value notification disabled event. */
} ble_LB_evt_type_t;

/**@brief LB Service event. */
typedef struct
{
	ble_LB_evt_type_t evt_type;    /**< Type of event. */
	uint16_t           conn_handle; /**< Connection handle. */
} ble_LB_evt_t;

// Forward declaration of the ble_LB_t type.
typedef struct ble_LB_s ble_LB_t;

/**@brief LB Service event handler type. */
typedef void (* ble_LB_evt_handler_t) (ble_LB_t * p_LB, ble_LB_evt_t * p_evt);

/**@brief LB Service init structure. This contains all options and data needed for initialization of the service.*/
typedef struct
{
	ble_LB_evt_handler_t  evt_handler;                    /**< Event handler to be called for handling events in the LB Service. */
	uint8_t 	 default_Button;
	float 	 default_battery_level;
	security_req_t 	 Button_rd_sec;
        security_req_t 	 Button_not_sec;
	security_req_t 	 battery_level_rd_sec;
	security_req_t 	 battery_level_cccd_wr_sec;
	security_req_t 	 battery_level_wr_sec;
} ble_LB_init_t;

/**@brief LB Service structure. This contains various status information for the service. */
struct ble_LB_s
{
	ble_LB_evt_handler_t evt_handler;
	uint16_t service_handle;
	ble_gatts_char_handles_t Button_handles;
	ble_gatts_char_handles_t battery_level_handles;
	uint16_t conn_handle;
	uint8_t Button;
	float battery_level;
};

/**@brief Function for initializing the LB Service.
 *
 * @param[out]  p_LB       LB Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_LB_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.*/
ret_code_t ble_LB_init(ble_LB_t * p_LB, const ble_LB_init_t * p_LB_init);

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the LB Service.
 *
 * @note For the requirements in the LB specification to be fulfilled,
 *       ble_LB_update() must be called upon reconnection if the LB parameters
 *       have changed while the service has been disconnected from a bonded client.
 *
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 * @param[in]   p_context   LB Service structure.
 */
void ble_LB_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);
uint32_t ble_LB_button_notify(ble_LB_t * p_LB, uint8_t button_state);
uint32_t ble_LB_Button_update(ble_LB_t * p_LB, uint8_t Button);
uint32_t ble_LB_battery_level_update(ble_LB_t * p_LB, uint8_t battery_level);
#ifdef __cplusplus
}
#endif

#endif
