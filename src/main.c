#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include "ble_service.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define SENSOR_UPDATE_INTERVAL_SEC 5

static const int32_t sleep_time_ms = 100;


int main(void)
{
    int ret;

    LOG_INF("Starting BLE Beacon Application");

    const struct device *bmp180_dev = DEVICE_DT_GET(DT_ALIAS(temp_sensor));
    struct sensor_value temp, press;

    if (!device_is_ready(bmp180_dev)) {
        LOG_ERR("BMP180 device not ready");
        return -ENODEV;
    }
    LOG_INF("BMP180 sensor initialized successfully");

    /* Initialize BLE Service */
    ret = ble_service_init();
    if (ret) {
        LOG_ERR("Failed to initialize BLE service (err %d)", ret);
        return ret;
    }
    LOG_INF("BLE service initialized");

    while(1){
        ret = sensor_sample_fetch(bmp180_dev);
        if (ret < 0) {
            LOG_ERR("Failed to fetch sample (%d)", ret);
            k_msleep(sleep_time_ms);
            continue;
        }

        /* Get temperature */
        ret = sensor_channel_get(bmp180_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        if (ret == 0) {
            // Convert to float: val1 + (val2 / 1,000,000)
            float temp_celsius = temp.val1 + (temp.val2 / 1000000.0f);
            
            LOG_INF("Temperature: %.2f C", temp_celsius);
            
            ret = ble_service_update_temperature(temp_celsius);
            if (ret < 0) {
                LOG_WRN("Failed to update BLE temperature");
            }
        } else {
            LOG_ERR("Failed to get temperature channel (%d)", ret);
        }

        /* Get pressure */
        ret = sensor_channel_get(bmp180_dev, SENSOR_CHAN_PRESS, &press);
        if (ret == 0) {
            // BMP180 returns pressure in kPa
            // val1 = integer kPa, val2 = fractional kPa (in millionths)
            // Convert to Pascals: kPa * 1000
            
            float pressure_kpa = press.val1 + (press.val2 / 1000000.0f);
            uint32_t pressure_pa = (uint32_t)(pressure_kpa * 1000.0f);
            
            LOG_INF("Pressure: %u Pa (%.3f hPa)", pressure_pa, pressure_pa / 100.0f);
            
            ret = ble_service_update_pressure(pressure_pa);
            if (ret < 0) {
                LOG_WRN("Failed to update BLE pressure");
            }
        } else {
            LOG_ERR("Failed to get pressure channel (%d)", ret);
        }

        k_msleep(SENSOR_UPDATE_INTERVAL_SEC * 1000);
    }
    return 0;
}