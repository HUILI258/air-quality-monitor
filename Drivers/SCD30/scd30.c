/**
 * @file    scd30.c
 * @brief   Sensirion SCD30 驱动实现
 * @note    协议要点：
 *          - 写命令(带参数): [CMD_MSB CMD_LSB ARG_MSB ARG_LSB CRC8(ARG)]
 *          - 写命令(无参数): [CMD_MSB CMD_LSB]
 *          - 读回: 每组 2 字节数据 + 1 字节 CRC8
 *          - 浮点数均为 IEEE754 大端序，小端 MCU 需字节反转
 *          - SCD30 支持 I2C 时钟拉伸，读操作会等待数据就绪，无需显式延时
 */

#include "scd30.h"
#include <string.h>

/* ==================== SCD30 命令集 ==================== */
#define SCD30_CMD_TRIGGER_CONT_MEAS   0x0010U  /* 启动连续测量 */
#define SCD30_CMD_STOP_MEAS           0x0104U  /* 停止测量 */
#define SCD30_CMD_SET_INTERVAL        0x4600U  /* 设置测量间隔 */
#define SCD30_CMD_GET_DATA_READY      0x0202U  /* 查询数据就绪状态 */
#define SCD30_CMD_READ_MEASUREMENT    0x0300U  /* 读取测量结果 */
#define SCD30_CMD_SOFT_RESET          0xD304U  /* 软复位 */
#define SCD30_CMD_FW_VERSION          0xD100U  /* 读固件版本 */

/* ==================== 内部函数 ==================== */

/* CRC8：多项式 0x31 (x^8+x^5+x^4+1)，初值 0xFF，无反射 */
static uint8_t SCD30_CRC8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/* 发送命令（arg 为参数，可为 NULL；arg_len 为参数字节数，通常 2） */
static HAL_StatusTypeDef SCD30_WriteCommand(I2C_HandleTypeDef *hi2c, uint16_t cmd,
                                             const uint8_t *arg, uint8_t arg_len)
{
    uint8_t buf[5];
    uint8_t len = 0;

    buf[len++] = (uint8_t)(cmd >> 8);
    buf[len++] = (uint8_t)(cmd & 0xFF);

    if (arg != NULL && arg_len > 0) {
        for (uint8_t i = 0; i < arg_len; i++) {
            buf[len++] = arg[i];
        }
        buf[len++] = SCD30_CRC8(arg, arg_len);
    }

    if (HAL_I2C_Master_Transmit(hi2c, SCD30_I2C_ADDR, buf, len, HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}

/* 发送命令并读回 word_count 个字（每字 3 字节：2 数据 + 1 CRC，共 word_count*3 字节） */
static HAL_StatusTypeDef SCD30_ReadWords(I2C_HandleTypeDef *hi2c, uint16_t cmd,
                                         uint8_t *rx, uint8_t word_count)
{
    uint8_t cmd_buf[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };

    if (HAL_I2C_Master_Transmit(hi2c, SCD30_I2C_ADDR, cmd_buf, 2, HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }

    if (HAL_I2C_Master_Receive(hi2c, SCD30_I2C_ADDR, rx, (uint16_t)(word_count * 3),
                               HAL_MAX_DELAY) != HAL_OK) {
        return HAL_ERROR;
    }

    /* 校验每组 CRC8 */
    for (uint8_t w = 0; w < word_count; w++) {
        uint8_t *p = &rx[w * 3];
        if (p[2] != SCD30_CRC8(p, 2)) {
            return HAL_ERROR;  /* CRC 校验失败 */
        }
    }
    return HAL_OK;
}

/* 大端 4 字节 → 小端 float */
static float SCD30_DecodeFloat(const uint8_t *p)
{
    uint32_t raw = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
                   ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
    float f;
    memcpy(&f, &raw, sizeof(f));
    return f;
}

/* ==================== 对外接口 ==================== */

HAL_StatusTypeDef SCD30_StartMeasurement(I2C_HandleTypeDef *hi2c, uint16_t pressure_mbar)
{
    uint8_t arg[2] = { (uint8_t)(pressure_mbar >> 8), (uint8_t)(pressure_mbar & 0xFF) };
    return SCD30_WriteCommand(hi2c, SCD30_CMD_TRIGGER_CONT_MEAS, arg, 2);
}

HAL_StatusTypeDef SCD30_StopMeasurement(I2C_HandleTypeDef *hi2c)
{
    return SCD30_WriteCommand(hi2c, SCD30_CMD_STOP_MEAS, NULL, 0);
}

HAL_StatusTypeDef SCD30_SetMeasurementInterval(I2C_HandleTypeDef *hi2c, uint16_t interval_s)
{
    uint8_t arg[2] = { (uint8_t)(interval_s >> 8), (uint8_t)(interval_s & 0xFF) };
    return SCD30_WriteCommand(hi2c, SCD30_CMD_SET_INTERVAL, arg, 2);
}

HAL_StatusTypeDef SCD30_IsDataReady(I2C_HandleTypeDef *hi2c, uint8_t *ready)
{
    uint8_t rx[3];
    if (SCD30_ReadWords(hi2c, SCD30_CMD_GET_DATA_READY, rx, 1) != HAL_OK) {
        return HAL_ERROR;
    }
    uint16_t status = ((uint16_t)rx[0] << 8) | rx[1];
    *ready = (status == 1U) ? 1U : 0U;
    return HAL_OK;
}

HAL_StatusTypeDef SCD30_ReadMeasurement(I2C_HandleTypeDef *hi2c, SCD30_Measurement_t *m)
{
    uint8_t rx[18];  /* 3 个浮点数 × (2 数据 + 1 CRC) */

    if (SCD30_ReadWords(hi2c, SCD30_CMD_READ_MEASUREMENT, rx, 6) != HAL_OK) {
        return HAL_ERROR;
    }

    /* rx: CO2 [0..3] 跳过 CRC[4] / Temp [6..9] 跳过 CRC[10] / Hum [12..15] 跳过 CRC[16] */
    m->co2         = SCD30_DecodeFloat(&rx[0]);
    m->temperature = SCD30_DecodeFloat(&rx[6]);
    m->humidity    = SCD30_DecodeFloat(&rx[12]);
    return HAL_OK;
}

HAL_StatusTypeDef SCD30_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t fw[3];
    uint16_t version;

    /* 1. 探测传感器是否在线 */
    if (HAL_I2C_IsDeviceReady(hi2c, SCD30_I2C_ADDR, 3, 50) != HAL_OK) {
        return HAL_ERROR;
    }

    /* 2. 读固件版本，验证通信 */
    if (SCD30_ReadWords(hi2c, SCD30_CMD_FW_VERSION, fw, 1) != HAL_OK) {
        return HAL_ERROR;
    }
    version = ((uint16_t)fw[0] << 8) | fw[1];
    (void)version;

    /* 3. 设置测量间隔 */
    if (SCD30_SetMeasurementInterval(hi2c, SCD30_MEASUREMENT_INTERVAL) != HAL_OK) {
        return HAL_ERROR;
    }

    /* 4. 启动连续测量 */
    if (SCD30_StartMeasurement(hi2c, 0) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}
