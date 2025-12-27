#include <zephyr/kernel.h>

// For advertising
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

// For connection and GATT
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

// For logging
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_service, LOG_LEVEL_INF);
// Custom Sensor Service UUID
#define BT_UUID_SENSOR_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x59E2FBF4,0x592A,0x45F6,0xAC84,0xBB3DCDB33F56)
#define BT_UUID_SENSOR_SERVICE \
    BT_UUID_DECLARE_128(BT_UUID_SENSOR_SERVICE_VAL)

// Temperature Characteristic UUID
#define BT_UUID_TEMP_CHAR_VAL \
    BT_UUID_128_ENCODE(0x57DF5C86,0x7899,0x42B3,0xADF0,0xBE2EFFE11E7E)
#define BT_UUID_TEMP_CHAR \
    BT_UUID_DECLARE_128(BT_UUID_TEMP_CHAR_VAL)

// Pressure Characteristic UUID
#define BT_UUID_PRESSURE_CHAR_VAL \
    BT_UUID_128_ENCODE(0x02CDC712,0xC8E2,0x4130,0xA9E8,0x2C10EE820BFC)
#define BT_UUID_PRESSURE_CHAR \
    BT_UUID_DECLARE_128(BT_UUID_PRESSURE_CHAR_VAL)

// Temperature Value
static int16_t temperature_value = 0;
static bool temp_notify_enabled = false;

// Pressure Value
static int32_t pressure_value = 0;
static bool pressure_notify_enabled = false;

static struct bt_conn *current_conn = NULL;

/**
 * Temperature CCC changed callback
 */
static void temp_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value){
    temp_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Temperature notification %s", temp_notify_enabled ? "enabled" : "disabled");
}

/**
 * Pressure CCC changed callback
 */
static void pressure_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value){
    pressure_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
    LOG_INF("Pressure notification %s", pressure_notify_enabled ? "enabled" : "disabled");
}

/**
 * Temperature read callback
 */
static ssize_t read_temp(struct bt_conn *conn,
                         const struct bt_gatt_attr *attr,
                         void *buf, uint16_t len,
                         uint16_t offset)
{
    int16_t temp = temperature_value;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &temp, sizeof(temp));
}

/**
 * Pressure read callback
 */
static ssize_t read_pressure(struct bt_conn *conn,
                              const struct bt_gatt_attr *attr,
                              void *buf, uint16_t len,
                              uint16_t offset)
{
    int32_t pressure = pressure_value;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &pressure, sizeof(pressure));
}

/**
 * GATT Service Definition
 */
BT_GATT_SERVICE_DEFINE(sensor_svc,
    // The service declaration
    BT_GATT_PRIMARY_SERVICE(BT_UUID_SENSOR_SERVICE),

    // The temperature characteristic declaration
    BT_GATT_CHARACTERISTIC(BT_UUID_TEMP_CHAR,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,             // Properties
                           BT_GATT_PERM_READ,                                   // Permissions
                           read_temp, NULL, NULL),                              // Callbacks: Read, Write, User Data

    // Descriptor: metadata for Client Characteristic Configuration                      
    BT_GATT_CCC(temp_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    // The pressure characteristic declaration
    BT_GATT_CHARACTERISTIC(BT_UUID_PRESSURE_CHAR,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,             // Properties
                           BT_GATT_PERM_READ,                                   // Permissions
                           read_pressure, NULL, NULL),                          // Callbacks: Read, Write, User Data

    // Descriptor: metadata for Client Characteristic Configuration                      
    BT_GATT_CCC(pressure_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/**
 * Connection callback
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed (err 0x%02x)", err);
    } else {
        current_conn = bt_conn_ref(conn);
        LOG_INF("Connected");
    }
}

/**
 * Disconnection callback
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected (reason 0x%02x)", reason);
    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }

    temp_notify_enabled = false;
    pressure_notify_enabled = false;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

/**
 * Advertising data
 */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_SENSOR_SERVICE_VAL),
};

/**
 * Scan response data
 */
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

int ble_service_init(void)
{
    int err;

    LOG_INF("Initializing BLE Sensor Service");

    // Enable Bluetooth Subsystem
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }

    LOG_INF("Bluetooth initialized");

    // Start Advertising
    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }

    LOG_INF("Advertising successfully started");

    return 0;
}

int ble_service_update_temperature(float temp_celsius)
{
    temperature_value = (int16_t)(temp_celsius * 100); // Convert to centi-degrees

    if (temp_notify_enabled && current_conn) {
        int err = bt_gatt_notify(current_conn, &sensor_svc.attrs[1], &temperature_value, sizeof(temperature_value));
        if (err) {
            LOG_ERR("Failed to send temperature notification (err %d)", err);
            return err;
        }
        LOG_DBG("Temperature notification sent: %d.%02d C", temperature_value / 100, temperature_value % 100);
    }

    return 0;
}

int ble_service_update_pressure(uint32_t pressure_pa)
{
    pressure_value = pressure_pa; 

    LOG_DBG("Pressure updated: %u Pa (%.2f hPa)", pressure_value, pressure_value / 100.0);
    if (pressure_notify_enabled && current_conn) {
        int err = bt_gatt_notify(current_conn, &sensor_svc.attrs[3], &pressure_value, sizeof(pressure_value));
        if (err) {
            LOG_ERR("Failed to send pressure notification (err %d)", err);
            return err;
        }
        LOG_DBG("Pressure notification sent: %u Pa", pressure_value);
    }

    return 0;
}

bool ble_service_is_temp_notify_enabled(void)
{
    return temp_notify_enabled;
}

bool ble_service_is_pressure_notify_enabled(void)
{
    return pressure_notify_enabled;
}




