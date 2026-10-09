/**
 * @file    task_display.h
 * @brief   DisplayTask — 1s 周期 OLED 刷新 + 按键事件处理
 */

#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H

#include "FreeRTOS.h"
#include "task.h"

#define DISPLAY_TASK_STACK_SIZE     512     /* OLED浮点渲染+I2C 需更大栈 */
#define DISPLAY_TASK_PRIORITY       2
#define DISPLAY_REFRESH_PERIOD_MS   300U    /* 300ms 刷新，按键更灵敏 */

void DisplayTask_Run(void *pvParameters);

#endif
