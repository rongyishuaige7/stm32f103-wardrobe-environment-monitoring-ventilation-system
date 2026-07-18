# 第三方声明

仓库中的 Rongyi 原创应用源码、最小显示字形、文档和脚本使用根目录 [MIT License](LICENSE)。构建依赖由 PlatformIO 单独下载，本仓不复制历史 Keil 工具链文件、CMSIS/设备头、启动文件、构建输出、数据手册、厂商 Logo 或来源不明的完整字体表。

## PlatformIO Core

- 上游：<https://github.com/platformio/platformio-core>
- 已验证版本：6.1.19
- 许可证：Apache-2.0
- 用途：包管理与构建编排

## PlatformIO ST STM32

- 上游：<https://github.com/platformio/platform-ststm32>
- 固定版本：19.5.0
- 许可证：Apache-2.0
- 用途：STM32 平台、板卡和工具包解析

## STM32CubeF1

- 上游：STMicroelectronics
- PlatformIO 包版本：1.8.6
- 用途：STM32F1 HAL、CMSIS、系统与启动支持
- 许可：以 PlatformIO 下载包中随附的 ST 许可证文件为准

## GNU Arm Embedded Toolchain

- PlatformIO 包：`toolchain-gccarmnoneeabi` 1.70201.0
- 用途：C 编译、链接与二进制生成
- 许可：以工具链各组件随附许可证为准

STM32、DHT11、MQ135、SSD1306、Keil、Arm、PlatformIO 等名称及商标属于各自权利人。列出兼容接口不表示厂商背书，也不确认历史实物使用了某个精确型号。
