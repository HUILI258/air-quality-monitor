/**
 * @file    noise.c
 * @brief   MAX9814 噪声采集实现
 * @note    测量流程：
 *          1. 采样 NOISE_SAMPLES 次求直流均值（MAX9814 输出偏置）
 *          2. 再次采样，减去均值后求 RMS（交流噪声有效值）
 *          ADC 时钟 = PCLK2(72MHz)/6 = 12MHz，单次转换约 252 周期 ≈ 21us
 */

#include "noise.h"
#include "main.h"
#include <math.h>

static ADC_HandleTypeDef hnoise_adc;

/* ==================== 初始化 ==================== */
void Noise_Init(void)
{
    GPIO_InitTypeDef       gpio = {0};
    ADC_ChannelConfTypeDef ch   = {0};

    /* 时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();

    /* PA1 模拟输入 */
    gpio.Pin  = NOISE_ADC_PIN;
    gpio.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(NOISE_ADC_PORT, &gpio);

    /* ADC1 配置：单通道，软件触发 */
    hnoise_adc.Instance                   = ADC1;
    hnoise_adc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hnoise_adc.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hnoise_adc.Init.ContinuousConvMode    = DISABLE;
    hnoise_adc.Init.NbrOfConversion       = 1;
    hnoise_adc.Init.DiscontinuousConvMode = DISABLE;
    hnoise_adc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hnoise_adc.Init.NbrOfDiscConversion   = 0;
    if (HAL_ADC_Init(&hnoise_adc) != HAL_OK) {
        Error_Handler();
    }

    /* 通道 1，采样时间取最长以获得更稳定读数 */
    ch.Channel      = NOISE_ADC_CHANNEL;
    ch.Rank         = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hnoise_adc, &ch) != HAL_OK) {
        Error_Handler();
    }

    /* ADC 自校准（F1 推荐，提高精度） */
    HAL_ADCEx_Calibration_Start(&hnoise_adc);
}

/* ==================== 单次采样 ==================== */
static uint16_t Noise_SampleOnce(void)
{
    uint16_t v = 0;
    HAL_ADC_Start(&hnoise_adc);
    if (HAL_ADC_PollForConversion(&hnoise_adc, 10) == HAL_OK) {
        v = (uint16_t)HAL_ADC_GetValue(&hnoise_adc);
    }
    HAL_ADC_Stop(&hnoise_adc);   /* 复位状态机，下次 Start 干净触发 */
    return v;
}

/* ==================== RMS 计算 ==================== */
float Noise_ReadRMS(void)
{
    uint32_t i;
    float    sum  = 0.0f;
    float    mean = 0.0f;
    float    acc  = 0.0f;

    /* 第一遍：求直流均值 */
    for (i = 0; i < NOISE_SAMPLES; i++) {
        sum += (float)Noise_SampleOnce();
    }
    mean = sum / (float)NOISE_SAMPLES;

    /* 第二遍：求交流 RMS */
    for (i = 0; i < NOISE_SAMPLES; i++) {
        float d = (float)Noise_SampleOnce() - mean;
        acc += d * d;
    }

    return sqrtf(acc / (float)NOISE_SAMPLES);
}

/* ==================== 分贝 / 指数 ==================== */
float Noise_ReadDB(void)
{
    float rms = Noise_ReadRMS();
    if (rms < 1.0f) {
        rms = 1.0f;
    }
    /* 相对 dB，参考 1 LSB；量程约 0 ~ 66 dB（未标定） */
    return 20.0f * log10f(rms);
}

uint8_t Noise_ReadIndex(void)
{
    float rms = Noise_ReadRMS();
    float idx = (rms / 2000.0f) * 100.0f;   /* 约 2000 LSB 视作满量程 */
    if (idx > 100.0f) idx = 100.0f;
    if (idx < 0.0f)    idx = 0.0f;
    return (uint8_t)idx;
}
