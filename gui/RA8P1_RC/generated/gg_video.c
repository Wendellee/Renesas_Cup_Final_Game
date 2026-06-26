
/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GG_VIDEO_C
#define GG_VIDEO_C

/*********************
 *      INCLUDES
 *********************/

#include "gg_video.h"

#if LV_USE_IMAGE == 0
#error "gg_video: lv_img is required. Enable it in lv_conf.h (LV_USE_IMAGE  1) "
#endif

#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// for run on target function.
#if !LV_USE_GUIDER_SIMULATOR
#include "h264_dec.h"
#if LV_USE_OS == LV_OS_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "sdcard.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_pxp.h"
#include "fsl_cache.h"
#else
#include <unistd.h>
#endif
#else
#include "decoder.h"
#include <unistd.h>
#endif

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &gg_video_class
#define LV_COLOR_SIZE 16
#ifndef DEMO_FILE_BUF_SIZE
#define DEMO_FILE_BUF_SIZE (16 * 1024)
#endif
#ifndef DEMO_DECODE_BUF_SIZE
#define DEMO_DECODE_BUF_SIZE (64 * 1024)
#endif

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"


/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static int16_t video_width = 0;
static int16_t video_height = 0;
static void gg_video_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void gg_video_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void Read_HFile(const char * fileName, lv_obj_t * obj);

#if !LV_USE_GUIDER_SIMULATOR && (LV_USE_OS == LV_OS_FREERTOS)
/* PXP Usage */

static void PXP_DisplayFrame(uint16_t width,
                             uint16_t height,
                             const uint8_t * Y,
                             const uint8_t * U,
                             const uint8_t * V,
                             uint32_t Y_Stride,
                             uint32_t UV_Stride,
                             lv_obj_t * obj);
#else
#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
static void scale_plane(uint8_t *src, int src_w, int src_h,
                        uint8_t *dst, int dst_w, int dst_h);
static void yuv420p_to_rgb565(uint8_t *py, uint8_t *pu,
                              uint8_t *pv, int width, int height,
                              uint16_t *rgb_output);
static void yuv420p_to_rgb565_scaled(uint8_t *src_y, uint8_t *src_u,
                                     uint8_t *src_v, int src_width, int src_height,
                                     uint16_t *dst_rgb, int dst_width, int dst_height);
static void CPU_DisplayFrame(SBufferInfo sDstBufInfo, unsigned char ** dst, lv_obj_t * obj);
#endif
/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t gg_video_class = {
    .base_class = &lv_image_class,
    .instance_size = sizeof(gg_video_t),
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .constructor_cb = gg_video_constructor,
    .destructor_cb = gg_video_destructor,
};

#if !LV_USE_GUIDER_SIMULATOR && (LV_USE_OS == LV_OS_FREERTOS)
/* PXP Usage */
static pxp_output_buffer_config_t s_pxpOutputBufferConfig;
static pxp_ps_buffer_config_t s_pxpPsBufferConfig;
static volatile uint8_t s_lcdActiveFbIdx = 0;
static int buffer_byte_per_pixel = 2;
static uint8_t * s_lcdBuffer[2];
#else
static int mallocInit = 0;
static uint8_t *rgb, *py, *pu, *pv;
#endif

static uint8_t s_decodeBuf[DEMO_DECODE_BUF_SIZE];

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * gg_video_create(lv_obj_t * parent, int widgetWidth, int widgetHeight)
{
    video_width = widgetWidth;
    video_height = widgetHeight;

#if !LV_USE_GUIDER_SIMULATOR  && (LV_USE_OS == LV_OS_FREERTOS)
    s_lcdBuffer[0] = malloc(widgetWidth * widgetHeight * buffer_byte_per_pixel);
    s_lcdBuffer[1] = malloc(widgetWidth * widgetHeight * buffer_byte_per_pixel);
#endif

    lv_obj_t * obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

void gg_video_play(lv_obj_t * obj)
{
    gg_video_t * video = (gg_video_t *)obj;
    Read_HFile(video->file_name, obj);
}

static int search_nalu(const uint8_t * data, int32_t len)
{
    int i;
    /* parse NALU 00 00 00 01 or 00 00 01 */
    for(i = 1; i < len - 4; i++) {
        if((data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 0 && data[i + 3] == 1) ||
                (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1)) {
            return i;
        }
    }

    if(data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1) {
        return i;
    }

    return -1;
}

int Decoder_Data(const uint8_t * data, int len, bool isStartOfFile, bool isEndOfFile, lv_obj_t * obj)
{
    gg_video_t * video = (gg_video_t *)obj;
    SBufferInfo sDstBufInfo = {0};
    int32_t copiedLen = 0;
    int32_t sliceSize = 0;
    uint8_t * dst[3];
    int32_t num_of_frames_in_buffer = 0;
    int32_t leftDataLen;

    static int32_t decodeBufStart = 0;
    static int32_t decodeBufEnd   = 0;

    if(isStartOfFile) {
        decodeBufStart         = 0;
        decodeBufEnd           = 0;
    }
    leftDataLen = len;
    while(leftDataLen > 0) {
        copiedLen = LV_MIN(leftDataLen, ((int32_t)sizeof(s_decodeBuf) - decodeBufEnd));

        /* Copy the input data to the end of decode buffer. */
        memcpy(&s_decodeBuf[decodeBufEnd], data, copiedLen);
        decodeBufEnd += copiedLen;
        data += copiedLen;
        leftDataLen -= copiedLen;
        while(decodeBufStart < decodeBufEnd) {
            sliceSize = search_nalu(&s_decodeBuf[decodeBufStart], decodeBufEnd - decodeBufStart);
            if(video->exist) {
                return 0;
            }
            /* Could not find NALU. */
            if(sliceSize < 0) {
                /* This is the file end part, pass them all to H264 decoder. */
                if(isEndOfFile && (0 == leftDataLen)) {
                    sliceSize = decodeBufEnd - decodeBufStart;
                }
                else {
                    /* After searching the full buffer, no slice found, then drop the data in buffer. */
                    if((decodeBufStart == 0) && (decodeBufEnd == sizeof(s_decodeBuf))) {
                        /* Drop the decode buffer, fill using left input data. */
                        decodeBufEnd   = 0;
                        decodeBufStart = 0;
                    }
                    /* Have processed all slice in the buffer. */
                    break;
                }
            }
            /* Slice size too small, skip it. */
            else if(sliceSize < 4) {

                decodeBufStart += sliceSize;
                continue;
            }
            /* Found NALU, decode. */
            if(OpenH264_Decode(&s_decodeBuf[decodeBufStart], sliceSize, dst, &sDstBufInfo) == 0) {

                if(sDstBufInfo.iBufferStatus == 1) {
#if !LV_USE_GUIDER_SIMULATOR && (LV_USE_OS == LV_OS_FREERTOS)
                    PXP_DisplayFrame(
                        sDstBufInfo.UsrData.sSystemBuffer.iWidth, sDstBufInfo.UsrData.sSystemBuffer.iHeight,
                        sDstBufInfo.pDst[0], sDstBufInfo.pDst[1], sDstBufInfo.pDst[2],
                        sDstBufInfo.UsrData.sSystemBuffer.iStride[0], sDstBufInfo.UsrData.sSystemBuffer.iStride[1], obj);
#else
                    CPU_DisplayFrame(sDstBufInfo, dst, obj);
#endif
                }

            }

            decodeBufStart += sliceSize;
        }
        /* Move the left data in decode buffer to the start of decode buffer,
         * left input data will be appended to the end.
         */
        memcpy(s_decodeBuf, &s_decodeBuf[decodeBufStart], decodeBufEnd - decodeBufStart);
        decodeBufEnd -= decodeBufStart;
        decodeBufStart = 0;

    }

    if(isEndOfFile) {
        OpenH264_GetOption(&num_of_frames_in_buffer);
        for(int32_t i = 0; i < num_of_frames_in_buffer; i++) {
            dst[0] = NULL;
            dst[1] = NULL;
            dst[2] = NULL;

            OpenH264_FlashFrame(dst, &sDstBufInfo);
            if(sDstBufInfo.iBufferStatus == 1) {
#if !LV_USE_GUIDER_SIMULATOR && (LV_USE_OS == LV_OS_FREERTOS)
                PXP_DisplayFrame(
                    sDstBufInfo.UsrData.sSystemBuffer.iWidth, sDstBufInfo.UsrData.sSystemBuffer.iHeight,
                    sDstBufInfo.pDst[0], sDstBufInfo.pDst[1], sDstBufInfo.pDst[2],
                    sDstBufInfo.UsrData.sSystemBuffer.iStride[0], sDstBufInfo.UsrData.sSystemBuffer.iStride[1], obj);
#else
                CPU_DisplayFrame(sDstBufInfo, dst, obj);
#endif
            }
        }
    }

    return 0;
}

void Read_HFile(const char * fileName, lv_obj_t * obj)
{
    gg_video_t * video = (gg_video_t *)obj;
    uint32_t bytesRead;
    while(1) {
        int error = lv_fs_open(&video->h264File, video->file_name, LV_FS_MODE_RD);
        if(error != LV_FS_RES_OK) {
            break;
        }
        video->fileStart = true;
        while(1) {
            if(video->play_status == 1) {
                error = lv_fs_read(&video->h264File, video->blk.data, DEMO_FILE_BUF_SIZE, &bytesRead);
                if(error != LV_FS_RES_OK) {
                    break;
                }
                video->blk.len           = bytesRead;
                video->blk.isEndOfFile   = (DEMO_FILE_BUF_SIZE > bytesRead);
                video->blk.isStartOfFile = video->fileStart;
                video->fileStart         = false;
                Decoder_Data(video->blk.data, video->blk.len, video->blk.isStartOfFile, video->blk.isEndOfFile, obj);
                if(video->blk.isEndOfFile) {
                    break;
                }
                if(video->exist) {
                    video->blk.data = NULL;
                    lv_fs_close(&video->h264File);
                    return;
                }
            }
            else {
#if !LV_USE_GUIDER_SIMULATOR && (LV_USE_OS == LV_OS_FREERTOS)
                vTaskDelay(5);
#else
#if LV_USE_GUIDER_SIMULATOR
                lv_timer_handler();
#endif
                usleep(5 * 1000);
#endif
            }
        }
        lv_fs_close(&video->h264File);
    }
}

#if !LV_USE_GUIDER_SIMULATOR && (LV_USE_OS == LV_OS_FREERTOS)
void gg_video_init_pxp(void)
{
    /* Initialize variables. */


    memset(&s_pxpPsBufferConfig, 0, sizeof(s_pxpPsBufferConfig));
    memset(&s_pxpOutputBufferConfig, 0, sizeof(s_pxpOutputBufferConfig));
    memset(s_lcdBuffer[0], 0, video_width * video_height * buffer_byte_per_pixel);
    memset(s_lcdBuffer[1], 0, video_width * video_height * buffer_byte_per_pixel);
    s_pxpPsBufferConfig.pixelFormat = kPXP_PsPixelFormatYVU420;
    s_pxpPsBufferConfig.swapByte    = false,

    s_pxpOutputBufferConfig.pixelFormat    = kPXP_OutputPixelFormatRGB565;
    s_pxpOutputBufferConfig.interlacedMode = kPXP_OutputProgressive;
    s_pxpOutputBufferConfig.buffer1Addr    = 0U,
    s_pxpOutputBufferConfig.pitchBytes     = (video_width * buffer_byte_per_pixel);

    /* Initialize hardware. */
    PXP_Init(PXP);

    PXP_SetProcessSurfaceBackGroundColor(PXP, 0U);

    /* Disable AS. */
    PXP_SetAlphaSurfacePosition(PXP, 0xFFFFU, 0xFFFFU, 0U, 0U);

    PXP_SetCsc1Mode(PXP, kPXP_Csc1YCbCr2RGB);
    PXP_EnableCsc1(PXP, true);
}

static void PXP_DisplayFrame(uint16_t width,
                             uint16_t height,
                             const uint8_t * Y,
                             const uint8_t * U,
                             const uint8_t * V,
                             uint32_t Y_Stride,
                             uint32_t UV_Stride,
                             lv_obj_t * obj)
{
    gg_video_t * video = (gg_video_t *)obj;
    void * lcdFrameAddr;
    bool rotate                    = false;
    static uint16_t oldInputWidth  = 0U;
    static uint16_t oldInputHeight = 0U;

    uint16_t lcdWidth  = video_width;
    uint16_t lcdHeight = video_height;

    DCACHE_CleanInvalidateByRange((uint32_t)Y, height * Y_Stride);
    DCACHE_CleanInvalidateByRange((uint32_t)U, height * UV_Stride / 2);
    DCACHE_CleanInvalidateByRange((uint32_t)V, height * UV_Stride / 2);

    /* PS configure. */
    s_pxpPsBufferConfig.bufferAddr  = (uint32_t)Y;
    s_pxpPsBufferConfig.bufferAddrU = (uint32_t)U;
    s_pxpPsBufferConfig.bufferAddrV = (uint32_t)V;
    s_pxpPsBufferConfig.pitchBytes  = Y_Stride;

    PXP_SetProcessSurfaceBufferConfig(PXP, &s_pxpPsBufferConfig);

    /* Input frame size changed. */
    if((oldInputHeight != height) || (oldInputWidth != width)) {

        rotate = (height > width);

        if(rotate) {
            s_pxpOutputBufferConfig.width  = lcdHeight;
            s_pxpOutputBufferConfig.height = lcdWidth;

            PXP_SetRotateConfig(PXP, kPXP_RotateOutputBuffer, kPXP_Rotate90, kPXP_FlipDisable);
            PXP_SetProcessSurfaceScaler(PXP, width, height, lcdHeight, lcdWidth);
            PXP_SetProcessSurfacePosition(PXP, 0, 0, lcdHeight - 1, lcdWidth - 1);
        }
        else {
            s_pxpOutputBufferConfig.width  = lcdWidth;
            s_pxpOutputBufferConfig.height = lcdHeight;

            PXP_SetProcessSurfaceScaler(PXP, width, height, lcdWidth, lcdHeight);
            PXP_SetProcessSurfacePosition(PXP, 0, 0, lcdWidth - 1, lcdHeight - 1);
        }

        oldInputHeight = height;
        oldInputWidth  = width;
    }

    lcdFrameAddr                              = s_lcdBuffer[s_lcdActiveFbIdx ^ 1];
    s_pxpOutputBufferConfig.buffer0Addr = (uint32_t)lcdFrameAddr;

    PXP_SetOutputBufferConfig(PXP, &s_pxpOutputBufferConfig);
    PXP_Start(PXP);

    while(0U == (kPXP_CompleteFlag & PXP_GetStatusFlags(PXP))) {
    }
    PXP_ClearStatusFlags(PXP, kPXP_CompleteFlag);

    video->frameImage.data = lcdFrameAddr;
    s_lcdActiveFbIdx ^= 1;
    lv_lock();
    lv_image_set_src(obj, &video->frameImage);
    lv_unlock();
}
#else

static void CPU_DisplayFrame(SBufferInfo sDstBufInfo, unsigned char ** dst, lv_obj_t * obj)
{
    gg_video_t * video = (gg_video_t *)obj;
    int width = 0;
    int height = 0;
    if(sDstBufInfo.iBufferStatus == 1) {
        width = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
        height = sDstBufInfo.UsrData.sSystemBuffer.iHeight;
        int YStride = sDstBufInfo.UsrData.sSystemBuffer.iStride[0];
        int UVStride = sDstBufInfo.UsrData.sSystemBuffer.iStride[1];
        unsigned char * pPtr = NULL;
        if(mallocInit == 0) {
            rgb = malloc(2 * video_width * video_height);
            py = malloc(width * height);
            pu = malloc(width * height / 4);
            pv = malloc(width * height / 4);
            mallocInit = 1;
        }
        pPtr = dst[0];
        for(int i = 0; i < height; i++) {
            memcpy(py + i * width, pPtr, width);
            pPtr += YStride;
        }

        height = height / 2;
        width = width / 2;
        pPtr = dst[1];
        for(int i = 0; i < height; i++) {
            memcpy(pu + i * width, pPtr, width);
            pPtr += UVStride;
        }
        pPtr = dst[2];
        for(int i = 0; i < height; i++) {
            memcpy(pv + i * width, pPtr, width);
            pPtr += UVStride;
        }
        yuv420p_to_rgb565_scaled(py, pu, pv, width * 2, height * 2, (uint16_t*)rgb, video_width, video_height);
        // update the image data.
        video->frameImage.data = rgb;
        lv_lock();
        lv_image_set_src(obj, &video->frameImage);
        lv_image_cache_drop(lv_image_get_src(obj));
        lv_obj_invalidate(obj);
        lv_unlock();
        lv_timer_handler();
        usleep(30 * 1000);
    }
}

static void scale_plane(uint8_t *src, int src_w, int src_h,
                        uint8_t *dst, int dst_w, int dst_h)
{
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            float src_x = (x + 0.5f) * src_w / dst_w - 0.5f;
            float src_y = (y + 0.5f) * src_h / dst_h - 0.5f;

            int x0 = (int)floorf(src_x);
            int y0 = (int)floorf(src_y);
            int x1 = x0 + 1;
            int y1 = y0 + 1;

            x0 = CLAMP(x0, 0, src_w - 1);
            x1 = CLAMP(x1, 0, src_w - 1);
            y0 = CLAMP(y0, 0, src_h - 1);
            y1 = CLAMP(y1, 0, src_h - 1);

            float dx = src_x - x0;
            float dy = src_y - y0;

            uint8_t v00 = src[y0 * src_w + x0];
            uint8_t v01 = src[y0 * src_w + x1];
            uint8_t v10 = src[y1 * src_w + x0];
            uint8_t v11 = src[y1 * src_w + x1];

            float val = (1 - dx) * (1 - dy) * v00
                        + dx * (1 - dy) * v01
                        + (1 - dx) * dy * v10
                        + dx * dy * v11;
            dst[y * dst_w + x] = (uint8_t)(val + 0.5f);
        }
    }
}

static void yuv420p_to_rgb565(uint8_t *py, uint8_t *pu, uint8_t *pv,
                              int width, int height, uint16_t *rgb_output)
{
    int uv_width = width / 2;
    int uv_height = height / 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t Y = py[y * width + x];

            int uv_x = x / 2;
            int uv_y = y / 2;
            uv_x = CLAMP(uv_x, 0, uv_width - 1);
            uv_y = CLAMP(uv_y, 0, uv_height - 1);

            uint8_t U = pu[uv_y * uv_width + uv_x];
            uint8_t V = pv[uv_y * uv_width + uv_x];

            // YUV to RGB（Based on ITU-R BT.601）
            int Y_val = Y - 16;
            int U_val = U - 128;
            int V_val = V - 128;

            int R = (298 * Y_val + 409 * V_val + 128) >> 8;
            int G = (298 * Y_val - 100 * U_val - 208 * V_val + 128) >> 8;
            int B = (298 * Y_val + 516 * U_val + 128) >> 8;

            R = CLAMP(R, 0, 255);
            G = CLAMP(G, 0, 255);
            B = CLAMP(B, 0, 255);

            uint16_t rgb = ((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3);
            rgb_output[y * width + x] = rgb;
        }
    }
}

static void yuv420p_to_rgb565_scaled(uint8_t *src_y, uint8_t *src_u, uint8_t *src_v,
                                     int src_width, int src_height, uint16_t *dst_rgb,
                                     int dst_width, int dst_height)
{
    // check if the dst width and height is even
    if (dst_width % 2 != 0 || dst_height % 2 != 0) {
        return;
    }

    // malloc memory for scaled planes
    uint8_t *scaled_y = malloc(dst_width * dst_height);
    uint8_t *scaled_u = malloc((dst_width/2) * (dst_height/2));
    uint8_t *scaled_v = malloc((dst_width/2) * (dst_height/2));

    if (!scaled_y || !scaled_u || !scaled_v) {
        free(scaled_y);
        free(scaled_u);
        free(scaled_v);
        return;
    }

    scale_plane(src_y, src_width, src_height, scaled_y, dst_width, dst_height);
    scale_plane(src_u, src_width/2, src_height/2, scaled_u, dst_width/2, dst_height/2);
    scale_plane(src_v, src_width/2, src_height/2, scaled_v, dst_width/2, dst_height/2);

    yuv420p_to_rgb565(scaled_y, scaled_u, scaled_v, dst_width, dst_height, dst_rgb);

    free(scaled_y);
    free(scaled_u);
    free(scaled_v);
}
#endif

/*=====================
 * Setter functions
 *====================*/

void gg_video_set_src(lv_obj_t * obj, const char * src)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    gg_video_t * video = (gg_video_t *)obj;

    video->file_name = src;
}

void gg_video_set_status(lv_obj_t * obj, int status)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    gg_video_t * video = (gg_video_t *)obj;

    video->play_status = status;
}

/*=====================
 * Getter functions
 *====================*/
int gg_video_get_status(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    gg_video_t * video = (gg_video_t *)obj;

    return video->play_status;
}


const char * gg_video_get_src(lv_obj_t * obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    gg_video_t * video = (gg_video_t *)obj;

    return video->file_name;
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

static void gg_video_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    gg_video_t * video = (gg_video_t *)obj;

    video->play_status = 1;
    video->exist = false;
    OpenH264_Init();
    video->frameImage.header.cf = LV_COLOR_FORMAT_RGB565;
    video->frameImage.header.magic = LV_IMAGE_HEADER_MAGIC;
    video->frameImage.header.w = video_width;
    video->frameImage.header.h = video_height;
    video->frameImage.data_size = video_width * video_height * LV_COLOR_SIZE / 8;
    video->blk.data = (uint8_t *)malloc(DEMO_FILE_BUF_SIZE + 4);
    video->frameImage.data = NULL;
    video->fileStart = true;
    lv_obj_update_layout(obj);
}

static void gg_video_destructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_UNUSED(class_p);
    gg_video_t * video = (gg_video_t *)obj;
    video->exist = true;
    if(video->blk.data) {
        free(video->blk.data);
        video->blk.data = NULL;
    }

#if !LV_USE_GUIDER_SIMULATOR && (LV_USE_OS == LV_OS_FREERTOS)
    if(s_lcdBuffer[0] != NULL && s_lcdBuffer[1] != NULL) {
        free(s_lcdBuffer[0]);
        free(s_lcdBuffer[1]);
        s_lcdBuffer[0] = NULL;
        s_lcdBuffer[1] = NULL;
    }
#else
    mallocInit = 0;
    if(rgb) {
        free(py);
        free(pu);
        free(pv);
        free(rgb);
    }
#endif
    lv_image_cache_drop(lv_image_get_src(obj));
}

#endif
