/**
 * @file    task_sr04.c
 * @brief   SR04Task — 阻塞在信号量上，触发后测量一次距离
 * @note    优先级 1（最低），忙等测距不会影响 UI / 传感器 / 按键。
 *          SensorTask 触发后通过 g_sr04_distance 读取结果。
 */

#include "main.h"
#include "task_sr04.h"
#include "task_shared.h"

/* SR04 测量结果（SensorTask 通过 Data_Mutex 保护读取） */
float g_sr04_distance = -999;

void SR04Task_Run(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        /* 阻塞等待触发信号 */
        if (xSemaphoreTake(SR04_Semaphore, portMAX_DELAY) == pdTRUE) {
            float val;
            if (SR04_Measure(&val) == 0 && val > 1.0f && val < 450.0f) {
                g_sr04_distance = val;
            } else {
                g_sr04_distance = -999;
            }
        }
    }
}
