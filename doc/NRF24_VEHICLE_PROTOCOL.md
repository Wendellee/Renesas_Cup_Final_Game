# 遥控器与小车 nRF24L01+ 统一通信协议

## 1. 链路总览

小车端需要两条单向无线链路。两条链路使用不同信道，可以使用两颗独立 nRF24，也可以在小车端根据硬件能力自行设计双模块实现。

| 链路 | 遥控器角色 | 小车角色 | 信道 | 载荷 |
|---|---|---|---:|---|
| 控制链路 | TX | RX | 76 | 固定 8 字节控制包 |
| 视频链路 | RX | TX | 100 | 固定 32 字节图像分片 |

推荐双方统一采用：

| 参数 | 值 |
|---|---|
| 数据率 | 2 Mbps |
| 地址宽度 | 5 字节 |
| 地址 | `E7 E7 E7 E7 E7` |
| RF CRC | 2 字节 |
| Dynamic Payload | 开启 |
| 控制链路 Auto ACK | 开启 |
| 控制链路重发 | ARD=5，即 1500 µs；ARC=15 |
| 视频链路 | 推荐 NO_ACK，依靠帧 CRC32 检错并丢弃坏帧 |
| 字节序 | 所有多字节整数均为小端序 |

注意：信道 76 和 100 是 nRF24 的 `RF_CH` 数值，不是 MHz。实际中心频率分别为 2476 MHz 和 2500 MHz。

## 2. 控制包

控制包固定 8 字节，由遥控器发送，小车接收。

| 偏移 | 长度 | 字段 | 说明 |
|---:|---:|---|---|
| 0 | 1 | `magic` | 固定 `0xA5` |
| 1 | 1 | `version` | 固定 `0x01` |
| 2 | 1 | `control` | 控制对象 |
| 3 | 1 | `action` | 操作类型 |
| 4 | 1 | `value_lsb` | `value` 低字节 |
| 5 | 1 | `value_msb` | `value` 高字节 |
| 6 | 1 | `sequence` | 每发送一包加 1，按 uint8_t 回绕 |
| 7 | 1 | `checksum` | 字节 0～6 的逐字节 XOR |

校验算法：

```c
uint8_t checksum = 0;
for (uint32_t i = 0; i < 7; i++) {
    checksum ^= packet[i];
}
```

### 2.1 `control` 定义

| 值 | 名称 | `value` 含义 |
|---:|---|---|
| 1 | DIRECTION | 方向枚举 |
| 2 | RUN_STOP | `0=Stop`，`1=Run` |
| 3 | SPEED | 速度档位索引 0～4 |
| 4 | MODE | `0=Manual`，`1=Automatic` |
| 5 | LED | `0=Off`，`1=On` |
| 6 | FAN | `0=Stop`，`1=Run` |
| 7 | WIFI | 下拉列表索引 |
| 8 | PAGE | `0=Main`，`1=About`；小车可忽略 |

### 2.2 `action` 定义

| 值 | 名称 | 说明 |
|---:|---|---|
| 0 | RELEASED | 按键释放 |
| 1 | PRESSED | 按键按下 |
| 2 | CHANGED | 状态或数值发生变化 |

### 2.3 方向值

| 值 | 方向 |
|---:|---|
| 0 | STOP |
| 1 | FORWARD |
| 2 | BACK |
| 3 | LEFT |
| 4 | RIGHT |

遥控器对方向按钮发送 `PRESSED + 对应方向`，释放时发送 `RELEASED + 原方向`。小车端收到任意合法方向 `RELEASED` 时应立即停止该方向运动，不应等待新的 `STOP` 包。

### 2.4 速度档位

| `value` | GUI 显示 | 建议小车换算 |
|---:|---:|---:|
| 0 | 50% | 50% PWM |
| 1 | 62.5% | 62.5% PWM |
| 2 | 75% | 75% PWM |
| 3 | 87.5% | 87.5% PWM |
| 4 | 100% | 100% PWM |

### 2.5 控制接收建议

小车端接收控制包时按以下顺序处理：

1. 长度必须等于 8。
2. 检查 `magic == 0xA5`。
3. 检查 `version == 1`。
4. 检查 XOR 校验。
5. 检查 `control/action/value` 范围。
6. 利用 `sequence` 统计丢包或重复包，但 uint8_t 回绕属于正常情况。
7. 方向释放、STOP 和失联停车应具有最高执行优先级。

建议小车增加 200～500 ms 控制失联超时；超时后强制电机停止。具体超时值需要根据后续控制心跳周期确定。

## 3. 视频帧格式

小车把每帧图像编码成以下格式：

- JPEG：基线灰度 JPEG；
- JPEG 原始尺寸：200×112；
- 遥控器解码后显示尺寸：480×272 RGB888；
- 单帧 JPEG 最大长度：128 KiB；
- 每个无线包：固定 32 字节；
- 每个 DATA 包有效载荷：28 字节。

每帧依次发送：

```text
START -> DATA[0] -> DATA[1] -> ... -> DATA[N-1] -> END
```

不得交叉发送两帧。发送下一帧前必须完成上一帧 END。

## 4. 图像公共定义

| 名称 | 值 |
|---|---:|
| `IMAGE_MAGIC` | `0x49`，ASCII `I` |
| `IMAGE_VERSION` | `1` |
| `START` | `1` |
| `DATA` | `2` |
| `END` | `3` |
| JPEG Codec | `2` |
| Color Format | `0x0F`，与遥控器 LVGL `LV_COLOR_FORMAT_RGB888` 一致 |
| Camera Pattern ID | `0xFE` |
| 单包长度 | 32 |
| DATA 数据区 | 28 字节 |

## 5. START 包

START 固定 32 字节：

| 偏移 | 长度 | 字段 | 规定值/说明 |
|---:|---:|---|---|
| 0 | 1 | magic | `0x49` |
| 1 | 1 | type | `0x01` |
| 2 | 2 | frame_id | 帧号，小端，建议从 1 开始 |
| 4 | 1 | version | `0x01` |
| 5 | 1 | color_format | `0x0F` |
| 6 | 2 | width | `200` |
| 8 | 2 | height | `112` |
| 10 | 4 | jpeg_size | JPEG 实际字节数 |
| 14 | 4 | jpeg_crc32 | 整个 JPEG 数据 CRC32 |
| 18 | 2 | chunk_count | `ceil(jpeg_size / 28)` |
| 20 | 1 | codec | `0x02` |
| 21 | 2 | source_width | `200` |
| 23 | 2 | source_height | `112` |
| 25 | 1 | pattern_id | 小车摄像头建议 `0xFE` |
| 26 | 5 | reserved | 全部填 0 |
| 31 | 1 | checksum | 字节 0～30 XOR |

## 6. DATA 包

DATA 固定 32 字节：

| 偏移 | 长度 | 字段 | 说明 |
|---:|---:|---|---|
| 0 | 1 | magic | `0x49` |
| 1 | 1 | type | `0x02` |
| 2 | 2 | chunk_index | 从 0 开始，小端 |
| 4 | 28 | jpeg_data | JPEG 分片；最后一包不足部分填 0 |

DATA 包不携带 `frame_id`，因此接收端将它归入最近一个合法 START 建立的活动帧。小车必须保证严格顺序发送且不能交叉帧。

DATA 包的第 31 字节属于 JPEG 数据区，不是应用层 XOR 校验。DATA 完整性由 nRF24 硬件 CRC 和帧尾 CRC32 检查。

## 7. END 包

END 固定 32 字节：

| 偏移 | 长度 | 字段 | 说明 |
|---:|---:|---|---|
| 0 | 1 | magic | `0x49` |
| 1 | 1 | type | `0x03` |
| 2 | 2 | frame_id | 必须与 START 一致 |
| 4 | 2 | chunk_count | 必须与 START 一致 |
| 6 | 4 | jpeg_size | 必须与 START 一致 |
| 10 | 4 | jpeg_crc32 | 必须与 START 一致 |
| 14 | 17 | reserved | 全部填 0 |
| 31 | 1 | checksum | 字节 0～30 XOR |

## 8. CRC32 算法

JPEG CRC32 使用反射形式：

- 初值：`0xFFFFFFFF`；
- 多项式：`0xEDB88320`；
- 逐字节、最低位优先；
- 最终按位取反。

等价于常见的 CRC-32/ISO-HDLC 数据处理形式。

```c
uint32_t crc32(const uint8_t *data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFU;
    for (uint32_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint32_t bit = 0; bit < 8; bit++) {
            uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}
```

CRC 只覆盖 `jpeg_size` 个实际 JPEG 字节，不包括最后一个 DATA 包的补零，也不包括 START/END 包。

## 9. 小车视频发送状态机

建议小车实现以下状态机：

```text
IDLE
  -> 摄像头获得完整帧
  -> 压缩为 200×112 灰度 JPEG
  -> 计算 jpeg_size、chunk_count、CRC32
START
  -> 发送 START
DATA
  -> 按 chunk_index=0..N-1 顺序发送
END
  -> 发送 END
  -> 回到 IDLE
```

遥控器接收端规则：

- 新 START 会重置当前接收状态；
- 分片序号跳跃会记录缺片；
- 重复或倒序分片不会修复先前缺片；
- 活动帧 250 ms 内没有新包会被丢弃；
- 只有长度、分片数、元数据和 CRC32 全部正确才解码显示；
- 坏帧直接丢弃，继续等待下一帧 START。

因此小车端应优先保持低延迟和严格顺序，不应为旧帧长时间阻塞新帧。

## 10. 联调检查表

### 控制链路

- 小车 RX 使用信道 76、2 Mbps、地址 `E7:E7:E7:E7:E7`。
- 打开 Dynamic Payload 和 2 字节 RF CRC。
- 打开 Auto ACK。
- 收到 8 字节后按 XOR 校验和枚举范围验证。
- 验证方向按下、释放、RUN/STOP 和速度档位。

### 视频链路

- 小车 TX 使用信道 100、2 Mbps、地址 `E7:E7:E7:E7:E7`。
- 每包固定发送 32 字节。
- 严格按 START、连续 DATA、END 顺序发送。
- 多字节字段使用小端序。
- JPEG 必须是 200×112 基线灰度 JPEG，最大 128 KiB。
- CRC32 必须只覆盖 JPEG 实际数据。
- 初次联调建议先降低帧率，并通过遥控器 RTT 查看 `[IMG CHECK]`、`[JPEG DEC]` 和 `[FRAME]` 日志。


