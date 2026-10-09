# STM32F103 室内空气质量监测终端

基于 **STM32F103C8T6 + HAL 库 + FreeRTOS** 的室内环境监测系统，采集 **CO₂、温湿度、光照、噪声** 四类参数，OLED 实时显示，阈值超标时声光报警，并计算**综合环境指数（AQI）**。

> 本项目由 [EnvMonitor](https://github.com/kennySzhucheng/EnvMonitor) 骨架扩展而来，新增 SCD30(CO₂)、MAX9814(噪声) 驱动与综合指数计算。详见 [ATTRIBUTION.md](ATTRIBUTION.md)。

---

## ✨ 功能

| 模块 | 说明 |
|------|------|
| 🌬️ **SCD30** | CO₂ 浓度(ppm) + 温湿度，I²C 接口，带 CRC8 校验 |
| ☀️ **BH1750** | 环境光照强度(lux)，I²C 接口 |
| 🎤 **MAX9814** | 环境噪声，模拟输出 → ADC 采样 → RMS → 相对 dB |
| 🖥️ **OLED SSD1306** | 0.96 寸 I²C 屏，5 页菜单：主页/详情/设置/WiFi/关于 |
| 🚨 **三色 LED** | 红=CO₂/AQI/高温，黄=噪声/低湿，绿=有人靠近 |
| 🔔 **蜂鸣器** | 报警条件上升沿触发，队列异步播放，不阻塞 |
| 📊 **综合指数 AQI** | CO₂/噪声/温湿度加权，0~100 + 优/良/中/差等级 |
| 🌡️ **DHT22(备选)** | SCD30 失效时回退提供温湿度 |

---

## 🔌 硬件接线

| 外设 | 引脚 | 说明 |
|------|------|------|
| SCD30 | SCL=PB6, SDA=PB7 | 与 BH1750/OLED 共用 I²C1(100kHz) |
| BH1750 | SCL=PB6, SDA=PB7 | 地址 0x23(ADDR 接地) |
| OLED SSD1306 | SCL=PB6, SDA=PB7 | 地址 0x3C |
| MAX9814 输出 | **PA1** | ADC1_IN1，模拟输入 |
| DHT22 | PA0 | 1-Wire（备选） |
| HC-SR04 | Trig=PB0, Echo=PB1 | 测距 |
| LED 红/黄/绿 | PB12/PB13/PB14 | 高电平点亮 |
| 蜂鸣器 | PB15 | 有源蜂鸣器 |
| 按键 | PA4/PA5/PA6 | 页面切换/确认/返回 |
| ESP8266 | PA9/PA10 | UART1，上云(可选) |

> ⚠️ SCD30、BH1750、OLED 三设备共用一根 I²C 总线，务必接上拉电阻（4.7kΩ 至 VCC），并确保地址不冲突。

---

## 🏗️ 软件架构

```
App/                应用层任务与逻辑
  task_sensor.c     采集任务(5s)：SCD30+BH1750(持 I2C 锁) + MAX9814 + AQI
  task_alarm.c      报警任务(500ms)：阈值判断 + LED + 蜂鸣器(边沿触发)
  task_display.c    显示任务(1s)：OLED 菜单渲染
  task_buzzer.c     蜂鸣器任务：队列驱动
  aqi.c/h           综合环境指数计算
Drivers/
  SCD30/scd30.c     自写 SCD30 驱动（HAL I²C + CRC8）
  Noise/noise.c     MAX9814 噪声驱动（ADC1 + RMS）
  BH1750/ OLED/ Buzzer/ LED/ DHT22/ HC_SR04/ ESP8266 ...
Core/               main.c / main.h / i2c.c / gpio.c（CubeMX 生成）
MDK-ARM/            Keil MDK 工程 EnvMonitor.uvprojx
```

**并发保护**：`I2C_Mutex`（OLED/BH1750/SCD30 共用总线）、`Data_Mutex`（全局数据）、`UART_Mutex`。

---

## 🔧 编译烧录

1. 安装 **Keil MDK-ARM**（≥5.0）+ **STM32F1xx_DFP** 器件包（工程里用的 `Keil.STM32F1xx_DFP.2.2.0`）。
2. 打开 `MDK-ARM/EnvMonitor.uvprojx`。
3. 编译器为 **AC5(ARMCC)**，C99，`USE_HAL_DRIVER, STM32F103xB` 已配置。
4. 用 ST-Link 烧录（工程已选 ST-LINK 下载算法）。

> 本工程新增了 `stm32f1xx_hal_adc.c / adc_ex.c`（从 STM32CubeF1 v1.8.0 补齐，HAL 1.1.10），并已启用 `HAL_ADC_MODULE_ENABLED`。

---

## 📐 标定与二次开发

- **噪声 dB 是相对值**（`Noise_ReadDB = 20·log10(RMS)`，参考 1 LSB），未经声压级标定。若需真实 dBSPL，用标准声压计在固定距离标定后，在 `noise.c` 里调整偏移即可。
- **AQI 权重**在 `App/aqi.c` 顶部宏里，可自行调整：
  - CO₂ 0.5（400ppm→0，2000ppm→100）
  - 噪声 0.3（20dB→0，65dB→100）
  - 温度舒适 0.1、湿度舒适 0.1
- **报警阈值**在 `Core/Inc/main.h`：`ALARM_CO2_HIGH`、`ALARM_NOISE_HIGH`、`ALARM_AQI_HIGH` 等。
- **ADC 与 PA1** 在 `Drivers/Noise/noise.c` 里手工初始化，不走 CubeMX。若你用 CubeMX 重新生成代码，需在 .ioc 里手动加 ADC1 通道 1(PA1)，否则会被覆盖。

---

## 📤 上传到你的 GitHub

本仓库是本地工程，尚未推送。上传前先在本机终端认证（任选其一）：

```bash
# 方式一：交互式登录（推荐，token 不进聊天记录）
gh auth login

# 方式二：浏览器授权（需已装 Git Credential Manager，Edge 已登录 GitHub 时可用）
# git push 时按提示在浏览器点授权
```

然后创建远程仓库并推送：

```bash
cd STM32F103_AirQualityMonitor
git init
git add -A
git commit -m "STM32F103 indoor air quality monitor (SCD30+BH1750+MAX9814+OLED)"
git branch -M main
git remote add origin https://github.com/HUILI258/<你的仓库名>.git
git push -u origin main
```

---

## 📜 许可证与致谢

- 新写的驱动（`SCD30/`、`Noise/`、`App/aqi.*`）与整合代码：MIT。
- 骨架与其余驱动来自 [EnvMonitor](https://github.com/kennySzhucheng/EnvMonitor)。
- SCD30 协议依据 Sensirion 官方 Datasheet 实现，并参考 cherifon(MIT)、jdtadeusz 的 SCD30 驱动。
- ST HAL/CMSIS 库版权归 STMicroelectronics（BSD-3-Clause）。

详见 [ATTRIBUTION.md](ATTRIBUTION.md)。
