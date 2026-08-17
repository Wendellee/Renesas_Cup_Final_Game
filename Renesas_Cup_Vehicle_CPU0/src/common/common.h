/*
 * common.h
 *
 *  Created on: 2026年8月10日
 *      Author: lingk
 */

#ifndef COMMON_COMMON_H_
#define COMMON_COMMON_H_

#include "hal_data.h"

/* 
 *[@type] global variable 
*/
#define HARDWARE_DISPLAY_INIT_DONE                             (1UL << 0)
#define HARDWARE_CAMERA_INIT_DONE                              (1UL << 1)
#define HARDWARE_ETHOSU_INIT_DONE                              (1UL << 2)
#define SOFTWARE_AI_INFERENCE_INIT_DONE                        (1UL << 3)


#define GLCDC_VSYNC                                            (1UL << 10)
#define CAMERA_FRAME_READY                                     (1UL << 11)
#define CAMERA_CAPTURE_ERROR                                   (1UL << 12)
#define AI_INFERENCE_INPUT_IMAGE_READY                         (1UL << 13)
#define AI_INFERENCE_RESULT_UPDATED                            (1UL << 14)

#define APP_ERROR_TRAP(err)        if(err) { __asm("BKPT #0\n");} //system execution breaks

#endif /* COMMON_COMMON_H_ */
