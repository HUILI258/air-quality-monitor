/**
 * @file    dht22.c
 * @brief   DHT22 单总线驱动实现（开漏模式全程复用，无需切换方向）
 * @note    STM32 开漏输出模式下仍可读取 IDR 寄存器获取引脚实际电平，
 *          因此全程保持 OUTPUT_OD 模式即可实现双向 1-Wire 通信，
 *          避免 HAL_GPIO_Init() 反复调用带来的竞态风险。
 */

#include "main.h"
#include "dht22.h"

#define DHT22_DELAY_US(us)  do { \
    for (volatile uint32_t _d = 0; _d < (us) * 12; _d++) { __NOP(); } \
} while(0)

void DHT22_Init(void)
{
    /* 全程开漏输出 + 上拉，读写都用这个模式 */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT22_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct);

    /* 总线空闲 = 释放（高） */
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
}

/**
 * @brief  读取一次 DHT22 数据（开漏模式，不加锁，偶尔失败可接受）
 */
uint8_t DHT22_Read(float *temperature, float *humidity)
{
    uint8_t data[5] = {0};
    uint16_t timeout;

    /* --- 1. 主机发送启动信号: 拉低 >1ms --- */
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET);
    DHT22_DELAY_US(2000);
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);   /* 释放总线 */
    DHT22_DELAY_US(30);

    /* --- 2. 等待 DHT22 响应（开漏模式下直接读 IDR） --- */
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
        if (++timeout > 500) return DHT22_ERR_TIMEO;
    }
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_RESET) {
        if (++timeout > 500) return DHT22_ERR_TIMEO;
    }
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
        if (++timeout > 500) return DHT22_ERR_TIMEO;
    }

    /* --- 3. 读取 40bit 数据 --- */
    for (uint8_t i = 0; i < 5; i++) {
        for (uint8_t j = 0; j < 8; j++) {
            timeout = 0;
            while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_RESET) {
                if (++timeout > 500) return DHT22_ERR_TIMEO;
            }
            DHT22_DELAY_US(40);                             /* 40us 后采样 */
            data[i] <<= 1;
            if (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
                data[i] |= 0x01;
            }
            timeout = 0;
            while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
                if (++timeout > 500) return DHT22_ERR_TIMEO;
            }
        }
    }

    /* --- 4. 校验 --- */
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return DHT22_ERR_CHECK;
    }

    /* --- 5. 解析 --- */
    uint16_t hum_raw = ((uint16_t)data[0] << 8) | data[1];
    uint16_t temp_raw = ((uint16_t)(data[2] & 0x7F) << 8) | data[3];

    *humidity = hum_raw / 10.0f;
    *temperature = (data[2] & 0x80) ? -(temp_raw / 10.0f) : (temp_raw / 10.0f);

    /* 释放总线 */
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);

    return DHT22_OK;
}
