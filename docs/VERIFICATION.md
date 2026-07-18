# 验证说明

> 状态日期：2026-07-18

## 一键本地门禁

```bash
bash scripts/verify.sh
```

脚本会在临时副本中执行：

1. 敏感信息、本机绝对路径、凭据、Keil/生成物、压缩包、二进制和未知类型检查；
2. 精确公开源文件 allowlist、必需文件、BOM、SVG、固定构建依赖和状态边界检查；
3. 不依赖硬件的源码契约测试；
4. PlatformIO `safe-default` 干净构建；
5. `fan-output-opt-in` 只做编译路径覆盖；
6. 构建后再次扫描临时副本，并确认候选没有生成物。

它不会读取历史源目录、烧录 STM32、打开串口、采样传感器、驱动 GPIO 或控制风扇。

## 固定环境

```text
PlatformIO Core 6.1.19
platformio/ststm32 19.5.0
Board genericSTM32F103C8
Framework STM32Cube
STM32CubeF1 1.8.6
GNU Arm toolchain package 1.70201.0
```

## 当前本地门禁结果

2026-07-18 已在当前候选内容上完整运行 `scripts/verify.sh`：

```text
secret/path scan: PASS
source manifest: PASS (15 files)
repository check: PASS (40 exact public files)
scanner/source contracts: 15 / 15 PASS
safe-default PlatformIO build: PASS
fan-output-opt-in compile path: PASS
```

构建资源占用：

```text
safe-default:       RAM 1,072 / 20,480 bytes (5.2%), Flash 4,592 / 65,536 bytes (7.0%)
fan-output-opt-in:  RAM 1,072 / 20,480 bytes (5.2%), Flash 4,776 / 65,536 bytes (7.3%)
```

这些数字只是编译器对构建目标的静态结果，不证明实物 Flash/RAM 容量、运行时栈、烧录、传感器、显示或风扇行为。GitHub exact-HEAD Actions 仍要在推送后实际回读。

## 当前真机复测清单（尚未执行）

后续复测必须记录日期、完整 commit、执行者、精确板卡/模块/驱动/风扇、供电和每项通过/失败/未测：

- [ ] 核对 STM32F103 板型、Flash、HSE、3.3 V 供电和 SWD；
- [ ] 默认固件烧录、冷启动、复位、断电重启，PB0/PB1 保持未驱动；
- [ ] USART1 115200 启动日志和持续状态日志；
- [ ] DHT11 供电、外部上拉、PB12 时序、校验和、断线和异常值；
- [ ] 使用逻辑分析仪核对 DHT11 0/1 脉宽和当前 `50 us` 分界；
- [ ] MQ135 模块准确型号、加热器供电、预热时间、AO 最大电压、分压/缓冲和 PA0 安全范围；
- [ ] 用已知电压源核对 ADC 原始值，确认 16 次平均和 `demo_index` 只作显示；
- [ ] OLED 控制器、供电、地址、PB6/PB7 上拉、启动探测和全部字符显示；
- [ ] 湿度/MQ 阈值、滞回、DHT 失败及 hold 分支；
- [ ] 精确风扇驱动器真值表、外部下拉、独立供电、保护和空载台架；
- [ ] 仅在前项确认后烧录 opt-in 固件，验证启动测试、开/关方向和紧急断电；
- [ ] 连续运行 30–60 分钟，记录复位、阻塞、读数、温升、电流和执行器异常；
- [ ] 照片、视频、日志去除位置、设备标识、私人环境和 EXIF/GPS。

完成相应项目之前，README、GitHub、Hardware Lab 与 Profile 必须保持“当前真机复测：未执行”。
