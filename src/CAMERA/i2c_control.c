/***********************************************************************************************************************
 * File Name    : i2c_control.c
 * Description  : Contains data structures and functions used to control I2C master module.
 **********************************************************************************************************************/
/***********************************************************************************************************************
* Copyright (c) 2025 Renesas Electronics Corporation and/or its affiliates
*
* SPDX-License-Identifier: BSD-3-Clause
***********************************************************************************************************************/

#include "i2c_control.h"

/* Private function declaration */
static fsp_err_t i2c_master_wait_event (const i2c_master_event_t i2c_event);//wait for i2c master event function declaration

/* Private global variables */
static volatile i2c_master_event_t g_i2c_event_for_peripheral = (i2c_master_event_t)RESET_VALUE;//global variable to store i2c master event,and the event will be set in the i2c master callback funtction,and the event will be used in the i2c_master_Wait_event function to wait for the event occur, and avoid infinite loop by checking the time out

/***********************************************************************************************************************
 *  Function Name: i2c_control_init
 *  Description  : This function is used to initialize I2C master module
 *  Arguments    : None
 *  Return Value : FSP_SUCCESS    Upon successful operation
 *                 Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t i2c_control_init (void)
{
    fsp_err_t err           = FSP_SUCCESS;//define a variable to store the error code,and the initial value is FSP_SUCCESS

    /* Initialize IIC module */
    err = R_IIC_MASTER_Open(&g_i2c_master_for_peripheral_ctrl, &g_i2c_master_for_peripheral_cfg);//open the i2c master module with the configuration,and return the error code if failed
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** R_IIC_MASTER_Open API FAILED ** \r\n");//if the open failed,then return the error code

    return err;//return the error code,if the initialization is successful, then the error code will be FSP_SUCCESS
}
/***********************************************************************************************************************
* End of function i2c_control_init
***********************************************************************************************************************/

/***********************************************************************************************************************
 *  Function Name: write_reg_8bit
 *  Description  : This function is used to write data to 8-bits camera register.
 *  Arguments    : address        Register address
 *                 data           Data to be written
 *  Return Value : FSP_SUCCESS    Upon successful operation
 *                 Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t write_reg_8bit (uint8_t address, uint8_t data)
{
    fsp_err_t err           = FSP_SUCCESS;//define a variable to store the error code,and the initial value is FSP_SUCCESS
    uint8_t i2c_buffer[2];                //define a buffer to store the data to be written to the camera register,the first byte is the register address,and the second byte is the data to be written to the register

    /* Prepare data for write operation */
    i2c_buffer[0] = address;//set the first byte of the buffer to the register address
    i2c_buffer[1] = data;//set the second byte of the buffer to the data to be written to the register

    /* Reset callback event */
    g_i2c_event_for_peripheral = (i2c_master_event_t)RESET_VALUE;//reset the i2c master event to RESET_VALUE before performing the i2c write operation

    /* Perform I2C write operation */
    err = R_IIC_MASTER_Write(&g_i2c_master_for_peripheral_ctrl, i2c_buffer, 2, false);//write the data in the buffer to the camera register through i2c master module,and return the error code if failer
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** R_IIC_MASTER_Write API FAILED ** \r\n");//if the write failed, then return the error code

    /* Wait until write transmission complete */
    err = i2c_master_wait_event (I2C_MASTER_EVENT_TX_COMPLETE);//wait for thr i2c master event of the write transmission complete, and return the error code if failed
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** i2c_master_wait_event FAILED ** \r\n");///if the wait failed, then return the error code

    return err;//return the error code,if the write operation is successful ,then the error code will be FSP_SUCCESS
}
/***********************************************************************************************************************
* End of function write_reg_8bit
***********************************************************************************************************************/

/***********************************************************************************************************************
 *  Function Name: read_reg_8bit
 *  Description  : This function is used to read data from 8-bits camera register.
 *  Arguments    : address        Register address
 *                 p_data         Pointer used to store register data
 *  Return Value : FSP_SUCCESS    Upon successful operation
 *                 Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t read_reg_8bit (uint8_t address, uint8_t * p_data)
{
    fsp_err_t err           = FSP_SUCCESS;//define a variable to store the error code,and the initial value is FSP_SUCCESS
    uint8_t i2c_buffer[1];                //define a buffer to store the datd to be written to the camera register,which is the register address,and the data read from the register will be stored in the variable pointer by p_data

    /* Prepare data for write operation */
    i2c_buffer[0] = address;        //set the first byte of the buffer to the register address, and the data read from the register will be stored in the variable pointer by p_data

    /* Reset callback event */
    g_i2c_event_for_peripheral = (i2c_master_event_t)RESET_VALUE;//reset the i2c master event to RESET_VALUE before performing the i2c write operation to write the register address to the camera,and then the i2c master will perform the read operation the camera register and store the datd read from the register to the variable pointer by p_data

    /* Perform I2C write operation */
    err = R_IIC_MASTER_Write(&g_i2c_master_for_peripheral_ctrl, i2c_buffer, 1, true);//write the register address to the camera through i2c master module
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** R_IIC_MASTER_Write API FAILED ** \r\n");//if the write failed ,then return the error code

    /* Wait until write transmission complete */
    err = i2c_master_wait_event (I2C_MASTER_EVENT_TX_COMPLETE);//wait for the i2c master event of the write transmisson complete ,and then return the error code if failed
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** i2c_master_wait_event FAILED ** \r\n");//if the write failed ,then return the error code

    /* Reset callback event */
    g_i2c_event_for_peripheral = (i2c_master_event_t)RESET_VALUE;//reset the i2c master event to RESET_VALUE before performing the i2c write operation to write the register address to the camera,and then the i2c master will perform the read operation the camera register and store the datd read from the register to the variable pointer by p_data

    /* Perform I2C read operation */
    err = R_IIC_MASTER_Read(&g_i2c_master_for_peripheral_ctrl, p_data, 1, false);//read the data from the camera register through i2c master module ,and store the data read from the register to the variable pointer by p_data
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** R_IIC_MASTER_Read API FAILED ** \r\n");//if the read failed ,then return the error code

    /* Wait until read transmission complete */
    err = i2c_master_wait_event (I2C_MASTER_EVENT_RX_COMPLETE);//wait for thr i2c master event of the read response complete and then return the error code if failed
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** i2c_master_wait_event FAILED ** \r\n");//if the read failed ,then return the error code
    return err;//return the error code , if the event is received successfully ,then the error code will be FSP_SUCCESS
}
/***********************************************************************************************************************
* End of function read_reg_8bit
***********************************************************************************************************************/

/***********************************************************************************************************************
 *  Function Name: read_reg_16bit
 *  Description  : This function is used to read data from 16-bits camera register.
 *  Arguments    : address        Register address
 *                 p_data         Pointer used to store register data
 *  Return Value : FSP_SUCCESS    Upon successful operation
 *                 Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t read_reg_16bit (uint16_t address, uint8_t * p_data)
{
    fsp_err_t err           = FSP_SUCCESS;//define a variable to store the error code,and the initial value is FSP_SUCCESS
    uint8_t i2c_buffer[2];

    /* Prepare data for write operation */
    i2c_buffer[0] = (uint8_t)((address >> 8 ) & 0xFF);
    i2c_buffer[1] = (uint8_t)(address & 0xFF);

    /* Reset callback event */
    g_i2c_event_for_peripheral = (i2c_master_event_t)RESET_VALUE;//reset the i2c master event to RESET_VALUE before performing the i2c write operation to write the register address to the camera,and then the i2c master will perform the read operation the camera register and store the datd read from the register to the variable pointer by p_data


    /* Perform I2C write operation */
    err = R_IIC_MASTER_Write(&g_i2c_master_for_peripheral_ctrl, i2c_buffer, 2, true);//write the register address to the camera through i2c master module
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** R_IIC_MASTER_Write API FAILED ** \r\n");//if the write failed ,then return the error code

    /* Wait until write transmission complete */
    err = i2c_master_wait_event (I2C_MASTER_EVENT_TX_COMPLETE);//wait for the i2c master event of the write transmisson complete ,and then return the error code if failed
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** i2c_master_wait_event FAILED ** \r\n");//if the write failed ,then return the error code


    /* Reset callback event */
    g_i2c_event_for_peripheral = (i2c_master_event_t)RESET_VALUE;//reset the i2c master event to RESET_VALUE before performing the i2c write operation to write the register address to the camera,and then the i2c master will perform the read operation the camera register and store the datd read from the register to the variable pointer by p_data


    /* Perform I2C read operation */
    err = R_IIC_MASTER_Read(&g_i2c_master_for_peripheral_ctrl, p_data, 1, false);//read the data from the camera register through i2c master module ,and store the data read from the register to the variable pointer by p_data
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** R_IIC_MASTER_Read API FAILED ** \r\n");//if the read failed ,then return the error code

    /* Wait until read transmission complete */
    err = i2c_master_wait_event (I2C_MASTER_EVENT_RX_COMPLETE);//wait for thr i2c master event of the read response complete and then return the error code if failed
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** i2c_master_wait_event FAILED ** \r\n");//if the read failed ,then return the error code

    return err;
}
/***********************************************************************************************************************
* End of function read_reg_16bit
***********************************************************************************************************************/

/***********************************************************************************************************************
 *  Function Name: write_reg_16bit
 *  Description  : This function is used to write data to 16-bits camera register.
 *  Arguments    : address        Register address
 *                 data           Data to be written
 *  Return Value : FSP_SUCCESS    Upon successful operation
 *                 Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t write_reg_16bit (uint16_t address, uint8_t data)
{
    fsp_err_t err           = FSP_SUCCESS;//define a variable to store the error code,and the initial value is FSP_SUCCESS
    uint8_t i2c_buffer[3];              //define a buffer to store the data to be written to the camera register,the first two bytes are the register address,and the third byte is the data to be written to the register  

    /* Prepare data for write operation */
    i2c_buffer[0] = (uint8_t)((address >> 8 ) & 0xFF);
    i2c_buffer[1] = (uint8_t)(address & 0xFF);
    i2c_buffer[2] = data;

    /* Reset callback event */
    g_i2c_event_for_peripheral = (i2c_master_event_t)RESET_VALUE;//reset the i2c master event to RESET_VALUE before performing the i2c write operation to write the register address to the camera,and then the i2c master will perform the read operation the camera register and store the datd read from the register to the variable pointer by p_data


    /* Perform I2C write operation */
    err = R_IIC_MASTER_Write(&g_i2c_master_for_peripheral_ctrl, i2c_buffer, 3, false);//write the data in the buffer to the camera register through i2c master module ,and return the error code if failed
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** R_IIC_MASTER_Write API FAILED ** \r\n");

    /* Wait until write transmission complete */
    err = i2c_master_wait_event (I2C_MASTER_EVENT_TX_COMPLETE);//wait for the i2c master event to be transmission complete and then return the error code if failed
    APP_ERR_RET( FSP_SUCCESS != err, err, " ** i2c_master_wait_event FAILED ** \r\n");

    return err;
}
/***********************************************************************************************************************
* End of function write_reg_16bit
***********************************************************************************************************************/

/***********************************************************************************************************************
 *  Function Name: i2c_master_wait_event
 *  Description  : This function is used to wait for i2c master event.
 *  Arguments    : i2c_event      i2c master events need to wait for
 *  Return Value : FSP_SUCCESS    Upon successful operation
 *                 Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
static fsp_err_t i2c_master_wait_event (const i2c_master_event_t i2c_event)
{
    fsp_err_t err      = FSP_SUCCESS;//default rerurn value is success.if the event is aborted ot timeout ,the return value will be updated to error code accordingly.
    uint8_t   time_out = UINT8_MAX;//default timeout value ,if the event is not received withtin this time,the function will return with timeout error.

    /* Wait until event occur */
    while(i2c_event != g_i2c_event_for_peripheral)//its should be RESET_VALUE at the beginning,once the event is received ,it will be updated to the event type in callback function,then the loop will be exited.
    {
        /* Check for aborted event */
        if(I2C_MASTER_EVENT_ABORTED == g_i2c_event_for_peripheral)//it the event is aborted ,return with error code immediately.
        {
            APP_ERR_RET( FSP_SUCCESS != err, FSP_ERR_TRANSFER_ABORTED,
                           " ** Error EVENT_ABORTED received during perform i2c operation **\r\n");
        }
        else
        {
            /* Start checking for time out to avoid infinite loop */
            time_out --;
            R_BSP_SoftwareDelay(I2C_TIMEOUT_UNIT, BSP_DELAY_UNITS_MICROSECONDS);//delay 10us for each loop and check the event again until timeout or event is received.

            /* Check for time elapse*/
            if (RESET_VALUE == time_out)//if the timeout value elapse and the event is still not received,return with timeout error code.
            {
                APP_ERR_RET( FSP_SUCCESS != err, FSP_ERR_TIMEOUT, " ** I2C Master Callback event not received **\r\n");
            }
        }
    }
    return err;//return the error code , if the event is received successfully ,then the error code will be FSP_SUCCESS
}
/***********************************************************************************************************************
* End of function i2c_master_wait_event
***********************************************************************************************************************/

/***********************************************************************************************************************
 *  Function Name: g_i2c_master_for_peripheral_callback
 *  Description  : This function is used to get i2c master event
 *  Arguments    : p_args      Pointer to callback argument
 *  Return Value : None
 **********************************************************************************************************************/
void g_i2c_master_for_peripheral_callback(i2c_master_callback_args_t * p_args)
{
    if (NULL != p_args)
    {
        g_i2c_event_for_peripheral = p_args->event;//set the i2c master event to the event received in the callback argument .and then return the error code if failed
    }
}
/***********************************************************************************************************************
* End of function g_i2c_master_for_peripheral_callback
***********************************************************************************************************************/
