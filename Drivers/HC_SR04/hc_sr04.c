/**
 * @file    hc_sr04.c
 * @brief   HC-SR04 — 非阻塞：发 Trig 即返回，TIM3 后台捕获
 * @note    SensorTask 每 5s 调一次：发 Trig → 立即返回 → 读上次结果
 *          TIM3 ISR 在后台完成捕获，零忙等、零信号量。
 */

#include "main.h"
#include "hc_sr04.h"

static volatile uint32_t sr04_rise   = 0;
static volatile uint32_t sr04_width  = 0;
static volatile uint8_t  sr04_ready  = 0;  /* 本次有新数据 */
static uint32_t sr04_last_valid = 0;
static uint8_t  sr04_started    = 0;

void SR04_Init(void)
{
    HAL_GPIO_WritePin(SR04_TRIG_PORT, SR04_TRIG_PIN, GPIO_PIN_RESET);
}

uint8_t SR04_Measure(float *distance_cm)
{
    uint8_t has_new = 0;

    /* 首次调用：初始化并启动 TIM3 */
    if (!sr04_started) {
        HAL_NVIC_SetPriority(TIM3_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM3_IRQn);
        __HAL_TIM_SET_CAPTUREPOLARITY(SR04_TIM, SR04_TIM_CHANNEL, TIM_INPUTCHANNELPOLARITY_RISING);
        HAL_TIM_IC_Start_IT(SR04_TIM, SR04_TIM_CHANNEL);
        sr04_started = 1;
    }

    /* 读上次触发的结果 */
    if (sr04_ready) {
        sr04_ready = 0;
        if (sr04_width > 0 && sr04_width < 38000) {
            sr04_last_valid = sr04_width;
            *distance_cm = (float)sr04_width / 58.0f;
            has_new = 1;
        }
    }

    /* 无论有没有新数据，都发 Trig 触发下次测量 */
    sr04_rise = 0;
    __HAL_TIM_SET_CAPTUREPOLARITY(SR04_TIM, SR04_TIM_CHANNEL, TIM_INPUTCHANNELPOLARITY_RISING);
    HAL_GPIO_WritePin(SR04_TRIG_PORT, SR04_TRIG_PIN, GPIO_PIN_SET);
    for (volatile uint32_t i = 0; i < 180; i++) { __NOP(); }
    HAL_GPIO_WritePin(SR04_TRIG_PORT, SR04_TRIG_PIN, GPIO_PIN_RESET);

    if (has_new) return 0;

    /* 无新数据时返回上次有效值，避免显示 -- */
    if (sr04_last_valid > 0) {
        *distance_cm = (float)sr04_last_valid / 58.0f;
        return 0;
    }
    return 1;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM3 || htim->Channel != HAL_TIM_ACTIVE_CHANNEL_4) return;

    uint32_t val = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4);

    if (sr04_rise == 0) {
        sr04_rise = val;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_4, TIM_INPUTCHANNELPOLARITY_FALLING);
    } else {
        sr04_width = (val > sr04_rise) ? (val - sr04_rise)
                                       : (0xFFFF - sr04_rise + val + 1);
        sr04_ready = 1;
        sr04_rise  = 0;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_4, TIM_INPUTCHANNELPOLARITY_RISING);
    }
}
