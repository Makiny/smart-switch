#include <string.h>

#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>

#include <app_hap_setup_payload.h>
#include <esp_log.h>
#include "device.h"
#include "switch_control.h"
#include "ha_switch.h"

#define SWITCH_TASK_PRIORITY  1
#define SWITCH_TASK_STACKSIZE (4 * 1024)
#define SWITCH_TASK_NAME "ha_switch"

static const char *TAG = "ha_switch";

static hap_char_t *device_active_char;
static hap_char_t *on_char;

void hap_device_active_update(bool is_active) {
    hap_val_t new_val;
    new_val.b = is_active;
    hap_char_update_val(device_active_char, &new_val);
}

void hap_switch_status_update(bool is_on)
{
    hap_val_t new_val;
    new_val.b = is_on;
    hap_char_update_val(on_char, &new_val);
}

/* Mandatory identify routine for the accessory.
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */
static int switch_identify(hap_acc_t *ha)
{
    ESP_LOGI(TAG, "Accessory identified");
    return HAP_SUCCESS;
}

int switch_read(hap_char_t *hc, hap_status_t *status_code, void *serv_priv, void *read_priv) {
    *status_code = HAP_STATUS_SUCCESS;
    return HAP_SUCCESS;
}

static int switch_write(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv)
{
    int i, ret = HAP_SUCCESS;
    hap_write_data_t *write;
    for (i = 0; i < count; i++) {
        write = &write_data[i];
        *(write->status) = HAP_STATUS_VAL_INVALID;
        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON)) {
            if(switch_status_update(write->val.b) == ESP_OK) {
                *(write->status) = HAP_STATUS_SUCCESS;
            }
        }

        if (*(write->status) == HAP_STATUS_SUCCESS) {
            hap_char_update_val(write->hc, &(write->val));
        } else {
            ret = HAP_FAIL;
        }
    }
    return ret;
}

static void hap_switch_task(void *arg)
{
    hap_acc_t *accessory;
    hap_serv_t *service;

    /* Initialize the HAP core */
    hap_init(HAP_TRANSPORT_WIFI);

    /* Initialise the mandatory parameters for Accessory which will be added as
     * the mandatory services internally
     */
    hap_acc_cfg_t cfg = {
            .name = device_info.model,
            .manufacturer = device_info.mft,
            .model = device_info.model,
            .serial_num = device_info.sn,
            .fw_rev = device_info.fw_v,
            .hw_rev = device_info.hw_v,
            .pv = "1.1.0",
            .identify_routine = switch_identify,
            .cid = HAP_CID_LIGHTING,
    };

    /* Create accessory object */
    accessory = hap_acc_create(&cfg);
    if (!accessory) {
        ESP_LOGE(TAG, "Failed to create accessory");
        goto light_err;
    }

    /* Add a dummy Product Data */
    uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
    hap_acc_add_product_data(accessory, product_data, sizeof(product_data));

    /* Create the Light Bulb Service. Include the "name" since this is a user visible service  */
    service = hap_serv_outlet_create(false, false);
    if (!service) {
        ESP_LOGE(TAG, "Failed to create LightBulb Service");
        goto light_err;
    }
    on_char = hap_serv_get_char_by_uuid(service, HAP_CHAR_UUID_ON);

    /* Add the optional characteristic to the Light Bulb Service */
    int ret = hap_serv_add_char(service, hap_char_name_create("My Switch"));
    if (ret != HAP_SUCCESS) {
        ESP_LOGE(TAG, "Failed to add optional characteristics to LightBulb");
        goto light_err;
    }
    device_active_char = hap_char_status_active_create(true);
    hap_serv_add_char(service, device_active_char);

    /* Set the write callback for the service */
    hap_serv_set_read_cb(service, switch_read);
    hap_serv_set_write_cb(service, switch_write);

    /* Add the Light Bulb Service to the Accessory Object */
    hap_acc_add_serv(accessory, service);

    /* Add the Accessory to the HomeKit Database */
    hap_add_accessory(accessory);

    /* TODO: Do the actual hardware initialization here */

    /* For production accessories, the setup code shouldn't be programmed on to
     * the device. Instead, the setup info, derived from the setup code must
     * be used. Use the factory_nvs_gen utility to generate this data and then
     * flash it into the factory NVS partition.
     *
     * By default, the setup ID and setup info will be read from the factory_nvs
     * Flash partition and so, is not required to set here explicitly.
     *
     * However, for testing purpose, this can be overridden by using hap_set_setup_code()
     * and hap_set_setup_id() APIs, as has been done here.
     */
#ifdef CONFIG_EXAMPLE_USE_HARDCODED_SETUP_CODE
    /* Unique Setup code of the format xxx-xx-xxx. Default: 111-22-333 */
    hap_set_setup_code(CONFIG_EXAMPLE_SETUP_CODE);
    /* Unique four character Setup Id. Default: ES32 */
    hap_set_setup_id(CONFIG_EXAMPLE_SETUP_ID);
#ifdef CONFIG_APP_WIFI_USE_WAC_PROVISIONING
    app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, true, cfg.cid);
#else
    app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, false, cfg.cid);
#endif
#endif

    /* Enable Hardware MFi authentication (applicable only for MFi variant of SDK) */
    hap_enable_mfi_auth(HAP_MFI_AUTH_NONE);

    /* After all the initializations are done, start the HAP core */
    hap_start();

    /* The task ends here. The read/write callbacks will be invoked by the HAP Framework */
    vTaskDelete(NULL);

    light_err:
    ESP_LOGE("HAP main", "HAP start error");
    hap_acc_delete(accessory);
    vTaskDelete(NULL);
}

void hap_switch_task_create() {
    xTaskCreate(hap_switch_task, SWITCH_TASK_NAME, SWITCH_TASK_STACKSIZE,
                NULL, SWITCH_TASK_PRIORITY, NULL);
}