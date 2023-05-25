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
#include <driver/gpio.h>

/* @brief tag used for ESP serial console messages */
static const char TAG[] = "main";

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
#define ESP_INTR_FLAG_DEFAULT 0

#define GPIO_INPUT_IO_0     18
#define GPIO_INPUT_IO_1     19
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))

static QueueHandle_t tx_task_queue;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    return ESP_OK;
}

uint8_t mask0 = 0;
uint8_t mask1 = 0;
int io0 = 0;
int io1 = 0;

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

void button_task(void *pvParameter)
{
    for(;;){
        uint8_t current0 = gpio_get_level(GPIO_INPUT_IO_0);
        uint8_t current1 = gpio_get_level(GPIO_INPUT_IO_1);

        mask0 = (mask0 << 1) | current0 | 0xF0;
        mask1 = (mask1 << 1) | current1 | 0xF0;

        // It's been high or low for 4 bits (4 * 5ms == 20ms)
        if (mask0 == 0xF0 || mask0 == 0xFF) {
            if (current0 != io0) {
                io0 = current0;
                unsigned int tx_action = GPIO_INPUT_IO_0;
                xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);
            }
        }

        if (mask1 == 0xF0 || mask1 == 0xFF) {
            if (current1 != io1) {
                io1 = current1;
                unsigned int tx_action = GPIO_INPUT_IO_1;
                xQueueSend(tx_task_queue, &tx_action, portMAX_DELAY);
            }
        }
        vTaskDelay( pdMS_TO_TICKS(10) );
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
            case GPIO_INPUT_IO_0:
                if (gpio_get_level(GPIO_INPUT_IO_0)) {
                    printf("button 1 pressed\n");
                } else {
                    printf("button 1 released\n");
                }
                break;
            case GPIO_INPUT_IO_1:
                if (gpio_get_level(GPIO_INPUT_IO_1)) {
                    printf("button 2 pressed\n");
                } else {
                    printf("button 2 released\n");
                }
                break;
        }
    }
}

void app_main(void)
{
    tx_task_queue = xQueueCreate(5, sizeof(uint32_t));
    xTaskCreatePinnedToCore(transmit_task, "request task", 4096, NULL, 8, NULL, tskNO_AFFINITY);

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 1;
    gpio_config(&io_conf);

    io0 = gpio_get_level(GPIO_INPUT_IO_0);
    io1 = gpio_get_level(GPIO_INPUT_IO_1);
    if (io0) {
        mask0 = 0xFF;
    }
    else {
        mask0 = 0xF0;
    }

    if (io1) {
        mask1 = 0xFF;
    }
    else {
        mask1 = 0xF0;
    }

    /* start the wifi manager */
    wifi_manager_start();

    wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
    xTaskCreatePinnedToCore(monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, tskNO_AFFINITY);
    xTaskCreatePinnedToCore(button_task, "button_task", 4096, NULL, 9, NULL, tskNO_AFFINITY);
}
