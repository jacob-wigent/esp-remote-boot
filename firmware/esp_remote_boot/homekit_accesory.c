/*

HomeKit Accesory Setup

HAP Documentation:
https://forum.iobroker.net/assets/uploads/files/1634848447889-apple-spezifikation-homekit.pdf

*/

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#include "device_info.h"

// HomeKit Lock States (HAP sections 9.52, 9.56)
#define PC_ON 0
#define PC_OFF 1
#define PC_UNKNOWN 3

void homekit_accessory_identify(homekit_value_t _value) {
	printf("accessory identify\n");
}

// Lock Mechanism (HAP section 8.26)
// required: LOCK CURRENT STATE, LOCK TARGET STATE
// optional: NAME

// Lock Characteristics (HAP sections 9.52, 9.56)
// format: uint8_t 
// 0: ON
// 1: OFF
// 2: unused
// 3: UNKNOWN
homekit_characteristic_t cha_lock_current_state = HOMEKIT_CHARACTERISTIC_(LOCK_CURRENT_STATE, PC_OFF);
homekit_characteristic_t cha_lock_target_state = HOMEKIT_CHARACTERISTIC_(LOCK_TARGET_STATE, PC_OFF);

// Name Characteristic (HAP section 9.62)
// format: string, max length 64
homekit_characteristic_t cha_name = HOMEKIT_CHARACTERISTIC_(NAME, DEVICE_NAME);

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_door_lock, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, DEVICE_NAME),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, DEVICE_MANUFACTURER),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, SERIAL_NUMBER),
            HOMEKIT_CHARACTERISTIC(MODEL, DEVICE_MODEL),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, DEVICE_FIRMWARE_VERSION),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, homekit_accessory_identify),
            NULL
        }),
		HOMEKIT_SERVICE(LOCK_MECHANISM, .primary=true, .characteristics=(homekit_characteristic_t*[]){
			&cha_lock_current_state,
      &cha_lock_target_state,
      &cha_name,
			NULL
		}),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
		.accessories = accessories,
		.password = HOMEKIT_PASSWORD
};