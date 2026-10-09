/**
 * @file    aqi.h
 * @brief   室内环境综合指数（AQI）计算
 * @note    将 CO2、噪声、温湿度舒适度加权综合为 0~100 的指数
 *          - 各因子权重与阈值均为可调宏，便于二次开发标定
 */

#ifndef AQI_H
#define AQI_H

#include <stdint.h>

/* 等级划分 */
typedef enum {
    AQI_LEVEL_EXCELLENT = 0,  /* 优   0 ~ 50  */
    AQI_LEVEL_GOOD,           /* 良   50 ~ 75 */
    AQI_LEVEL_MODERATE,       /* 中   75 ~ 90 */
    AQI_LEVEL_POOR            /* 差   90 ~ 100 */
} AQI_Level_t;

/**
 * @brief 计算综合环境指数
 * @param co2_ppm     CO2 浓度(ppm)，<0 表示无效
 * @param temperature 温度(°C)，<-900 表示无效
 * @param humidity    湿度(%RH)，<-900 表示无效
 * @param noise_db    相对噪声(dB)，<0 表示无效
 * @return 0~100 综合指数
 */
float AQI_Compute(float co2_ppm, float temperature, float humidity, float noise_db);

/* 指数 → 等级 */
AQI_Level_t  AQI_Level(float aqi);

/* 等级 → 中文描述 */
const char  *AQI_LevelStr(AQI_Level_t level);

#endif /* AQI_H */
