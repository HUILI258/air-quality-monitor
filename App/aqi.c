/**
 * @file    aqi.c
 * @brief   室内环境综合指数实现
 * @note    权重分配（可调）：
 *            CO2      0.5   —— 400ppm→0，2000ppm→100
 *            噪声     0.3   —— 20dB→0，65dB→100（相对值）
 *            温度舒适  0.1   —— 最适 20~26°C
 *            湿度舒适  0.1   —— 最适 40~60%RH
 *          无效传感器（负值）自动剔除并重新归一化权重
 */

#include "aqi.h"

/* ==================== 可调参数 ==================== */
#define AQI_W_CO2     0.5f
#define AQI_W_NOISE   0.3f
#define AQI_W_TEMP    0.1f
#define AQI_W_HUMI    0.1f

#define CO2_PPM_MIN    400.0f
#define CO2_PPM_MAX    2000.0f
#define NOISE_DB_MIN   20.0f
#define NOISE_DB_MAX   65.0f
#define TEMP_OPT_LOW   20.0f
#define TEMP_OPT_HIGH  26.0f
#define HUMI_OPT_LOW   40.0f
#define HUMI_OPT_HIGH  60.0f

/* ==================== 内部工具 ==================== */
static float AQI_Clamp(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* 越接近最优区间中心越舒适，0~100 */
static float AQI_TempComfort(float t)
{
    float center = (TEMP_OPT_LOW + TEMP_OPT_HIGH) / 2.0f;   /* 23°C */
    float half   = (TEMP_OPT_HIGH - TEMP_OPT_LOW) / 2.0f;   /* 3°C */
    float d = t - center;
    if (d < 0) d = -d;
    return AQI_Clamp((d - half) / 15.0f * 100.0f, 0.0f, 100.0f);
}

static float AQI_HumiComfort(float h)
{
    if (h < HUMI_OPT_LOW)  return AQI_Clamp((HUMI_OPT_LOW - h) / HUMI_OPT_LOW * 100.0f, 0.0f, 100.0f);
    if (h > HUMI_OPT_HIGH) return AQI_Clamp((h - HUMI_OPT_HIGH) / HUMI_OPT_HIGH * 100.0f, 0.0f, 100.0f);
    return 0.0f;
}

/* ==================== 综合指数 ==================== */
float AQI_Compute(float co2_ppm, float temperature, float humidity, float noise_db)
{
    float total = 0.0f;
    float wsum  = 0.0f;

    if (co2_ppm >= 0.0f) {
        float sub = AQI_Clamp((co2_ppm - CO2_PPM_MIN) / (CO2_PPM_MAX - CO2_PPM_MIN) * 100.0f,
                              0.0f, 100.0f);
        total += AQI_W_CO2 * sub;
        wsum  += AQI_W_CO2;
    }

    if (noise_db >= 0.0f) {
        float sub = AQI_Clamp((noise_db - NOISE_DB_MIN) / (NOISE_DB_MAX - NOISE_DB_MIN) * 100.0f,
                              0.0f, 100.0f);
        total += AQI_W_NOISE * sub;
        wsum  += AQI_W_NOISE;
    }

    if (temperature > -900.0f) {
        total += AQI_W_TEMP * AQI_TempComfort(temperature);
        wsum  += AQI_W_TEMP;
    }

    if (humidity > -900.0f) {
        total += AQI_W_HUMI * AQI_HumiComfort(humidity);
        wsum  += AQI_W_HUMI;
    }

    if (wsum <= 0.0f) {
        return 0.0f;   /* 所有传感器都无效 */
    }
    return AQI_Clamp(total / wsum, 0.0f, 100.0f);
}

/* ==================== 等级映射 ==================== */
AQI_Level_t AQI_Level(float aqi)
{
    if (aqi < 50.0f) return AQI_LEVEL_EXCELLENT;
    if (aqi < 75.0f) return AQI_LEVEL_GOOD;
    if (aqi < 90.0f) return AQI_LEVEL_MODERATE;
    return AQI_LEVEL_POOR;
}

const char *AQI_LevelStr(AQI_Level_t level)
{
    /* 注意：OLED 为 ASCII 5x7 字库，无法显示中文，故用英文等级 */
    switch (level) {
        case AQI_LEVEL_EXCELLENT: return "Excellent";  /* 优 */
        case AQI_LEVEL_GOOD:      return "Good";       /* 良 */
        case AQI_LEVEL_MODERATE:  return "Fair";       /* 中 */
        case AQI_LEVEL_POOR:      return "Poor";       /* 差 */
        default:                  return "?";
    }
}
