# M33 双 NRF 无线子系统移植说明

## 1. 职责与单向依赖

```text
Command RX Thread -> command_radio -> control_protocol -> Vehicle command mailbox
                         |
                         v
                    nrf24 driver
                         ^
                         |
                  FSP SPI0/IRQ port

IPC Thread -> video_frame_mailbox -> Video TX Thread -> video_radio -> video_protocol
                                                        |
                                                        v
                                                   nrf24 driver
                                                        ^
                                                        |
                                                 FSP SPI1 port
```

- `driver/`：从手持控制器 `gui` 分支移植的通用 nRF24L01+ 驱动，不包含 FSP/RTOS。
- `platform/`：RA8P1 SPI、GPIO、外部 IRQ 与 FreeRTOS 信号量适配。
- `protocol/`：8 字节遥控协议和 32 字节图像分包协议。
- `application/`：两条无线链路的初始化与业务编排。
- `adapters/rtos/`：IPC 与 Video TX Thread 之间的零拷贝帧邮箱。

两块 NRF 不共享 SPI 对象，也不需要互斥锁：Command RX Thread 独占 SPI0，
Video TX Thread 独占 SPI1。

## 2. 巡检车 M33 引脚

| 链路 | NRF信号 | RA8P1引脚 | FSP功能 |
|---|---|---|---|
| 命令接收 | MISO | P700 | SPI0 MISO0 |
| 命令接收 | MOSI | P701 | SPI0 MOSI0 |
| 命令接收 | SCK | P702 | SPI0 RSPCK0 |
| 命令接收 | CSN | P703 | SPI0 SSLA0 |
| 命令接收 | CE | P704 | GPIO输出，初始低 |
| 命令接收 | IRQ | P705 | IRQ19，输入上拉、下降沿 |
| 图传发送 | MISO | P100 | SPI1 MISO1 |
| 图传发送 | MOSI | P101 | SPI1 MOSI1 |
| 图传发送 | SCK | P102 | SPI1 RSPCK1 |
| 图传发送 | CSN | P103 | SPI1 SSLB0 |
| 图传发送 | CE | P104 | GPIO输出，初始低 |
| 图传发送 | IRQ | 不接 | 当前发送策略不需要IRQ |

选择 P100～P104 是因为手持端原 SPI1 使用的 P709 在巡检车 CPU0 上已经作为
OV5640 RESET 使用，不能照搬。上述引脚在当前 CPU0 配置中均未占用；焊接前仍须
对照你的 PCB 网络名确认物理连接。

## 3. configuration.xml 已修改项

- SPI0：通道0、8 MHz、Mode 0、回调 `nrf24_command_spi_callback`。
- SPI1：通道1、8 MHz、Mode 0、回调 `nrf24_video_spi_callback`。
- 16 MHz 已降为8 MHz，因为 nRF24L01+ SPI时钟上限为10 MHz。
- `g_external_irq0`：IRQ19、下降沿、IPL12，回调 `nrf24_command_irq_callback`。
- 删除尚未使用且与IRQ通道重复的 `g_external_irq1` 占位实例。
- 添加 P700～P705、P100～P104 的 SPI/GPIO/IRQ pin configuration。
- 开启 FreeRTOS 栈溢出检查和 `uxTaskGetStackHighWaterMark()`。

线程参数：

| 线程 | 优先级 | 栈 | 原因 |
|---|---:|---:|---|
| Vehicle | 7 | 3072 B | 车辆执行与安全状态最高 |
| Command RX | 6 | 2048 B | 遥控低延迟，收到后只投递命令 |
| IPC | 5 | 2048 B | 尽快消费M85描述符并释放共享资源 |
| Video TX | 4 | 3072 B | 持续吞吐，但不能压过控制和IPC |
| Wi-Fi Upload | 3 | 3072 B | 故障图片上传允许后台执行 |

图传线程每发送8批（24个无线包）主动阻塞1 tick，避免在关闭时间片轮转的配置下
长期占用CPU，让低优先级Wi-Fi线程获得运行机会。

## 4. 空中协议兼容性

命令链路保持手持端现有配置：

- 频道76、2 Mbps、默认5字节地址；
- 8字节 `wireless_touch_packet_t`；
- XOR校验与sequence去重；
- 方向松开立即转换为停车命令；
- 速度索引0～4映射为50%、63%、75%、88%、100%。

图传协议保持手持端 `hal_entry.c` 当前接收格式：

- 32字节固定无线包，24字节JPEG净载荷；
- START/DATA/END、frame id、chunk index、CRC32；
- JPEG源图必须为240x136灰度JPEG，手持端解码成480x272 RGB888。

为避免两条链路在同一射频频道互相争用，车端图传设置为频道100。因此手持端最终版
必须把“图传接收NRF”改到频道100；命令发送NRF继续使用频道76。当前手持 `gui`
分支仍是同频道环回测试配置，这一处必须同步修改后才能真机互通。

## 5. IPC向图传线程交接案例

```c
/* IPC Thread：收到M85共享内存描述符并完成地址、长度、CRC、Cache校验之后 */
video_frame_t frame = {
    .p_jpeg       = shared_memory_address,
    .jpeg_size    = descriptor.jpeg_size,
    .crc32        = descriptor.crc32,
    .frame_id     = descriptor.frame_id,
    .source_width = 240U,
    .source_height = 136U,
};

if (VideoFrameMailbox_Publish(&frame)) {
    /* 发布后不能让M85覆盖这块共享内存。 */
}

uint16_t completed_id;
bool success;
if (VideoFrameMailbox_CompletionTake(&completed_id, &success)) {
    /* 通过IPC ACK通知M85：该frame对应缓冲区现在可以复用。 */
}
```

邮箱只传元数据和指针，不复制最大128 KB的JPEG。这样速度快，但必须严格遵守
“Publish后不可覆盖、Completion后才能复用”的所有权规则。

## 6. e2 studio 操作顺序

1. 在双核 Solution Configurator 中把 `SPI0`、`SPI1`、`ICU EXT IRQ19` 和表中引脚
   分配给 CPU1；确认 CPU0 不再拥有这些资源。
2. 打开 CPU1 `configuration.xml`，核对红色冲突标记为0。
3. 点击 Generate Project Content，让 `ra_gen`、`ra_cfg` 和安全资源文件重新生成。
4. Refresh CPU1工程，然后 Clean Project、Build Project。
5. 检查生成的 `g_spi0_cfg.channel == 0`、`g_spi1_cfg.channel == 1`，以及
   `g_external_irq0_cfg.channel == 19`。
6. 首次上电先只接命令NRF，验证连接和200 ms失联停车；再接图传NRF。

不要手工长期维护 `ra_gen` 或 `Debug/subdir.mk`，它们应由FSP/e2 studio生成。
