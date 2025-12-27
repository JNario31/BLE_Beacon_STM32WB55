#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <zephyr/types.h>

int ble_service_init(void);
int ble_service_update_temperature(float temp_celsius);
int ble_service_update_pressure(uint32_t pressure_pa);
bool ble_service_is_temp_notify_enabled(void);
bool ble_service_is_pressure_notify_enabled(void);

#endif // BLE_SERVICE_H