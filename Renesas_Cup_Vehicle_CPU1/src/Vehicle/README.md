# CPU1 Vehicle 子系统分层说明

## 目标

本目录用一个可运行的底盘模块示范 RTOS 工程中的单向依赖和单一硬件所有者。核心规则是：

1. 只有 `Vehicle Thread` 可以调用 `vehicle_service_*()` 并最终写 GPT/IIC。
2. NRF、IPC、RTT 等输入模块只提交命令，不直接操作电机。
3. 业务代码不包含 `hal_data.h` 或 FreeRTOS 头文件。
4. FSP 生成对象只允许出现在 `platform/`。

## 目录职责

```text
Vehicle/
├─ contracts/       跨层数据、命令和硬件能力接口
├─ domain/          纯控制算法；无 FSP、无 RTOS
├─ device/          MPU6050 设备协议；只依赖 I2C 抽象端口
├─ application/     车辆用例和状态机；组织算法与设备
├─ platform/        RA8P1 FSP 的 GPT/IIC 具体实现
└─ adapters/rtos/   FreeRTOS 队列等输入适配
```

依赖方向：

```text
线程/NRF/RTT → application → domain/device → contracts
                    ↑
                platform
```

箭头表示“调用或实现”。`application` 只认识 `contracts` 中的接口；启动时由
`fsp_vehicle_factory.c` 把实际 FSP 驱动注入应用服务，这就是依赖倒置。

## 案例一：更换 IMU 总线

`mpu6050.c` 只调用 `vehicle_i2c_port_t.read/write`。如果以后 MPU6050 从 IIC0
换到软件 I2C，只需新增一个 platform 实现并在 factory 中替换，设备解析和车辆状态机不变。

## 案例二：NRF 遥控命令

Command RX Thread 收到合法数据后构造 `vehicle_command_t`，调用：

```c
vehicle_command_t command = {
    .source = VEHICLE_COMMAND_SOURCE_NRF,
    .kind = VEHICLE_COMMAND_MANUAL,
    .manual_action = VEHICLE_MANUAL_FORWARD,
    .speed_percent = 40U,
    .received_tick = xTaskGetTickCount(),
};
vehicle_command_mailbox_submit(&command);
```

队列长度为 1，新摇杆位置覆盖旧位置。Vehicle Thread 在下一次 10 ms 周期取出命令，
应用服务再写 GPT。这样不会积压过期动作，也不会出现两个线程同时切换电机方向。

## 案例三：为何吸附启动不用阻塞延时

原实现开启风机后 `SoftwareDelay(2000 ms)`，这两秒内控制线程无法处理急停。现在 init
只启动风机，`vehicle_service_step()` 每 10 ms 累计时间；满两秒后才允许车轮运动。
因此启动条件保持不变，但线程始终活着，能够及时处理急停和故障。

## 尚需结合硬件验证的安全点

- FSP XML 中 GPT6/GPT7/GPT8 的初始 duty 应全部改为 0%。
- 两轮由正转切反转时，当前实现先关闭两路；若 H 桥要求死区，还需在应用状态机中加入
  数毫秒的非阻塞换向阶段。
- IIC0 callback 当前使用 Task Notification，因此 IIC0 必须只归 Vehicle Thread 所有。
- 车辆线程栈建议先设为 2048～3072 字节，并用 high-water mark 实测后缩减。
