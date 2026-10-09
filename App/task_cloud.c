/**
 * @file    task_cloud.c
 * @brief   CloudTask 实现 — 每 60 秒将传感器数据上传到 ThingSpeak
 * @note    最低优先级 (prio=1)，ESP8266 AT 指令阻塞时间 5~15s
 *          UART_Mutex 序列化所有 ESP8266 通信
 */

#include "main.h"
#include "task_cloud.h"
#include "task_shared.h"

volatile uint8_t wifi_status = 0;
static SemaphoreHandle_t connect_sem = NULL;

void CloudTask_Run(void *pvParameters)
{
    (void)pvParameters;
    connect_sem = xSemaphoreCreateBinary();
    configASSERT(connect_sem != NULL);

    /* LED 闪 1 次 = CloudTask 启动 */
    LED_On(LED_GREEN);  vTaskDelay(pdMS_TO_TICKS(50)); LED_Off(LED_GREEN);

    ESP8266_Init();

    /* LED 闪 2 次 = ESP 初始化完成 */
    for(int i=0;i<2;i++){LED_On(LED_GREEN);vTaskDelay(pdMS_TO_TICKS(50));LED_Off(LED_GREEN);vTaskDelay(pdMS_TO_TICKS(100));}

    vTaskDelay(pdMS_TO_TICKS(3000));

    /* LED 闪 3 次 = 主循环就绪 */
    for(int i=0;i<3;i++){LED_On(LED_GREEN);vTaskDelay(pdMS_TO_TICKS(50));LED_Off(LED_GREEN);vTaskDelay(pdMS_TO_TICKS(100));}

    while (1) {
        /* 等 KEY2 或 30s 自动重试 */
        if (xSemaphoreTake(connect_sem, pdMS_TO_TICKS(30000)) == pdTRUE || !ESP8266_IsConnected()) {
            if (ESP8266_IsConnected()) continue;  /* 已连则跳过 */

            wifi_status = 1;
            ESP_Status_t r;
            xSemaphoreTakeRecursive(UART_Mutex, portMAX_DELAY);
            r = ESP8266_ConnectWiFi();
            xSemaphoreGiveRecursive(UART_Mutex);

            if (r == ESP_OK) {
                wifi_status = 2;
            } else if (r == ESP_ERR_TIMEOUT) {
                wifi_status = 3;
            } else {
                wifi_status = 4;
            }
        }

        if (ESP8266_IsConnected()) {
            SensorData_t local;
            xSemaphoreTakeRecursive(Data_Mutex, portMAX_DELAY);
            local = g_sensorData;
            xSemaphoreGiveRecursive(Data_Mutex);

            xSemaphoreTakeRecursive(UART_Mutex, portMAX_DELAY);
            ESP8266_SendData(local.temperature, local.humidity,
                             local.light, local.distance);
            xSemaphoreGiveRecursive(UART_Mutex);
        }
    }
}

void CloudTask_RequestConnect(void)
{
    if (connect_sem) xSemaphoreGive(connect_sem);
}
