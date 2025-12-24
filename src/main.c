#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_beacon, LOG_LEVEL_INF);

// Advertising Data
static const struct bt_data ad[] = {
    /**
     * Declare elements of bt_data array
     * */
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
            sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

int main(void){
    int err;

    LOG_INF("Starting BLE Beacon on STM32WB55RG...");

    /**
     * Enable Bluetooth Subsystem
     */
    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }

    LOG_INF("Bluetooth initialized");

    /**
     * Start Advertising data
     * @param BT_LE_ADV_CONN_NAME: Advertising parameters for connectable advertising with name
     * @param ad: Advertising data
     * @param ARRAY_SIZE(ad): Size of advertising data array
     * @param NULL: No scan response data
     * @param 0: Size of scan response data array
     */
    err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }

    LOG_INF("Advertising successfully started");

    while(1){
        k_sleep(K_SECONDS(1));
    }   

    return 0;
}