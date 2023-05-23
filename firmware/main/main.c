#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include <esp_wifi.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <lwip/ip4_addr.h>
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "wifi_manager.h"
#include <esp_http_client.h>

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "main";

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

static QueueHandle_t tx_task_queue;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    return ESP_OK;
}

/**
 * @brief this is an exemple of a callback that you can setup in your own app to get notified of wifi manager event.
 */
void cb_connection_ok(void *pvParameter){
    ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;

    /* transform IP to human readable string */
    char str_ip[16];
    esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

    ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);

    unsigned int tx_action = 1;
    xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);
}

void monitoring_task(void *pvParameter)
{
	for(;;){
		ESP_LOGI(TAG, "free heap: %"PRIu32, esp_get_free_heap_size());
		vTaskDelay( pdMS_TO_TICKS(10000) );
	}
}

void
transmit_task(void *arg)
{
    while (1) {
        unsigned int action;
        char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
        xQueueReceive(tx_task_queue, &action, portMAX_DELAY);

        printf("transmitting %d\n", action);

        switch(action) {
            case 1:
                {
                    esp_http_client_config_t config = {
                        .host = "192.168.0.10",
                        .path = "/switch_cammode.cgi",
                        .query = "mode=shutter",
                        .event_handler = _http_event_handler,
                        .user_data = local_response_buffer,        // Pass address of local buffer to get response
                        .disable_auto_redirect = true,
                    };

                    esp_http_client_handle_t client = esp_http_client_init(&config);

                    // GET
                    esp_err_t err = esp_http_client_perform(client);
                    if (err == ESP_OK) {
                        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRIu64,
                                esp_http_client_get_status_code(client),
                                esp_http_client_get_content_length(client));
                    } else {
                        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
                    }
                }

                break;
        }
    }
}

void app_main(void)
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    tx_task_queue = xQueueCreate(1, sizeof(uint32_t));
    xTaskCreatePinnedToCore(transmit_task, "request task", 4096, NULL, 8, NULL, tskNO_AFFINITY);

    /* start the wifi manager */
    wifi_manager_start();

    wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
    xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1);
}
