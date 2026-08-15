# RA8P1 触摸遥控与无线图传工程说明

## 1. 工程定位

本工程运行在 Renesas RA8P1 上，使用 FreeRTOS、LVGL、GLCDC、GT911 和两颗 nRF24L01+，作为车辆的触摸遥控与图传显示终端。

当前正式数据流为：

```text
LVGL -> Command TX Thread -> SPI1 nRF24（76 信道） -> 小车控制接收端
LVGL main_img_1 <- Video RX Thread <- SPI0 nRF24（100 信道） <- 小车图传发送端
```

遥控器只发送控制命令，不接收小车控制命令；遥控器只接收小车图像，不通过 Video RX 链路发送数据。

历史版本在同一块板上通过 SPI1 nRF24 向 SPI0 nRF24 回环发送本地 OV5640 图像。相关摄像头、JPEG 编码和视频发送代码仍保留，用于后续诊断，但当前由 `src/app_config.h` 中的编译开关关闭：

```c
#define APP_VIDEO_RX_ENABLE              1U
#define APP_CAMERA_CAPTURE_ENABLE        0U
#define APP_LOCAL_VIDEO_LOOPBACK_ENABLE  0U
#define APP_VIDEO_RX_CONTROL_COMPAT_ENABLE 0U
```

## 2. 软件与硬件组成

| 模块 | 当前用途 |
|---|---|
| RA8P1 | 主控 MCU |
| FreeRTOS | 任务调度 |
| LVGL + GUI Guider | 1024×600 触摸界面 |
| GLCDC + Dave2D | LCD 输出和图形加速 |
| GT911 + IIC1 | 触摸输入 |
| SPI1 nRF24L01+ | 76 信道，向小车发送控制命令 |
| SPI0 nRF24L01+ | 100 信道，接收小车图像 |
| TJpgDec | 解码小车发送的 200×112 灰度 JPEG |
| SEGGER RTT | 运行日志和故障诊断 |
| OV5640 + MIPI CSI + VIN | 源码保留，当前不启动本地采集 |

## 3. FreeRTOS 任务职责

| 任务 | 优先级 | 栈 | 当前职责 |
|---|---:|---:|---|
| Display Thread | 4 | 16 KiB | `lv_timer_handler()`、GT911 轮询、刷新 `main_img_1` |
| Video RX Thread | 3 | 16 KiB | 系统初始化；100 信道图像接收、重组、CRC、JPEG 解码 |
| Command TX Thread | 2 | 16 KiB | 消费 LVGL 控制队列；76 信道发送控制包 |
| Touch Thread | 1 | 2 KiB | 预留；当前不访问 LVGL 或 I²C |

所有 LVGL API 只由 Display Thread 调用。触摸事件回调只把控制包写入 TX 软件环形队列，真正的 SPI/nRF24 发送由 Command TX Thread 完成。

## 4. 为什么 FSP Stack 在 `_hal.0`，不放在线程中

`configuration.xml` 中的 `stack` 指 FSP 模块依赖栈，不是 FreeRTOS 的任务栈内存。当前 SPI0、SPI1、I²C、GLCDC、IRQ、VIN 等实例位于 `_hal.0`，四个线程节点只记录任务名、优先级、任务栈大小和静态分配方式。

这样配置的原因是：

1. 这些外设是板级单例或跨线程共享资源。例如 LVGL 事件在 Display Thread 产生控制数据，而 SPI1 由 Command TX Thread 执行发送。
2. `_hal.0` 中的模块由 FSP 生成全局实例，例如 `g_spi0`、`g_spi1`、`g_display`，应用可以在明确同步规则下从不同任务调用。
3. 如果把同一个外设 Stack 分别放入多个 Thread，FSP 通常会生成线程私有实例或产生实例/中断/DMA 通道冲突。
4. 外设放在哪个 Thread 节点不等于驱动只能在哪个任务运行。运行时所有权应由任务入口、队列、互斥锁和初始化顺序定义。
5. 只有确实由单一线程独占、且希望由该线程自动初始化的模块，才适合挂在线程上下文中。

本工程继续保留全局 FSP Stack，但在应用层规定：SPI0 RX 归 Video RX Thread，SPI1 TX 归 Command TX Thread，LVGL/GT911 归 Display Thread。

## 5. 启动流程

1. FSP 生成的 `main()` 创建四个静态 FreeRTOS 任务。
2. Video RX Thread 优先执行统一初始化：IOPORT、SDRAM、LVGL Port、I²C/GT911、两颗 nRF24 和 GUI。
3. SPI0 nRF24 配置为 100 信道接收机；SPI1 nRF24 配置为 76 信道发射机。
4. 初始化完成后调用 `FreeRtosApp_NotifyInitialized()`。
5. Display、Command TX 和 Touch 任务解除等待，分别进入自己的循环。

本地摄像头功能关闭时不调用 `CameraCapture_Init()`，因此不会启动 GPT12 XCLK、OV5640、MIPI CSI 或 VIN。FSP XML 中暂时仍保留 VIN/MIPI Stack，方便以后重新启用和生成配置。

## 6. 控制链路

方向、RUN/STOP、速度、手动/自动、LED、风扇、Wi-Fi 和页面操作由 LVGL 事件产生。事件被编码成 8 字节控制包并写入 8 项软件 TX 队列。

Command TX Thread 周期性调用 `WirelessRadioTx_Service()`：

- 使用 SPI1 nRF24；
- 使用 76 信道和 2 Mbps；
- 使用硬件 ACK、2 字节 CRC 和自动重发；
- 发送失败时保留队头包，下次继续重试。

## 7. 视频链路

Video RX Thread 只处理 100 信道收到的图像协议包。非图像包直接拒绝并输出 RTT 日志，不再解析控制包。

图像处理顺序：

```text
nRF24 IRQ -> SPI0 硬件 FIFO -> 32 项软件 RX 队列
-> START/DATA/END 分片重组 -> CRC32
-> TJpgDec 解码 200×112 JPEG
-> 近邻放大为 480×272 RGB888
-> 双解码缓冲 -> LVGL 稳定展示缓冲
-> D-Cache Clean -> main_img_1 invalidate -> GLCDC 显示
```

接收端不再与本机发送缓冲做 `memcmp`，校验依据是包元数据、分片顺序、接收长度和 CRC32，因此可以接收真正来自小车端的图像。

## 8. 当前建议继续优化的项目

### 高优先级

1. **给视频协议增加丢帧统计和帧序号连续性统计。** 当前能发现坏帧，但缺少长期丢帧率和 FPS 指标。
2. **测量稳定图传吞吐与完整帧率。** 实机 ACK 回程持续触发 `MAX_RT=4`，当前视频改为 NO_ACK 冗余发送：边界包三发、DATA 双发并节流；小车端仍保留 NO_ACK 三包突发实现，待链路稳定后再评估分组 FEC 或选择性重发。
3. **增加控制心跳与失联保护。** 小车端应在一定时间未收到控制心跳时自动停车；方向 RELEASED 包也应具备最高安全优先级。
4. **控制队列合并。** 速度、模式等状态类命令可以保留最新值；方向/急停命令应允许抢占普通队列，防止队列满时丢失停车指令。
5. **将图像协议常量抽成双方共享头文件。** 文档已经统一协议，但共享 C 头文件更能避免遥控端和小车端字段漂移。

### 中优先级

1. 使用 Event Group 或 Task Notification 替代单个全局布尔初始化标志，并分别表示 GUI、TX Radio、RX Radio 是否就绪。
2. 给 SPI/I²C 增加明确的所有权断言或互斥策略，防止以后新增任务后发生并发访问。
3. 打开 FreeRTOS 栈溢出检测和 High Water Mark，按实测缩小三个 16 KiB 任务栈。
4. 将大图像缓冲继续放入 SDRAM，并记录 Cache Line 对齐规则；避免后续把 DMA 缓冲误放进普通 RAM。
5. 如果长期不使用本地摄像头，可建立独立的 `camera-loopback` FSP 配置，正式配置移除 VIN/MIPI Stack，从而减少生成代码和约 3.5 MiB VIN SDRAM 缓冲。

### 低优先级和维护性

1. 将 `hal_entry.c` 拆成 `app_init.c`、`video_rx_service.c`、`command_tx_service.c`、`image_protocol.c`，降低单文件复杂度。
2. 将 GUI Guider 生成代码和用户事件代码分离，避免重新生成界面时覆盖无线事件逻辑。
3. 修正历史任务命名和注释，确保 FSP Thread 名称、入口函数和实际职责一致。
4. 为控制包校验、图像分片、CRC32、超时丢帧建立主机端单元测试。

## 9. 当前编译配置结论

- 正式功能：触摸控制发送 + 小车图像接收显示。
- 本地 OV5640 采集：源码保留，条件编译关闭。
- 本机视频回环发送：源码保留，条件编译关闭。
- Video RX 接收控制包：历史解析代码保留但条件编译关闭，当前只接受图像协议。
- 无线发射功率：默认 0 dBm，已退出近距离环回的 -18 dBm 配置。
