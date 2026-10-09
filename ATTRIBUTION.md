# 致谢与来源

本项目整合自多个开源项目，特此列出各组成部分的来源与许可。

## 代码来源

| 组成部分 | 来源 | 许可 |
|----------|------|------|
| 工程骨架（FreeRTOS 任务框架、OLED/BH1750/Buzzer/LED/DHT22/HC-SR04/ESP8266 驱动、菜单） | [kennySzhucheng/EnvMonitor](https://github.com/kennySzhucheng/EnvMonitor) | 未声明（作者公开发布的练习项目，个人/学习使用；若商用请先联系作者） |
| SCD30 驱动（`Drivers/SCD30/`，自写实现） | 协议依据 [Sensirion SCD30 Datasheet](https://sensirion.com/products/catalog/SCD30)；参考 [cherifon/STM32_SDC30_CO2_Sensor](https://github.com/cherifon/STM32_SDC30_CO2_Sensor)（MIT）与 [jdtadeusz/System_pomiarowy_SCD30](https://github.com/jdtadeusz/System_pomiarowy_SCD30) | 本项目实现为 MIT |
| MAX9814 噪声驱动（`Drivers/Noise/`，自写实现） | 无外部依赖 | MIT |
| 综合指数（`App/aqi.*`，自写实现） | 无外部依赖 | MIT |
| STM32 HAL / CMSIS 库 | STMicroelectronics [STM32CubeF1](https://github.com/STMicroelectronics/STM32CubeF1) v1.8.0（HAL 1.1.10） | BSD-3-Clause（见各 `LICENSE.txt`） |
| FreeRTOS 内核 | [FreeRTOS](https://github.com/FreeRTOS/FreeRTOS) | MIT |

## 说明

- `stm32f1xx_hal_adc.c / _ex.c / .h / _ex.h` 是从 STM32CubeF1 v1.8.0 补齐的 ADC 驱动，版权归 STMicroelectronics，遵循其 BSD-3-Clause 许可（文件头保留 ST 原始版权声明）。
- 若你计划将本项目（含 EnvMonitor 骨架部分）商用分发，请核对并遵守各上游项目的许可要求。
