/***********************************************************************************************************************
 * File Name    : i2c_control.h
 * Description  : Contains data structures and functions used in i2c_control.c.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#ifndef I2C_CONTROL_H_
#define I2C_CONTROL_H_

#include "common_utils.h"//include common_utils.h header file for using the common utility functions and marcos defined in the common_utils.h file

#define I2C_TIMEOUT_UNIT    (10U)//define a macro for i2c timeout unit,the value is 10 microseconds,and the macro will be used in the i2c_master_wait_event function to check the time out when waiting for i2c master event
/***********************************************************************************************************************
 * Public function prototypes
 **********************************************************************************************************************/
fsp_err_t i2c_control_init (void);//initialize the i2c master module
fsp_err_t write_reg_16bit (uint16_t address, uint8_t data);//write data to 16-bits camera register
fsp_err_t read_reg_16bit (uint16_t address, uint8_t * p_data);//read data from 16_bits camera register
fsp_err_t write_reg_8bit (uint8_t address, uint8_t data);//write data to 8-bits camera register
fsp_err_t read_reg_8bit (uint8_t address, uint8_t * p_data);//read data from 8-bits camera register

#endif /* I2C_CONTROL_H_ */
