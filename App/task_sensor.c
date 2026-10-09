/**
 * @file    task_sensor.c
 * @brief   SensorTask 实现 — 采集 SCD30(CO2/温湿度) / BH1750(光照) /
 *          MAX9814(噪声) / HC-SR04(距离) / WiFi 状态，并计算综合指数
 * @note    SCD30 + BH1750: 需持有 I2C 互斥量（与 OLED 共享 I2C1）
 *          MAX9814:       ADC1 采样，无竞争
 *          HC-SR04:       定时器输入捕获，阻塞但短
 *          DHT22:         仅作 SCD30 失效时的温湿度备选
 */

#include "main.h"
#include "task_sensor.h"
#include "task_shared.h"

void SensorTask_Run(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    float val;

    while (1) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSOR_SAMPLE_PERIOD_MS));

        /* 临时存储（先采集再一次性写入，减少持锁时间） */
        float temperature = -999, humidity = -999, light = -999, distance = -999;
        float co2 = -999, noise_db = -999, aqi = 0;
        uint8_t aqi_level = AQI_LEVEL_EXCELLENT;
        uint8_t wifi_ok = 0;

        /* --- SCD30(CO2+温湿度) 与 BH1750(光照)，共持 I2C 互斥量 --- */
        xSemaphoreTakeRecursive(I2C_Mutex, portMAX_DELAY);
        {
            uint8_t ready = 0;
            if (SCD30_IsDataReady(&I2C_BUS, &ready) == HAL_OK && ready) {
                SCD30_Measurement_t m;
                if (SCD30_ReadMeasurement(&I2C_BUS, &m) == HAL_OK) {
                    co2         = m.co2;
                    temperature = m.temperature;   /* SCD30 自带温湿度 */
                    humidity    = m.humidity;
                }
            }

            float lux = 0;
            if (BH1750_ReadLight(&lux) == 0) {
                light = lux;
            }
        }
        xSemaphoreGiveRecursive(I2C_Mutex);

        /* --- DHT22 温湿度备选（仅当 SCD30 未读到有效值时） --- */
        if (temperature < -900.0f || humidity < -900.0f) {
            float t = 0, h = 0;
            if (DHT22_Read(&t, &h) == DHT22_OK) {
                if (t >= -40.0f && t <= 80.0f)  temperature = t;
                if (h >= 0.0f  && h <= 100.0f) humidity    = h;
            }
        }

        /* --- MAX9814 噪声 (ADC1) --- */
        noise_db = Noise_ReadDB();

        /* --- HC-SR04 距离：信号量阻塞等待，ISR 唤醒 --- */
        if (SR04_Measure(&val) == 0 && val > 1.0f && val < 450.0f) {
            distance = val;
        }

        /* --- 综合环境指数 --- */
        aqi       = AQI_Compute(co2, temperature, humidity, noise_db);
        aqi_level = (uint8_t)AQI_Level(aqi);

        /* --- WiFi 状态（读静态变量，不需要 UART 锁） --- */
        wifi_ok = ESP8266_IsConnected();

        /* --- 写入全局数据（持锁） --- */
        xSemaphoreTakeRecursive(Data_Mutex, portMAX_DELAY);
        if (temperature > -900.0f) g_sensorData.temperature = temperature;
        if (humidity > -900.0f)    g_sensorData.humidity    = humidity;
        if (light > -900.0f)       g_sensorData.light       = light;
        if (distance > -900.0f)    g_sensorData.distance    = distance;
        if (co2 > -900.0f)         g_sensorData.co2         = co2;
        if (noise_db > -900.0f)    g_sensorData.noise_db    = noise_db;
        g_sensorData.aqi            = aqi;
        g_sensorData.aqi_level      = aqi_level;
        g_sensorData.wifi_connected = wifi_ok;
        xSemaphoreGiveRecursive(Data_Mutex);
    }
}
