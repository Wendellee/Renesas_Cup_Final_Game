#include "touch_GT1151.h"
#include <string.h>

/* GT1151 配置表 (184字节，位于寄存器 0x8047 ~ 0x80FE)
 *
 * 寄存器映射:
 *   0x8047        : 配置版本号
 *   0x8048-0x8049 : X 输出最大值 (800 = 0x0320, little-endian)
 *   0x804A-0x804B : Y 输出最大值 (480 = 0x01E0, little-endian)
 *   0x804C        : 最大触摸点数
 *   0x804D        : 模块开关1 (触发边沿 / XY交换 / XY翻转)
 *   0x804E        : 模块开关2 (INT模式 / 触控按键)
 *   0x804F        : 消抖次数
 *   0x8050        : 滤波等级
 *   0x8051        : 大触摸阈值
 *   0x8052        : 噪声消除
 *   0x8053        : 按下阈值
 *   0x8054        : 抬起阈值
 *   0x8055        : 低功耗扫描次数
 *   0x8056        : 刷新率
 *   0x8057-0x8058 : X 门限 (物理触摸传感器宽度)
 *   0x8059-0x805A : Y 门限 (物理触摸传感器高度)
 *   0x805B        : X 速度调整系数
 *   0x805C        : Y 速度调整系数
 *   0x805D-0x805E : 边缘校正间距
 *   0x805F        : 拉伸比例
 *   0x8060-0x8063 : 拉伸系数 R0/R1/R2/New
 *   0x8064-0x806B : 跳频参数
 *   0x806C-0x8071 : 驱动组配置
 *   0x8072-0x807F : 保留 / 驱动组延时
 *   0x8080-0x809F : 触控按键映射 (无触控按键则置0)
 *   0x80A0-0x80AF : 灵敏度曲线表 (小尺寸触摸)
 *   0x80B0-0x80BF : 灵敏度曲线表 (大尺寸触摸)
 *   0x80C0-0x80EF : 保留
 *   0x80F0-0x80FD : 拉伸曲线表
 *   0x80FE        : 保留
 *   0x80FF        : 校验和 (由 gt1151_calc_checksum 计算写入)
 *   0x8100        : 配置刷新标志 (写入 0x01)
 */
static const uint8_t g_gt1151_config[] = {
    /* 0x8047 */ 0x61,
    /* 0x8048 */ 0x20, 0x03,   /* X=800 */
    /* 0x804A */ 0xE0, 0x01,   /* Y=480 */
    /* 0x804C */ 0x05,         /* 最大5点触摸 */
    /* 0x804D */ 0x05,         /* 模块开关1: 上升沿触发, 不交换XY */
    /* 0x804E */ 0x00,         /* 模块开关2: 脉冲INT, 无触控按键 */
    /* 0x804F */ 0x01,         /* 消抖次数 */
    /* 0x8050 */ 0x08,         /* 滤波等级 */
    /* 0x8051 */ 0x28,         /* 大触摸阈值 */
    /* 0x8052 */ 0x05,         /* 噪声消除 */
    /* 0x8053 */ 0x50,         /* 按下阈值 */
    /* 0x8054 */ 0x32,         /* 抬起阈值 */
    /* 0x8055 */ 0x03,         /* 低功耗扫描次数 */
    /* 0x8056 */ 0x05,         /* 刷新率 (ms) */
    /* 0x8057 */ 0x00, 0x10,   /* X门限 = 4096 */
    /* 0x8059 */ 0x00, 0x08,   /* Y门限 = 2048 */
    /* 0x805B */ 0x00,         /* X速度调整 */
    /* 0x805C */ 0x00,         /* Y速度调整 */
    /* 0x805D */ 0x00, 0x00,   /* 边缘间距 */
    /* 0x805F */ 0x00,         /* 拉伸比例 */
    /* 0x8060 */ 0x86, 0x27, 0x08, 0x17,  /* 拉伸系数 */
    /* 0x8064 */ 0x15, 0x31, 0x0D, 0x00, 0x00, 0x02, 0xBB, 0x03,  /* 跳频 */
    /* 0x806C */ 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x8072 */ 0x03, 0x64, 0x32, 0x00, 0x00, 0x00,
    /* 0x8078 */ 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00,
    /* 0x8080 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x8088 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x8090 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x8098 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x80A0 */ 0x12, 0x10, 0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04,
    /* 0x80A8 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
    /* 0x80B0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x80B8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x80C0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x80C8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x80D0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x80D8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x80E0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x80E8 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0x80F0 */ 0x24, 0x22, 0x21, 0x20, 0x1F, 0x1E, 0x1D, 0x00,
    /* 0x80F8 */ 0x02, 0x04, 0x06, 0x08, 0x0A, 0xFF, 0xFF
};

/* 计算配置校验和 */
static uint8_t gt1151_calc_checksum(const uint8_t *buf, uint8_t len)
{
    uint8_t ccsum = 0;
    for (uint8_t i = 0; i < len; i++)
    {
        ccsum += buf[i];
    }
    ccsum = (~ccsum) + 1;
    return ccsum;
}

/* 复位 GT1151 时序 */
static void gt1151_reset(void)
{
    /* Step 1: RST 拉低 */
    R_IOPORT_PinWrite(&g_ioport_ctrl, TCH_RST, BSP_IO_LEVEL_LOW);

    /* Step 2: INT 设为输出低 */
    R_IOPORT_PinCfg(&g_ioport_ctrl, TCH_INT,
        (uint32_t)IOPORT_CFG_PORT_DIRECTION_OUTPUT |
        (uint32_t)IOPORT_CFG_PORT_OUTPUT_LOW);

    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MICROSECONDS);

    /* Step 3: INT 拉高 (0x5D地址模式) */
#if (GT1151_I2C_ADDRESS_0x14 == GT1151_I2C_ADDRESS)
    R_IOPORT_PinWrite(&g_ioport_ctrl, TCH_INT, BSP_IO_LEVEL_HIGH);
#elif (GT1151_I2C_ADDRESS_0x5D == GT1151_I2C_ADDRESS)
    R_IOPORT_PinWrite(&g_ioport_ctrl, TCH_INT, BSP_IO_LEVEL_LOW);
#endif

    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MICROSECONDS);

    /* Step 4: RST 拉高 */
    R_IOPORT_PinWrite(&g_ioport_ctrl, TCH_RST, BSP_IO_LEVEL_HIGH);

    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

    /* Step 5: INT 拉低进入工作模式 */
    R_IOPORT_PinWrite(&g_ioport_ctrl, TCH_INT, BSP_IO_LEVEL_LOW);

    R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);

    /* Step 6: INT 设为输入，使能IRQ */
    R_IOPORT_PinCfg(&g_ioport_ctrl, TCH_INT,
        ((uint32_t)IOPORT_CFG_IRQ_ENABLE |
         (uint32_t)IOPORT_CFG_PORT_DIRECTION_INPUT));
}

/*******************************************************************************
 * I2C 读写接口 (基于 i2c_control.c)
 ******************************************************************************/

fsp_err_t gt1151_write_reg(uint16_t addr, uint8_t data)
{
    return write_reg_16bit(addr, data);
}

fsp_err_t gt1151_read_reg(uint16_t addr, uint8_t *p_data)
{
    return read_reg_16bit(addr, p_data);
}

fsp_err_t gt1151_read_multi(uint16_t addr, uint8_t *p_data, uint8_t len)
{
    fsp_err_t err = FSP_SUCCESS;
    for (uint8_t i = 0; i < len; i++)
    {
        err = read_reg_16bit((uint16_t)(addr + i), &p_data[i]);
        if (FSP_SUCCESS != err)
        {
            break;
        }
    }
    return err;
}

/*******************************************************************************
 * GT1151 初始化
 ******************************************************************************/

fsp_err_t gt1151_init(void)
{
    fsp_err_t err;
    uint8_t product_id[4];

    /* 初始化 I2C */
    err = i2c_control_init();
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* 复位 GT1151 */
    gt1151_reset();

    /* 读取产品 ID (应为 "1151") */
    err = gt1151_read_multi(GT1151_REG_PRODUCT_ID, product_id, 4);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* 验证产品 ID: '1','1','5','1' = 0x31,0x31,0x35,0x31 */
    if (product_id[0] != '1' || product_id[1] != '1' ||
        product_id[2] != '5' || product_id[3] != '1')
    {
        return FSP_ERR_ASSERTION;
    }

    return FSP_SUCCESS;
}

/*******************************************************************************
 * GT1151 使能 (写入配置并开启触摸)
 ******************************************************************************/

fsp_err_t gt1151_enable(void)
{
    fsp_err_t err;
    uint8_t checksum;
    uint8_t config_fresh;

    /* 复位 GT1151 */
    gt1151_reset();

    /* 写入配置表到寄存器 0x8047 ~ 0x80FE */
    for (uint8_t i = 0; i < sizeof(g_gt1151_config); i++)
    {
        err = gt1151_write_reg((uint16_t)(GT1151_REG_CONFIG_VERSION + i),
                               g_gt1151_config[i]);
        if (FSP_SUCCESS != err)
        {
            return err;
        }
    }

    /* 写入校验和 (0x80FF) 和配置刷新标志 (0x8100) */
    checksum = gt1151_calc_checksum((uint8_t *)g_gt1151_config,
                                     sizeof(g_gt1151_config));
    err = gt1151_write_reg(GT1151_REG_CONFIG_CHECKSUM, checksum);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    config_fresh = 0x01;
    err = gt1151_write_reg(GT1151_REG_CONFIG_FRESH, config_fresh);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    /* 清除命令寄存器，使触摸IC进入工作模式 */
    err = gt1151_write_reg(GT1151_REG_COMMAND, 0x00);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    return FSP_SUCCESS;
}

/*******************************************************************************
 * 读取触摸坐标数据 (供 LVGL touchpad_read 回调调用)
 *
 * 参数:
 *   coords : 坐标数组 (调用者分配，建议大小 >= 5)
 *   count  : 输出: 有效触摸点数
 *   event  : 输出: 触摸事件类型
 * 返回:
 *   FSP_SUCCESS 成功，其他值表示I2C通信错误
 ******************************************************************************/

fsp_err_t gt1151_read_touch(TouchCoordinate_t *coords, uint8_t *count,
                             touch_event_t *event)
{
    fsp_err_t err;
    uint8_t status_reg;
    uint8_t num_points;
    uint8_t read_data[GT1151_POINT_DATA_SIZE];

    /* 读取状态寄存器 0x814E */
    err = gt1151_read_reg(GT1151_REG_READ_COORD_ADDR, &status_reg);
    if (FSP_SUCCESS != err)
    {
        *event = TOUCH_EVENT_NONE;
        *count = 0;
        return err;
    }

    /* 检查缓冲区是否就绪 */
    if (BUFFER_READY != (BUFFER_READY & status_reg))
    {
        *event = TOUCH_EVENT_NONE;
        *count = 0;
        return FSP_SUCCESS;
    }

    num_points = status_reg & NUM_TOUCH_POINTS_MASK;

    if (0 == num_points)
    {
        /* 抬起事件 */
        *event = TOUCH_EVENT_UP;
        *count = 0;
    }
    else
    {
        /* 读取各触摸点坐标 */
        for (uint8_t i = 0; i < num_points; i++)
        {
            err = gt1151_read_multi(
                (uint16_t)(GT1151_REG_POINT1_X_ADDR + (i * GT1151_POINT_DATA_SIZE)),
                read_data, GT1151_POINT_DATA_SIZE);
            if (FSP_SUCCESS != err)
            {
                *count = i;
                return err;
            }

            coords[i].x = (uint16_t)((read_data[2] << 8) | read_data[1]);
            coords[i].y = (uint16_t)((read_data[4] << 8) | read_data[3]);
        }

        *event = TOUCH_EVENT_DOWN;
        *count = num_points;
    }

    /* 写 0 清除状态寄存器，等待下一次触摸事件 */
    err = gt1151_write_reg(GT1151_REG_READ_COORD_ADDR, 0x00);

    return err;
}
