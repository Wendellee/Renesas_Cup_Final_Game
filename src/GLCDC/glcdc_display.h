/*
 * glcdc_display.h
 *
 *  Created on: march 31, 2026
 *      Author: Wendell
 */

#ifndef GLCDC_DISPLAY_H_
#define GLCDC_DISPLAY_H_

#include "common_utils.h" 
//it's about RTT_viewer and terminal interface, 
// we can use it for debug output

// #include "../src/user_config.h" 
//this is renesas display config
//but we dont need it we need glcdc_display always be enabled for display output,
//so we can directly include glcdc_display.c in main.c and call glcdc_init() to initialize the display

// #if (DISPLAY_OUTPUT == 1U) //display always be enabled 

#define RESET_FLAG              (0U)
#define SET_FLAG                (1U)

/* Public function declarations */
fsp_err_t glcdc_init(void);

// #endif /* DISPLAY_OUTPUT */
#endif /* GLCDC_DISPLAY_H_ */
