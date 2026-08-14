/*
 * i2c_control.h
 *
 *  Created on: 2026年8月10日
 *      Author: lingk
 */

#ifndef CAMERA_I2C_CONTROL_H_
#define CAMERA_I2C_CONTROL_H_

#include "camera_thread.h"

/*
 *[@name] i2c_control_init
 *[@type] function
 *[@usage] 在Camera Thread任务上下文初始化摄像头IIC主机
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，否则返回对应FSP错误码
 */
fsp_err_t i2c_control_init(void);

/*
 *[@name] write_reg_16bit
 *[@type] function
 *[@usage] 向摄像头写入一个16位地址、8位数据寄存器
 *[@argument] address 16位寄存器地址
 *[@argument] data 待写入的8位数据
 *[@return] 成功返回FSP_SUCCESS，否则返回对应FSP错误码
 */
fsp_err_t write_reg_16bit(uint16_t address, uint8_t data);

/*
 *[@name] read_reg_16bit
 *[@type] function
 *[@usage] 从摄像头读取一个16位地址、8位数据寄存器
 *[@argument] address 16位寄存器地址
 *[@argument] p_data 返回读取到的8位数据
 *[@return] 成功返回FSP_SUCCESS，否则返回对应FSP错误码
 */
fsp_err_t read_reg_16bit(uint16_t address, uint8_t * p_data);

/*
 *[@name] write_reg_8bit
 *[@type] function
 *[@usage] 向摄像头写入一个8位地址、8位数据寄存器
 *[@argument] address 8位寄存器地址
 *[@argument] data 待写入的8位数据
 *[@return] 成功返回FSP_SUCCESS，否则返回对应FSP错误码
 */
fsp_err_t write_reg_8bit(uint8_t address, uint8_t data);

/*
 *[@name] read_reg_8bit
 *[@type] function
 *[@usage] 从摄像头读取一个8位地址、8位数据寄存器
 *[@argument] address 8位寄存器地址
 *[@argument] p_data 返回读取到的8位数据
 *[@return] 成功返回FSP_SUCCESS，否则返回对应FSP错误码
 */
fsp_err_t read_reg_8bit(uint8_t address, uint8_t * p_data);

#endif /* CAMERA_I2C_CONTROL_H_ */
