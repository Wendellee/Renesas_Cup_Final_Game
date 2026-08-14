#include "ai_thread.h"
#include "common/common.h"
#include "Camera/camera_capture.h"
#include "AI/ai_preprocess.h"
#include "AI/yolo_postprocess.h"
#include "model/model.h"
#include "SEGGER_RTT/bsp_print.h"
#include "AI/ai_inference_result.h"
#include "ImageUpload/Image_JPEG_Encoder.h"
#include "IPC/shared_jpeg_cpu0.h"
#include <stddef.h>
#include <stdint.h>

#define AI_CONFIDENCE_THRESHOLD       (0.50f) /*最低检测置信度阈值*/
#define AI_NMS_IOU_THRESHOLD          (0.45f)
#define AI_JPEG_CLEAR_FRAME_COUNT     (10U)

/*
 *[@name] g_jpeg_cfg
 *[@type] static global variable
 *[@usage] 保存彩色错误快照的裁剪、缩放和JPEG质量参数
 */
static const image_jpeg_encode_cfg_t g_jpeg_cfg =
{
    .source_width         = 1024U,
    .source_height        = 600U,
    .source_stride_pixels = 1024U,

    .crop_x               = 212U,
    .crop_y               = 0U,
    .crop_width           = 600U,
    .crop_height          = 600U,

    .output_width         = 240U,
    .output_height        = 240U,
    .quality              = 60U
};

/*
 *[@name] g_ai_preprocess_horizontal_map
 *[@type] static global variable
 *[@usage] 保存Helium预处理使用的横向源像素索引，只允许AI Thread访问
 */
static uint16_t
    g_ai_preprocess_horizontal_map[AI_PREPROCESS_HORIZONTAL_MAP_LENGTH]
    BSP_ALIGN_VARIABLE(16);

/*
 *[@name] ai_thread_fatal_stop
 *[@type] static function
 *[@usage] 触发CPU0断点，调试器继续执行后使AI Thread保持阻塞
 *[@argument] error 需要记录和触发断点的FSP错误码
 *[@return] none，此函数不会返回
 */
static void ai_thread_fatal_stop(fsp_err_t error)
{
    APP_ERROR_TRAP(error);

    for(;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

/*
 *[@name] ai_thread_entry
 *[@type] thread entry function
 *[@usage] 初始化Ethos-U55与模型接口，等待Camera完成帧，执行Helium预处理、模型推理和YOLO后处理
 *[@argument] pvParameters FSP传入的线程参数，当前未使用
 *[@return] none
 */
void ai_thread_entry(void * pvParameters)
{
    fsp_err_t err;
    EventBits_t events;

    int8_t * p_model_input;
    int8_t * p_model_output;
    uint32_t last_processed_sequence = 0U;
    bool upload_event_latched = false;
    bool jpeg_job_outstanding = false;
    bool jpeg_publish_pending = false;
    uint32_t jpeg_publish_sequence = 0U;
    size_t jpeg_publish_size = 0U;
    uint16_t jpeg_job_confidence_milli = 0U;
    uint16_t jpeg_publish_confidence_milli = 0U;
    uint32_t clean_frame_count = 0U;

    FSP_PARAMETER_NOT_USED(pvParameters);

    err = ImageJpeg_AsyncInit();
    if(FSP_SUCCESS != err)
    {
        g_printf("[JPEG][ERR] Worker init failed: %u.\r\n",
                 (unsigned int) err);
        ai_thread_fatal_stop(err);
    }

    g_printf("[JPEG] Worker ready: 240x240 quality=%u rearm=%u.\r\n",
             (unsigned int) g_jpeg_cfg.quality,
             (unsigned int) AI_JPEG_CLEAR_FRAME_COUNT);

    err = RM_ETHOSU_Open(&g_rm_ethosu0_ctrl,
                         &g_rm_ethosu0_cfg);
    if(FSP_SUCCESS != err)
    {
        g_printf("[AI][ERR] RM_ETHOSU_Open failed: %u\r\n",
                 (unsigned int) err);

        ai_thread_fatal_stop(err);
    }

    (void) xEventGroupSetBits(g_ai_app_event,
                              HARDWARE_ETHOSU_INIT_DONE);

    p_model_input = GetModelInputPtr_x();
    p_model_output = GetModelOutputPtr_Identity_70374();

    if((NULL == p_model_input) ||
       (NULL == p_model_output))
    {
        g_printf("[AI][ERR] Model tensor pointer is NULL.\r\n");
        ai_thread_fatal_stop(FSP_ERR_INTERNAL);
    }

    (void) xEventGroupSetBits(g_ai_app_event,
                              SOFTWARE_AI_INFERENCE_INIT_DONE);

    events = xEventGroupWaitBits(g_ai_app_event,
                                 HARDWARE_CAMERA_INIT_DONE,
                                 pdFALSE,
                                 pdTRUE,
                                 portMAX_DELAY);

    if(0U == (events & HARDWARE_CAMERA_INIT_DONE))
    {
        g_printf("[AI][ERR] Camera initialization wait failed.\r\n");
        ai_thread_fatal_stop(FSP_ERR_INTERNAL);
    }

    for(;;)
    {
        uint8_t * p_completed_frame;
        uint32_t completed_sequence = 0U;

        ai_preprocess_status_t preprocess_status;

        yolo_detection_t detections[YOLO_MAX_DETECTIONS];
        int detection_count;
        bool jpeg_snapshot_ready = false;
        uint32_t jpeg_frame_sequence = 0U;
        size_t jpeg_size = 0U;

        if(!jpeg_publish_pending)
        {
            err = ImageJpeg_GetAsyncResult(
                &jpeg_frame_sequence,
                &jpeg_size);
            if(FSP_SUCCESS == err)
            {
                jpeg_publish_pending = true;
                jpeg_publish_sequence = jpeg_frame_sequence;
                jpeg_publish_size = jpeg_size;
                jpeg_publish_confidence_milli = jpeg_job_confidence_milli;
            }
            else if(FSP_ERR_IN_USE != err)
            {
                jpeg_job_outstanding = false;
                g_printf("[JPEG][ERR] Encode failed: %u, frame=%u.\r\n",
                         (unsigned int) err,
                         (unsigned int) jpeg_frame_sequence);
            }
        }

        if(jpeg_publish_pending)
        {
            const uint8_t * p_jpeg_data = NULL;
            size_t encoded_size = 0U;

            err = ImageJpeg_GetEncodedData(&p_jpeg_data, &encoded_size);
            if((FSP_SUCCESS == err) && (encoded_size == jpeg_publish_size))
            {
                shared_jpeg_cpu0_result_t const publish_result =
                    shared_jpeg_cpu0_publish(p_jpeg_data,
                                             encoded_size,
                                             jpeg_publish_sequence,
                                             jpeg_publish_confidence_milli);

                if((SHARED_JPEG_CPU0_SUCCESS == publish_result) ||
                   (SHARED_JPEG_CPU0_NOTIFY_PENDING == publish_result))
                {
                    jpeg_publish_pending = false;
                    jpeg_job_outstanding = false;
                }
                else if((SHARED_JPEG_CPU0_BUSY != publish_result) &&
                        (SHARED_JPEG_CPU0_NOT_INITIALIZED != publish_result))
                {
                    jpeg_publish_pending = false;
                    jpeg_job_outstanding = false;
                    g_printf("[SHM0][ERR] Publish failed: %u, frame=%u.\r\n",
                             (unsigned int) publish_result,
                             (unsigned int) jpeg_publish_sequence);
                }
            }
            else if(FSP_SUCCESS != err)
            {
                jpeg_publish_pending = false;
                jpeg_job_outstanding = false;
                g_printf("[SHM0][ERR] JPEG access failed: %u, frame=%u.\r\n",
                         (unsigned int) err,
                         (unsigned int) jpeg_publish_sequence);
            }
        }

        events = xEventGroupWaitBits(
            g_ai_app_event,
            AI_INFERENCE_INPUT_IMAGE_READY,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);

        if(0U == (events & AI_INFERENCE_INPUT_IMAGE_READY))
        {
            continue;
        }

        p_completed_frame =
            camera_completed_frame_get(&completed_sequence);

        if(NULL == p_completed_frame)
        {
            continue;
        }

        if(completed_sequence == last_processed_sequence)
        {
            continue;
        }

#if BSP_CFG_DCACHE_ENABLED

        SCB_InvalidateDCache_by_Addr(
            (uint32_t *) p_completed_frame,
            (int32_t) VIN_BYTES_PER_FRAME);

#endif

        preprocess_status =
            ai_preprocess_rgb565_to_int8(
                p_completed_frame,
                VIN_BYTES_PER_FRAME,
                p_model_input,
                AI_PREPROCESS_DESTINATION_BYTES,
                g_ai_preprocess_horizontal_map,
                AI_PREPROCESS_HORIZONTAL_MAP_LENGTH);

        if(AI_PREPROCESS_SUCCESS != preprocess_status)
        {
            g_printf("[AI][ERR] Preprocess failed: status=%u, frame=%u.\r\n",
                     (unsigned int) preprocess_status,
                     (unsigned int) completed_sequence);

            ai_thread_fatal_stop(FSP_ERR_INTERNAL);
        }

        /*
         * 在运行模型前复制同一VIN帧的240x240 RGB888快照。
         * 函数返回后JPEG Worker不再访问VIN原始帧。
         */
        if((!upload_event_latched) && (!jpeg_job_outstanding))
        {
            err = ImageJpeg_CaptureRgb565Snapshot(
                (const uint16_t *) p_completed_frame,
                &g_jpeg_cfg);

            if(FSP_SUCCESS == err)
            {
                jpeg_snapshot_ready = true;
            }
            else if(FSP_ERR_IN_USE != err)
            {
                g_printf("[JPEG][ERR] Snapshot failed: %u, frame=%u.\r\n",
                         (unsigned int) err,
                         (unsigned int) completed_sequence);
            }
        }

        p_completed_frame = NULL;

        RunModel(false);

        detection_count =
            yolo_decode_int8_output(
                p_model_output,
                detections,
                YOLO_MAX_DETECTIONS,
                AI_CONFIDENCE_THRESHOLD);

        detection_count =
            yolo_nms(detections,
                     detection_count,
                     AI_NMS_IOU_THRESHOLD);

        bool publish_ok = ai_inference_result_publish(completed_sequence, detections, detection_count);

        if(!publish_ok)
        {
            APP_ERROR_TRAP(FSP_ERR_INTERNAL);
        }

        __DMB();

        float max_confidence = 0.0f;

        for(int i = 0; i < detection_count; i++)
        {
            if(detections[i].score > max_confidence)
            {
                max_confidence = detections[i].score;
            }
        }

        if((detection_count > 0) &&
           (max_confidence >= AI_CONFIDENCE_THRESHOLD))
        {
            clean_frame_count = 0U;

            if((!upload_event_latched) && jpeg_snapshot_ready)
            {
                err = ImageJpeg_SubmitCapturedSnapshot(
                    g_jpeg_cfg.quality,
                    completed_sequence);

                if(FSP_SUCCESS == err)
                {
                    uint32_t confidence_milli =
                        (uint32_t) ((max_confidence * 1000.0f) + 0.5f);

                    if(confidence_milli > 1000U)
                    {
                        confidence_milli = 1000U;
                    }

                    jpeg_job_confidence_milli = (uint16_t) confidence_milli;
                    upload_event_latched = true;
                    jpeg_job_outstanding = true;
                }
                else
                {
                    g_printf("[JPEG][ERR] Queue failed: %u, frame=%u.\r\n",
                             (unsigned int) err,
                             (unsigned int) completed_sequence);
                }
            }
        }
        else if(upload_event_latched)
        {
            if(clean_frame_count < AI_JPEG_CLEAR_FRAME_COUNT)
            {
                clean_frame_count++;
            }

            if((clean_frame_count >= AI_JPEG_CLEAR_FRAME_COUNT) &&
               (!jpeg_job_outstanding))
            {
                upload_event_latched = false;
                clean_frame_count = 0U;
            }
        }

        (void)xEventGroupSetBits(g_ai_app_event, AI_INFERENCE_RESULT_UPDATED);
        last_processed_sequence = completed_sequence;

    }
}
