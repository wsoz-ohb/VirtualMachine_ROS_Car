#include "my_wifi_app.h"
#include "my_oled.h"
#include "esp_netif.h"

static const char *TAG = "my_wifi_app";

SemaphoreHandle_t wifi_ok = NULL; // 创建一个信号量

void wifi_connected_thread(void *pvParameters)
{
    wifi_ok = xSemaphoreCreateBinary(); // 创建一个二值信号量
    if (wifi_ok == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return;
    }
    app_wifi_start();
    while (wifi_connected_already() != WIFI_STATUS_CONNECTED_OK)
    {
        ESP_LOGI(TAG, "wifi is connecting...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "wifi is connected!");

    // 获取 IP 地址并显示到 OLED
    esp_netif_ip_info_t ip_info;
    esp_netif_t* sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif && esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK) {
        char ip_str[20];
        snprintf(ip_str, sizeof(ip_str), "IP:%d.%d.%d.%d",
                 IP2STR(&ip_info.ip));
        OLED_ShowStr(0, 1, ip_str, OLED_FONT_SIZE_6X8);
        ESP_LOGI(TAG, "Displayed IP on OLED: %s", ip_str);
    }

    xSemaphoreGive(wifi_ok); // 释放信号量
    vTaskDelete(NULL);
}

void my_wifi_app_start(void)
{
    xTaskCreate(wifi_connected_thread, "wifi_connected_thread", 4096, NULL, 5, NULL);
}