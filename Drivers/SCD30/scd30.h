/**
 * @file    scd30.h
 * @brief   Sensirion SCD30 二氧化碳/温湿度传感器驱动 (I2C)
 * @note    协议依据 Sensirion SCD30 官方 Datasheet 实现
 *          - I2C 地址 0x61 (7-bit)
 *          - 每条命令 2 字节(大端)，写参数时参数后跟 1 字节 CRC8
 *          - 读回数据每组 2 字节 + 1 字节 CRC8
 *          - 本驱动无 HAL_Delay，可直接在 FreeRTOS 任务中调用
 *          - 与 BH1750 / OLED 共用 I2C1，调用前需持有 I2C 互斥量
 */

#ifndef SCD30_H
#define SCD30_H

#include "stm32f1xx_hal.h"

/* 8-bit 写地址 = (0x61 << 1) = 0xC2，HAL I2C 要求左移后的地址 */
#define SCD30_I2C_ADDR              (0x61U << 1)

/* 测量间隔：2s（SCD30 允许 2~1800s），2s 最灵敏 */
#define SCD30_MEASUREMENT_INTERVAL  2U

typedef struct {
    float co2;          /* 二氧化碳浓度，ppm */
    float temperature;  /* 温度，°C */
    float humidity;     /* 相对湿度，%RH */
} SCD30_Measurement_t;

/* 初始化：ping + 读固件版本 + 设测量间隔 + 启动连续测量 */
HAL_StatusTypeDef SCD30_Init(I2C_HandleTypeDef *hi2c);

/* 启动连续测量（pressure_mbar=0 表示使用环境气压补偿） */
HAL_StatusTypeDef SCD30_StartMeasurement(I2C_HandleTypeDef *hi2c, uint16_t pressure_mbar);

/* 停止测量 */
HAL_StatusTypeDef SCD30_StopMeasurement(I2C_HandleTypeDef *hi2c);

/* 设置测量间隔（2~1800s） */
HAL_StatusTypeDef SCD30_SetMeasurementInterval(I2C_HandleTypeDef *hi2c, uint16_t interval_s);

/* 查询是否有新数据就绪（*ready: 1=就绪） */
HAL_StatusTypeDef SCD30_IsDataReady(I2C_HandleTypeDef *hi2c, uint8_t *ready);

/* 读取一次测量结果（带 CRC8 校验，失败返回 HAL_ERROR） */
HAL_StatusTypeDef SCD30_ReadMeasurement(I2C_HandleTypeDef *hi2c, SCD30_Measurement_t *m);

#endif /* SCD30_H */
