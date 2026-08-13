#ifndef CAMERA_CAPTURE_H_
#define CAMERA_CAPTURE_H_

#include "hal_data.h"

#define CAMERA_CAPTURE_WIDTH       (1024U)
#define CAMERA_CAPTURE_HEIGHT      (600U)
#define CAMERA_CAPTURE_STRIDE      (VIN_CFG_BYTES_PER_LINE)
#define CAMERA_CAPTURE_PIXEL_BYTES (2U)

fsp_err_t CameraCapture_Init(void);
bool CameraCapture_GetLatestFrame(uint8_t const ** pp_frame, uint32_t * p_sequence);
uint32_t CameraCapture_FrameCountGet(void);
uint32_t CameraCapture_ErrorCountGet(void);

#endif
