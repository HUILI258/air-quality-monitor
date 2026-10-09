/**
 * @file    menu.c
 * @brief   OLED 5个页面的渲染实现
 * @note    128×64 像素，5×7字体，每行8像素，共8页
 *          ┌──────────────┐ 页0: 标题
 *          │  标题        │ 页1-2: 主内容
 *          │  内容区      │ 页7: 页脚（按键提示）
 *          │              │
 *          │  页脚提示    │
 *          └──────────────┘
 */

#include "main.h"
#include "menu.h"

/* ==================== 主页 ==================== */
void Menu_ShowMain(const SensorData_t *data)
{
    /* 第0行：标题 */
    OLED_ShowString(0, 0, "=== AIR QUALITY ===", 0);

    /* 第1行：CO2 */
    OLED_ShowString(0, 1, "CO2:", 0);
    if (data->co2 > -900) {
        OLED_ShowNum(30, 1, (int32_t)data->co2, 4);
        OLED_ShowString(60, 1, "ppm", 0);
    } else {
        OLED_ShowString(30, 1, "-- ppm", 0);
    }

    /* 第2行：温湿度 */
    OLED_ShowString(0, 2, "T:", 0);
    if (data->temperature > -900) {
        OLED_ShowFloat(12, 2, data->temperature, 2, 1);
        OLED_ShowString(46, 2, "C", 0);
    } else {
        OLED_ShowString(12, 2, "--C", 0);
    }
    OLED_ShowString(64, 2, "H:", 0);
    if (data->humidity > -900) {
        OLED_ShowFloat(76, 2, data->humidity, 2, 1);
        OLED_ShowString(110, 2, "%", 0);
    } else {
        OLED_ShowString(76, 2, "--%", 0);
    }

    /* 第3行：光照 + 噪声 */
    OLED_ShowString(0, 3, "Lux:", 0);
    if (data->light > -900) {
        OLED_ShowNum(30, 3, (int32_t)data->light, 5);
    } else {
        OLED_ShowString(30, 3, "--", 0);
    }
    OLED_ShowString(64, 3, "dB:", 0);
    if (data->noise_db > -900) {
        OLED_ShowNum(82, 3, (int32_t)data->noise_db, 2);
    } else {
        OLED_ShowString(82, 3, "--", 0);
    }

    /* 第5行：综合指数 */
    OLED_ShowString(0, 5, "AQI:", 0);
    if (data->co2 > -900 || data->noise_db > -900) {
        OLED_ShowNum(30, 5, (int32_t)data->aqi, 3);
        OLED_ShowString(54, 5, AQI_LevelStr((AQI_Level_t)data->aqi_level), 0);
    } else {
        OLED_ShowString(30, 5, "--", 0);
    }

    /* 第7行：页脚（WiFi状态 + 页面指示） */
    if (data->wifi_connected) {
        OLED_ShowString(0, 7, "WiFi:OK", 0);
    } else {
        OLED_ShowString(0, 7, "WiFi:--", 0);
    }
    OLED_ShowString(78, 7, "[1/5]", 0);
}

/* ==================== 详情页：数值 + 简单条形图 ==================== */
void Menu_ShowDetail(const SensorData_t *data)
{
    OLED_ShowString(0, 0, "--- Detail ---", 0);

    /* CO2 */
    OLED_ShowString(0, 1, "CO2:", 0);
    if (data->co2 > -900) {
        OLED_ShowNum(30, 1, (int32_t)data->co2, 4);
        OLED_ShowString(60, 1, "ppm", 0);
    } else {
        OLED_ShowString(30, 1, "-- ppm", 0);
    }

    /* 光照 */
    OLED_ShowString(0, 2, "Lux:", 0);
    if (data->light > -900) {
        OLED_ShowNum(30, 2, (int32_t)data->light, 5);
    } else {
        OLED_ShowString(30, 2, "--", 0);
    }

    /* 噪声 */
    OLED_ShowString(0, 3, "Noise:", 0);
    if (data->noise_db > -900) {
        OLED_ShowNum(42, 3, (int32_t)data->noise_db, 2);
        OLED_ShowString(60, 3, "dB", 0);
    } else {
        OLED_ShowString(42, 3, "-- dB", 0);
    }

    /* AQI + 进度条 */
    OLED_ShowString(0, 5, "AQI:", 0);
    OLED_ShowNum(30, 5, (int32_t)data->aqi, 3);
    OLED_ShowString(54, 5, AQI_LevelStr((AQI_Level_t)data->aqi_level), 0);
    uint8_t aqi_p = (uint8_t)data->aqi;
    if (aqi_p > 100) aqi_p = 100;
    OLED_DrawProgressBar(0, 6, 100, aqi_p);

    OLED_ShowString(78, 7, "[2/5]", 0);
}

/* ==================== 设置页：报警阈值 ==================== */
void Menu_ShowSettings(void)
{
    OLED_ShowString(0, 0, "--- Settings ---", 0);

    OLED_ShowString(0, 1, "CO2 Alrm: >", 0);
    OLED_ShowNum(72, 1, (int32_t)ALARM_CO2_HIGH, 4);
    OLED_ShowString(102, 1, "ppm", 0);

    OLED_ShowString(0, 3, "Noise: >", 0);
    OLED_ShowNum(72, 3, (int32_t)ALARM_NOISE_HIGH, 2);
    OLED_ShowString(90, 3, "dB", 0);

    OLED_ShowString(0, 5, "AQI Alrm: >", 0);
    OLED_ShowNum(72, 5, (int32_t)ALARM_AQI_HIGH, 3);

    OLED_ShowString(78, 7, "[3/5]", 0);
}

/* ==================== WiFi 页面 ==================== */
void Menu_ShowWiFi(const SensorData_t *data)
{
    OLED_ShowString(0, 0, "--- WiFi ---", 0);

    OLED_ShowString(0, 2, "SSID:", 0);
    OLED_ShowString(30, 2, WIFI_SSID, 0);

    /* Status: 只显示连接/未连接 */
    OLED_ShowString(0, 4, "Status:", 0);
    if (data->wifi_connected) {
        OLED_ShowString(48, 4, "Connected", 0);
    } else {
        OLED_ShowString(48, 4, "Disconnected", 0);
    }

    /* 操作提示行：动态反映连接进度 */
    switch (wifi_status) {
        case 0:
        case 2: OLED_ShowString(0, 6, "[OK] Reconnect", 0); break;
        case 1: OLED_ShowString(0, 6, "Reconnecting...", 0); break;
        case 3: OLED_ShowString(0, 6, "Error 1: ESP",   0); break;
        case 4: OLED_ShowString(0, 6, "Error 2: WiFi",  0); break;
        default: break;
    }

    OLED_ShowString(78, 7, "[4/5]", 0);
    (void)data;
}

/* ==================== 关于页 ==================== */
void Menu_ShowAbout(void)
{
    OLED_ShowString(0, 0, "--- About ---", 0);
    OLED_ShowString(0, 2, "Air Quality v1.0", 0);
    OLED_ShowString(0, 3, "STM32F103C8T6", 0);
    OLED_ShowString(0, 4, "SCD30 BH1750", 0);
    OLED_ShowString(0, 5, "MAX9814 OLED", 0);
    OLED_ShowString(0, 6, "DHT22 HC-SR04", 0);
    OLED_ShowString(78, 7, "[5/5]", 0);
}
