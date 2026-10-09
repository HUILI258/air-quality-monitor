/**
 * @file    noise.h
 * @brief   MAX9814 麦克风噪声采集驱动 (ADC)
 * @note    MAX9814 是带 AGC 的驻极体麦克风放大器，模拟输出，直流偏置约 Vcc/2
 *          - 输出接 PA1 (ADC1_IN1)
 *          - 采样后去除直流分量求 RMS，得到交流噪声有效值
 *          - 结果未经声压级(SPL)标定，是相对值，用于阈值比较已足够
 */

#ifndef NOISE_H
#define NOISE_H

#include "stm32f1xx_hal.h"

/* MAX9814 输出引脚：PA1 = ADC1_IN1（避开已占用的 PA0/DHT22、PA4-6/按键、PA9-10/UART） */
#define NOISE_ADC_PORT      GPIOA
#define NOISE_ADC_PIN       GPIO_PIN_1
#define NOISE_ADC_CHANNEL   ADC_CHANNEL_1
#define NOISE_SAMPLES       256U   /* 采样点数，越大越平滑 */

void  Noise_Init(void);          /* 配置 GPIO + ADC1 */
float Noise_ReadRMS(void);       /* 返回交流噪声 RMS（ADC 原始 LSB 单位，0~4095） */
float Noise_ReadDB(void);        /* 相对分贝值 = 20*log10(RMS)，参考 1 LSB */
uint8_t Noise_ReadIndex(void);   /* 0~100 噪声指数（线性映射） */

#endif /* NOISE_H */
