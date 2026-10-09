/**
 * @file    task_alarm.c
 * @brief   AlarmTask 实现 — 读取传感器数据、判断阈值、控制声光报警
 * @note    报警规则：
 *           红灯 = CO2 超标 / 综合指数超标 / 高温
 *           黄灯 = 噪声超标 / 湿度过低
 *           绿灯 = 有人靠近
 *           蜂鸣器 = 报警条件「上升沿」触发一次（避免每周期重复鸣叫塞满队列）
 *           BuzzerTask 通过队列异步执行蜂鸣，不阻塞报警循环
 */

#include "main.h"
#include "task_alarm.h"
#include "task_buzzer.h"
#include "task_shared.h"

/* 报警状态位 */
#define FLAG_CO2    0x01U
#define FLAG_TEMP   0x02U
#define FLAG_NOISE  0x04U
#define FLAG_AQI    0x08U
#define FLAG_DIST   0x10U

/* 非阻塞发送蜂鸣指令 */
static void Alarm_Beep(uint8_t times, uint16_t on_ms, uint16_t off_ms)
{
    BuzzerCmd_t cmd;
    cmd.times  = times;
    cmd.on_ms  = on_ms;
    cmd.off_ms = off_ms;
    xQueueSend(BuzzerQueue, &cmd, 0);   /* 队列满则丢弃，不阻塞 */
}

void AlarmTask_Run(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    static uint8_t prev_flags = 0;

    while (1) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(ALARM_CHECK_PERIOD_MS));

        /* 获取传感器数据快照 */
        xSemaphoreTakeRecursive(Data_Mutex, portMAX_DELAY);
        SensorData_t local = g_sensorData;
        xSemaphoreGiveRecursive(Data_Mutex);

        /* ====== 计算当前报警状态 ====== */
        uint8_t flags = 0;
        if (local.co2     > ALARM_CO2_HIGH   && local.co2 > 0.0f)     flags |= FLAG_CO2;
        if (local.temperature > ALARM_TEMP_HIGH)                        flags |= FLAG_TEMP;
        if (local.noise_db > ALARM_NOISE_HIGH && local.noise_db > 0.0f) flags |= FLAG_NOISE;
        if (local.aqi     > ALARM_AQI_HIGH)                            flags |= FLAG_AQI;
        if (local.distance < ALARM_DIST_CLOSE && local.distance > 0.0f) flags |= FLAG_DIST;

        /* ====== 蜂鸣器：仅上升沿触发（优先级：CO2/AQI > 温度 > 噪声 > 靠近） ====== */
        uint8_t rise = flags & (uint8_t)(~prev_flags);
        if (rise & (FLAG_CO2 | FLAG_AQI)) {
            Alarm_Beep(3, 200, 200);   /* 滴滴滴三声 */
        } else if (rise & FLAG_TEMP) {
            Alarm_Beep(2, 300, 200);
        } else if (rise & FLAG_NOISE) {
            Alarm_Beep(2, 150, 150);
        } else if (rise & FLAG_DIST) {
            Alarm_Beep(1, 100, 0);
        }
        prev_flags = flags;

        /* ====== 红灯：CO2 超标 / 综合指数超标 / 高温 ====== */
        if (flags & (FLAG_CO2 | FLAG_AQI | FLAG_TEMP)) {
            LED_On(LED_RED);
        } else {
            LED_Off(LED_RED);
        }

        /* ====== 黄灯：噪声超标 / 湿度过低 ====== */
        if ((flags & FLAG_NOISE) ||
            (local.humidity < ALARM_HUMI_LOW && local.humidity > 0.0f)) {
            LED_On(LED_YELLOW);
        } else {
            LED_Off(LED_YELLOW);
        }

        /* ====== 绿灯：有人靠近 ====== */
        if (flags & FLAG_DIST) {
            LED_On(LED_GREEN);
        } else {
            LED_Off(LED_GREEN);
        }
    }
}
