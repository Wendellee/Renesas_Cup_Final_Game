/*
 * glcdc_display.c
 *
 *  Created on: march 31, 2026
 *      Author: Wendell
 */

#include "glcdc_display.h"


/* Variables to store resolution information */
volatile uint8_t g_vsync_flag = RESET_FLAG; //flag to store vsync event 

/* Variables used for buffer usage */
uint32_t g_buffer_size; 
//the size of the buffer, which is determined by the resolution of the display and the color format

extern uint16_t g_image_width;
//the width of the display,in our project ,we use 800*480 resolution, so the width is 800
extern uint16_t g_image_height;
//the height of the display,in our project ,we use 800*480 resolution, so the height is 480

/***********************************************************************************************************************
 *  Function Name: glcdc_init
 *  Description  : This function is used to initialize GLCDC module with selected resolution
 *  Arguments    : None
 *  Return Value : FSP_SUCCESS    Upon successful operation
 *                 Any Other Error code apart from FSP_SUCCESS
 **********************************************************************************************************************/
fsp_err_t glcdc_init(void)
{
    fsp_err_t err = FSP_SUCCESS;

    /* Close opened GLCDC module*/
    if(DISPLAY_STATE_CLOSED != g_display_ctrl.state)//if the display is not closed, then close it
    {
        err = R_GLCDC_Close(&g_display_ctrl);//close the display ,and return the error code if failed
        APP_ERR_RET( FSP_SUCCESS != err, err, "** R_GLCDC_Close API FAILED **");//if the close failed, then return the error code
    }
    display_cfg_t g_display_user_cfg;//initial the display configuration structure,and we will use it to initialize the display,we will set the resolution of the display in this structure
    memcpy(&g_display_user_cfg, &g_display_cfg, sizeof(display_cfg_t));//copy the default configuration to the user configuration structure, and then we will modify the user configuration structure according to our needs
    g_display_user_cfg.input[0].hsize = g_image_width;//set the horizontal size od the display,which is the width of the display, in out project, we use 800*480 resolution,so the width is 800
    g_display_user_cfg.input[0].vsize = g_image_height;//set the vertical size of the display,which is the height of the display, in out project, we use 800*480 resolution,so the height is 480

    /* Initialize GLCDC driver */
    err = R_GLCDC_Open(&g_display_ctrl, &g_display_user_cfg);//open the display with the user configuration, and return the error code if failed
    APP_ERR_RET( FSP_SUCCESS != err, err, "** R_GLCDC_Open API FAILED **");//if the open failed,the return the error code

    /* Start GLCDC display output */
    err = R_GLCDC_Start(&g_display_ctrl);//start the display output, and return the error if failed
    APP_ERR_RET( FSP_SUCCESS != err, err, "** R_GLCDC_Start API FAILED **");//if the start failed,then return the error code

    g_vsync_flag = RESET_FLAG;//reset the vsync flag before waiting for the vsync event
    /* Wait for a Vsync event */
    while(!g_vsync_flag);//wait for the vsync event, and the vsync event will be set in the vsync interrupt service routine,which will be called when the vsync event occurs

    return err;//return the error code, if the initialization is successful, then the error code will be FSP_SUCCESS
}
/***********************************************************************************************************************
* End of function glcdc_init
***********************************************************************************************************************/

/***********************************************************************************************************************
 *  Function Name: glcdc_vsync_isr
 *  Description  : This function is used to detect vsync interrupt.
 *  Arguments    : p_args      Pointer to callback argument
 *  Return Value : None
 **********************************************************************************************************************/
void glcdc_vsync_isr(display_callback_args_t *p_args)
{
    if (p_args->event == DISPLAY_EVENT_LINE_DETECTION)//if the event is line detection, which means the vsync event occurs, then set the vsync flag to SET_FLAG
    {
        g_vsync_flag = SET_FLAG;//set the vsync flag to SET_FLAG, which means the vsync event occurs
    }
}
/***********************************************************************************************************************
* End of function glcdc_vsync_isr
***********************************************************************************************************************/

