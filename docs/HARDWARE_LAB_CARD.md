# Hardware Lab 索引卡片

```yaml
name: 基于 STM32F103 的衣柜环境监测与自动通风控制系统
platform: STM32F103 · C · PlatformIO · STM32Cube · DHT11 · MQ135 · OLED
summary: 读取温湿度和 MQ135 模块原始 ADC 值，通过 OLED/USART1 展示，并用带滞回阈值形成通风控制意图的低压教学原型。
status: 来源边界已确认 · 安全默认、公开门禁与 GitHub Actions 已建立 · 最新候选仍须 exact-HEAD CI 成功 · 当前整机尚未重新真机复测
media_scope: 当前没有实物照片、演示视频、原理图、PCB、EDA、Gerber 或制造文件；只公开 BOM、接口边界图、来源和验证说明。
known_boundaries:
  - MQ 原始 ADC 与 demo_index 不是空气质量或气体浓度，阈值不是健康标准。
  - 默认不把 PB0/PB1 配置为输出，不执行历史启动风扇测试。
  - PB0/PB1 不得直接驱动电机；MQ 模块模拟输出必须先核对 3.3 V ADC 范围。
  - 构建与源码契约不证明传感器、OLED、串口、风扇、接线或电气安全。
```

- **历史媒体 / EDA：** 已加入经脱敏的历史衍生材料；范围和版本差异见 [`MEDIA_EVIDENCE.md`](MEDIA_EVIDENCE.md)。它们不证明当前公开提交已完成真机复测。
