# 遥控器—小车 nRF24 无线协议（当前稳定图传版本）

本文档与遥控器工程 `doc/NRF24_VEHICLE_PROTOCOL.md` 对应。所有多字节整数采用小端序，两端 nRF24 均使用 5 字节地址、2 字节硬件 CRC、Dynamic Payload 和 2 Mbps 空中速率。

## 1. 两条独立链路

| 功能 | 遥控器 | 小车 | RF_CH | 地址 | 包长 | Auto ACK |
|---|---|---|---:|---|---:|---|
| 控制 | TX | RX | 76 | `43 4D 44 52 58`（`CMDRX`） | 8 | 开启 |
| 视频 | RX | TX | 100 | `56 49 44 45 4F`（`VIDEO`） | 32 | 关闭 |

地址、信道、速率或 CRC 任一不一致都会导致无法通信。返回值 `4` 表示 `NRF24_RESULT_MAX_RETRANSMIT`，首先检查接收端是否上电、地址是否一致，以及 CE/CSN/IRQ 接线。

## 2. 控制包（遥控器到小车）

固定 8 字节：

| 偏移 | 字段 | 说明 |
|---:|---|---|
| 0 | magic | `0xA5` |
| 1 | version | `0x01` |
| 2 | control | 1方向、2启停、3速度、4模式、5 LED、6风扇、7 Wi-Fi、8页面 |
| 3 | action | 0释放、1按下、2变化 |
| 4..5 | value | uint16，小端 |
| 6 | sequence | uint8 自增并自然回绕 |
| 7 | checksum | byte 0..6 逐字节 XOR |

方向 value：0停止、1前进、2后退、3左转、4右转。小车收到方向释放包时立即停车。

## 3. 视频帧（小车到遥控器）

图像为 200×112 基线灰度 JPEG，遥控器解码并缩放到 480×272 RGB888。每帧严格发送：

```text
START -> DATA[0] -> DATA[1] -> ... -> DATA[N-1] -> END
```

不得交叉两帧。由于实机 ACK 回程持续出现 `MAX_RT=4`，当前版本不依赖 ACK：START/END 各发送 3 次，每个 DATA 分片连续发送 2 次，并且每 4 个分片主动让出 1 tick。遥控器会接受第一份有效分片并忽略重复分片。三包 NO_ACK 高速发送代码仍保留，由 `VIDEO_FAST_BATCH_NO_ACK_ENABLE=0` 排除编译。

### START（32 字节）

| 偏移 | 长度 | 字段 |
|---:|---:|---|
| 0 | 1 | magic=`0x49` |
| 1 | 1 | type=`1` |
| 2 | 2 | frame_id |
| 4 | 1 | version=`1` |
| 5 | 1 | LVGL color format=`0x0F` |
| 6 | 2 | width=`200` |
| 8 | 2 | height=`112` |
| 10 | 4 | jpeg_size |
| 14 | 4 | jpeg_crc32 |
| 18 | 2 | chunk_count=`ceil(jpeg_size/28)` |
| 20 | 1 | codec=`2` |
| 21 | 2 | source_width=`200` |
| 23 | 2 | source_height=`112` |
| 25 | 1 | pattern_id，当前为 0 |
| 26 | 5 | 保留，填 0 |
| 31 | 1 | byte 0..30 XOR |

### DATA（32 字节）

| 偏移 | 长度 | 字段 |
|---:|---:|---|
| 0 | 1 | magic=`0x49` |
| 1 | 1 | type=`2` |
| 2 | 2 | chunk_index，从 0 开始 |
| 4 | 28 | JPEG 数据；末包不足补 0 |

DATA 的 byte 31 属于 JPEG 数据区，不是 XOR 校验。完整性由 nRF24 硬件 CRC 和整帧 CRC32 保证。

### END（32 字节）

| 偏移 | 长度 | 字段 |
|---:|---:|---|
| 0 | 1 | magic=`0x49` |
| 1 | 1 | type=`3` |
| 2 | 2 | frame_id |
| 4 | 2 | chunk_count |
| 6 | 4 | jpeg_size |
| 10 | 4 | jpeg_crc32 |
| 14 | 17 | 保留，填 0 |
| 31 | 1 | byte 0..30 XOR |

JPEG CRC32 参数：初值 `0xFFFFFFFF`、反射多项式 `0xEDB88320`、最低位优先、最终取反，只覆盖 `jpeg_size` 个有效 JPEG 字节。

## 4. 联调判据

1. 先启动并烧录遥控器视频 RX，再启动小车视频 TX，避免 START 包因无接收端而进入 MAX_RT。
2. 小车 RTT 应出现 `[VIDEO NRF] ready SPI1 ch=100`；当前视频发送使用 NO_ACK，不再因接收端没有返回 ACK 而出现 `send failed=4`。
3. 遥控器 RTT 应依次出现 `[IMG RX] START`、`[IMG CHECK] ... PASS`、`[JPEG DEC] ... PASS` 和 `[FRAME]`。
4. `IMG CHECK FAIL` 表示分片、长度或 CRC 不完整；先检查供电去耦、天线距离和 SPI/IRQ，再考虑切换高速 NO_ACK 模式。
5. 两块板近距离桌面联调时不要让两颗天线紧贴；必要时将双方发射功率同步降低。
