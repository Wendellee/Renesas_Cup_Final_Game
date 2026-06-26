
/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GG_VIDEO_H
#define GG_VIDEO_H

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint8_t * data;     /* Pointer to data. */
    uint32_t len;        /* Length of the data. */
    bool isEndOfFile;   /* Has reached file end. */
    bool isStartOfFile; /* Is start of file. */
} file_data_block_t;


/** Data of the video */
typedef struct {
    lv_image_t img;
    int play_status;
    const char * file_name;
    lv_image_dsc_t frameImage;
    lv_fs_file_t h264File;
    file_data_block_t blk;
    bool fileStart;
    bool exist;
} gg_video_t;

LV_ATTRIBUTE_EXTERN_DATA extern const lv_obj_class_t gg_video_class;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * Play the video.
 * @param obj       pointer the video object
 */
void gg_video_play(lv_obj_t * obj);

/**
 * Create a video object
 * @param parent       pointer to an object
 * @param widgetWidth   the width of the video object.
 * @param widgetHeight  the height of the video object.
 * @return pointer to the created video.
 */
lv_obj_t * gg_video_create(lv_obj_t * parent, int widgetWidth, int widgetHeight);

/**
 * Init PXP for video.
 */
void gg_video_init_pxp(void);

/*======================
 * Add/remove functions
 *=====================*/

/*=====================
 * Setter functions
 *====================*/

/**
 * Set the video data to display on the object.
 * @param obj       pointer to a video object
 * @param src       path to the video h264 file (e.g. "S:/dir/video.h264")
 */
void gg_video_set_src(lv_obj_t * obj, const char * src);

/**
 * Set the video play status.
 * @param obj       pointer to a video object
 * @param status    0/1, 0 represents pause, and 1 represents play.
 */
void gg_video_set_status(lv_obj_t * obj, int status);
/*=====================
 * Getter functions
 *====================*/

/**
 * Get the play status.
 * @param obj      pointer to a video object
 * @return         the status of the current video.
 */
int gg_video_get_status(lv_obj_t * obj);

/**
 * Get the video h264 data file path.
 * @param obj      pointer to a video object
 * @return         path to the video h264 file.
 */
const char * gg_video_get_src(lv_obj_t * obj);

#endif
