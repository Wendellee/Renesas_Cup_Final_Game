#include "Image_JPEG_Encoder.h"
#include <stdbool.h>
#include <string.h>

#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ThirdParty/stb_image_write.h"

/*=================================引入Helium指令集技术==================================*/
#if !defined(__ARM_FEATURE_MVE) || ((__ARM_FEATURE_MVE & 1) == 0)
#error "Arm Helium MVE is not enabled for Image_JPEG_Encoder.c"
#endif

#include <arm_mve.h>
#define IMAGE_MVE_PIXELS_PER_VECTOR    (8U)//Helium 每次处理的向量像素数
/*======================================================================================*/

/*==================本项目使用240*240_RGB888图像编码,64KiB为JPEG最大数出===================*/
#define IMAGE_UPLOAD_WIDTH             (240U)
#define IMAGE_UPLOAD_HEIGHT            (240U)

#define IMAGE_RGB888_WORKSPACE_SIZE    \
    (IMAGE_UPLOAD_WIDTH * IMAGE_UPLOAD_HEIGHT * 3U)

#define IMAGE_JPEG_MAX_SIZE            (64U * 1024U)
#define IMAGE_JPEG_WORKER_STACK_BYTES  (0x2000U)
#define IMAGE_JPEG_WORKER_PRIORITY     (0U)
/*=======================================================================================*/

/*=================================将数组放置在片外SDRAM中=================================*/
static uint8_t g_upload_rgb888[IMAGE_RGB888_WORKSPACE_SIZE]
    BSP_ALIGN_VARIABLE(64)
    BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");

static uint8_t g_upload_jpeg[IMAGE_JPEG_MAX_SIZE]
    BSP_ALIGN_VARIABLE(64)
    BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".sdram_noinit");
/*
 * 最近一次成功发布的 JPEG 有效长度。
 * 零表示当前没有可供外部读取的完整 JPEG。
 */
static size_t g_upload_jpeg_size = 0U;
static bool g_rgb888_snapshot_valid = false;

typedef enum e_image_jpeg_async_state
{
    IMAGE_JPEG_ASYNC_IDLE = 0,
    IMAGE_JPEG_ASYNC_QUEUED,
    IMAGE_JPEG_ASYNC_ENCODING,
    IMAGE_JPEG_ASYNC_COMPLETE
} image_jpeg_async_state_t;

static StaticTask_t g_image_jpeg_worker_tcb;
static StackType_t g_image_jpeg_worker_stack[IMAGE_JPEG_WORKER_STACK_BYTES / sizeof(StackType_t)]
    BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
static TaskHandle_t g_image_jpeg_worker_handle = NULL;
static volatile image_jpeg_async_state_t g_image_jpeg_async_state = IMAGE_JPEG_ASYNC_IDLE;
static uint8_t g_image_jpeg_pending_quality = 0U;
static uint32_t g_image_jpeg_pending_sequence = 0U;
static size_t g_image_jpeg_async_size = 0U;
static fsp_err_t g_image_jpeg_async_error = FSP_ERR_NOT_INITIALIZED;
/*========================================================================================*/

/*===================================JPEG写入上下文结构体===================================*/
typedef struct st_image_jpeg_write_context
{
    uint8_t * p_buffer;      /* JPEG 输出缓冲区。 */
    size_t capacity;         /* 缓冲区总容量。 */
    size_t size;             /* 当前已写入字节数。 */
    bool overflow;           /* 容量不足标志。 */
} image_jpeg_write_context_t;
/*========================================================================================*/

#define IMAGE_JPEG_RGB_COMPONENTS    (3)
#define IMAGE_JPEG_GRAY_COMPONENTS   (1)
#define IMAGE_JPEG_QUALITY_MIN       (1U)
#define IMAGE_JPEG_QUALITY_MAX       (100U)

static void image_jpeg_write_callback(
    void * p_context,
    void * p_data,
    int data_size);
static void image_jpeg_worker_entry(void * p_context);

/*
 *[@name] ImageJpeg_AsyncInit
 *[@type] function
 *[@usage] 创建静态低优先级JPEG后台任务，由AI Thread在调度器启动后调用一次
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，任务创建失败返回FSP_ERR_OUT_OF_MEMORY
 */
fsp_err_t ImageJpeg_AsyncInit(void)
{
    if(NULL != g_image_jpeg_worker_handle)
    {
        return FSP_SUCCESS;
    }

    g_image_jpeg_worker_handle = xTaskCreateStatic(
        image_jpeg_worker_entry,
        "JPEG Worker",
        IMAGE_JPEG_WORKER_STACK_BYTES / sizeof(StackType_t),
        NULL,
        IMAGE_JPEG_WORKER_PRIORITY,
        g_image_jpeg_worker_stack,
        &g_image_jpeg_worker_tcb);

    if(NULL == g_image_jpeg_worker_handle)
    {
        return FSP_ERR_OUT_OF_MEMORY;
    }

    return FSP_SUCCESS;
}

fsp_err_t ImageJpeg_EncodeGray8(
    const uint8_t * p_gray8,
    uint16_t width,
    uint16_t height,
    uint8_t quality,
    uint8_t * p_jpeg_output,
    size_t jpeg_output_capacity,
    size_t * p_jpeg_size)
{
    image_jpeg_write_context_t write_context;

    if((NULL == p_gray8) || (NULL == p_jpeg_output) || (NULL == p_jpeg_size) ||
       (0U == width) || (0U == height) || (0U == jpeg_output_capacity) ||
       (quality < IMAGE_JPEG_QUALITY_MIN) || (quality > IMAGE_JPEG_QUALITY_MAX))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    *p_jpeg_size = 0U;
    write_context.p_buffer = p_jpeg_output;
    write_context.capacity = jpeg_output_capacity;
    write_context.size = 0U;
    write_context.overflow = false;
    int const encode_result = stbi_write_jpg_to_func(
        image_jpeg_write_callback,
        &write_context,
        (int) width,
        (int) height,
        IMAGE_JPEG_GRAY_COMPONENTS,
        p_gray8,
        (int) quality);

    if((0 == encode_result) || write_context.overflow ||
       (write_context.size < 4U))
    {
        return write_context.overflow ? FSP_ERR_OVERFLOW : FSP_ERR_INTERNAL;
    }
    *p_jpeg_size = write_context.size;
    return FSP_SUCCESS;
}

/*
 *[@name] ImageJpeg_SubmitCapturedSnapshot
 *[@type] function
 *[@usage] 原子发布质量和帧序号，并使用任务通知唤醒JPEG后台任务
 *[@argument] quality JPEG质量，范围1到100
 *[@argument] frame_sequence JPEG对应的摄像头帧序号
 *[@return] 提交成功返回FSP_SUCCESS，任务忙或参数无效时返回对应FSP错误码
 */
fsp_err_t ImageJpeg_SubmitCapturedSnapshot(
    uint8_t quality,
    uint32_t frame_sequence)
{
    if(NULL == g_image_jpeg_worker_handle)
    {
        return FSP_ERR_NOT_INITIALIZED;
    }

    if((quality < IMAGE_JPEG_QUALITY_MIN) ||
       (quality > IMAGE_JPEG_QUALITY_MAX))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    taskENTER_CRITICAL();
    if(IMAGE_JPEG_ASYNC_IDLE != g_image_jpeg_async_state)
    {
        taskEXIT_CRITICAL();
        return FSP_ERR_IN_USE;
    }

    if(!g_rgb888_snapshot_valid)
    {
        taskEXIT_CRITICAL();
        return FSP_ERR_NOT_INITIALIZED;
    }

    g_image_jpeg_pending_quality = quality;
    g_image_jpeg_pending_sequence = frame_sequence;
    g_image_jpeg_async_size = 0U;
    g_image_jpeg_async_error = FSP_ERR_IN_USE;
    g_image_jpeg_async_state = IMAGE_JPEG_ASYNC_QUEUED;
    taskEXIT_CRITICAL();

    (void) xTaskNotifyGive(g_image_jpeg_worker_handle);
    return FSP_SUCCESS;
}

/*
 *[@name] ImageJpeg_GetAsyncResult
 *[@type] function
 *[@usage] 原子读取后台编码结果，并在结果被取走后恢复IDLE状态
 *[@argument] p_frame_sequence 返回已完成JPEG对应的摄像头帧序号
 *[@argument] p_jpeg_size 返回有效JPEG长度，单位为字节
 *[@return] 返回后台编码结果，尚未完成时返回FSP_ERR_IN_USE
 */
fsp_err_t ImageJpeg_GetAsyncResult(
    uint32_t * p_frame_sequence,
    size_t * p_jpeg_size)
{
    fsp_err_t result;

    if((NULL == p_frame_sequence) || (NULL == p_jpeg_size))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    taskENTER_CRITICAL();
    if(IMAGE_JPEG_ASYNC_COMPLETE != g_image_jpeg_async_state)
    {
        taskEXIT_CRITICAL();
        return FSP_ERR_IN_USE;
    }

    *p_frame_sequence = g_image_jpeg_pending_sequence;
    *p_jpeg_size = g_image_jpeg_async_size;
    result = g_image_jpeg_async_error;
    g_image_jpeg_async_state = IMAGE_JPEG_ASYNC_IDLE;
    taskEXIT_CRITICAL();

    return result;
}

/*
 *[@name] image_jpeg_worker_entry
 *[@type] static thread entry function
 *[@usage] 等待任务通知，在低优先级任务中执行软件JPEG编码并原子发布结果
 *[@argument] p_context FreeRTOS任务上下文，当前未使用
 *[@return] none
 */
static void image_jpeg_worker_entry(void * p_context)
{
    FSP_PARAMETER_NOT_USED(p_context);

    for(;;)
    {
        size_t jpeg_size = 0U;
        fsp_err_t err;

        (void) ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        taskENTER_CRITICAL();
        g_image_jpeg_async_state = IMAGE_JPEG_ASYNC_ENCODING;
        taskEXIT_CRITICAL();

        err = ImageJpeg_EncodeCapturedSnapshot(
            g_image_jpeg_pending_quality,
            &jpeg_size);

        taskENTER_CRITICAL();
        g_image_jpeg_async_size = jpeg_size;
        g_image_jpeg_async_error = err;
        g_image_jpeg_async_state = IMAGE_JPEG_ASYNC_COMPLETE;
        taskEXIT_CRITICAL();
    }
}

/*
 *[@name] ImageJpeg_CaptureRgb565Snapshot
 *[@type] function
 *[@usage] 将RGB565源帧转换到模块私有RGB888快照，返回后编码阶段不再持有源帧
 *[@argument] p_source_rgb565 稳定RGB565帧首地址
 *[@argument] p_cfg 裁剪、缩放和输出尺寸配置
 *[@return] 成功返回FSP_SUCCESS，否则返回对应错误码
 */
fsp_err_t ImageJpeg_CaptureRgb565Snapshot(
    const uint16_t * p_source_rgb565,
    const image_jpeg_encode_cfg_t * p_cfg)
{
    fsp_err_t err;

    taskENTER_CRITICAL();
    if(IMAGE_JPEG_ASYNC_IDLE != g_image_jpeg_async_state)
    {
        taskEXIT_CRITICAL();
        return FSP_ERR_IN_USE;
    }
    taskEXIT_CRITICAL();

    g_rgb888_snapshot_valid = false;

    if((NULL == p_source_rgb565) || (NULL == p_cfg))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    if((IMAGE_UPLOAD_WIDTH != p_cfg->output_width) || (IMAGE_UPLOAD_HEIGHT != p_cfg->output_height))
    {
        return FSP_ERR_INVALID_SIZE;
    }

    err = ImageJpeg_ConvertRgb565ToRgb888Helium(
        p_source_rgb565,
        p_cfg,
        g_upload_rgb888,
        sizeof(g_upload_rgb888));
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    __DMB();
    g_rgb888_snapshot_valid = true;
    return FSP_SUCCESS;
}

/*
 *[@name] ImageJpeg_ConvertRgb565ToRgb888Scalar
 *[@type] function
 *[@usage] 使用标量C代码完成RGB565裁剪、最近邻缩放和RGB888转换，作为Helium版本的正确性参考
 *[@argument] p_source_rgb565 RGB565源帧缓冲区首地址
 *[@argument] p_cfg 源尺寸、行步长、裁剪区域和输出尺寸配置
 *[@argument] p_destination_rgb888 RGB888输出缓冲区首地址
 *[@argument] destination_size RGB888输出缓冲区容量，单位为字节
 *[@return] 转换成功返回FSP_SUCCESS，否则返回参数、尺寸或缓冲区错误码
 */
fsp_err_t ImageJpeg_ConvertRgb565ToRgb888Scalar(
    const uint16_t * p_source_rgb565,
    const image_jpeg_encode_cfg_t * p_cfg,
    uint8_t * p_destination_rgb888,
    size_t destination_size)
{
    size_t required_size;

    if(NULL == p_source_rgb565 ||
       NULL ==p_cfg ||
       NULL == p_destination_rgb888)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    if ((0U == p_cfg->source_width) ||
        (0U == p_cfg->source_height) ||
        (0U == p_cfg->source_stride_pixels) ||
        (0U == p_cfg->crop_width) ||
        (0U == p_cfg->crop_height) ||
        (0U == p_cfg->output_width) ||
        (0U == p_cfg->output_height))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    if (p_cfg->source_stride_pixels < p_cfg->source_width)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    if ((((uint32_t) p_cfg->crop_x + p_cfg->crop_width) >
         p_cfg->source_width) ||
        (((uint32_t) p_cfg->crop_y + p_cfg->crop_height) >
         p_cfg->source_height))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    if ((size_t) p_cfg->output_width >
        (SIZE_MAX / (size_t) p_cfg->output_height))
    {
        return FSP_ERR_INVALID_SIZE;
    }

    required_size =
        (size_t) p_cfg->output_width *
        (size_t) p_cfg->output_height;

    if (required_size > (SIZE_MAX / 3U))
    {
        return FSP_ERR_INVALID_SIZE;
    }

    required_size *= 3U;

    if (destination_size < required_size)
    {
        return FSP_ERR_INVALID_SIZE;
    }

    for (uint32_t destination_y = 0U;
         destination_y < p_cfg->output_height;
         destination_y++)
    {
        uint32_t source_y =
            (uint32_t) p_cfg->crop_y +
            ((destination_y * p_cfg->crop_height) /
             p_cfg->output_height);

        for (uint32_t destination_x = 0U;
             destination_x < p_cfg->output_width;
             destination_x++)
        {
            uint32_t source_x =
                (uint32_t) p_cfg->crop_x +
                ((destination_x * p_cfg->crop_width) /
                 p_cfg->output_width);

            size_t source_index =
                ((size_t) source_y *
                 p_cfg->source_stride_pixels) +
                source_x;

            size_t destination_index =
                (((size_t) destination_y *
                  p_cfg->output_width) +
                 destination_x) * 3U;

            uint16_t rgb565 = p_source_rgb565[source_index];

            uint8_t red_5 =
                (uint8_t) ((rgb565 >> 11U) & 0x1FU);

            uint8_t green_6 =
                (uint8_t) ((rgb565 >> 5U) & 0x3FU);

            uint8_t blue_5 =
                (uint8_t) (rgb565 & 0x1FU);

            /*
             * 位复制扩展比单纯左移更接近完整的 0～255 映射。
             */
            p_destination_rgb888[destination_index + 0U] =
                (uint8_t) ((red_5 << 3U) | (red_5 >> 2U));

            p_destination_rgb888[destination_index + 1U] =
                (uint8_t) ((green_6 << 2U) | (green_6 >> 4U));

            p_destination_rgb888[destination_index + 2U] =
                (uint8_t) ((blue_5 << 3U) | (blue_5 >> 2U));
        }
    }

    return FSP_SUCCESS;
}


/*
 *[@name] ImageJpeg_ConvertRgb565ToRgb888Helium
 *[@type] function
 *[@usage] 使用Arm Helium MVE完成RGB565裁剪、最近邻缩放和RGB888转换
 *[@argument] p_source_rgb565 RGB565源帧缓冲区首地址
 *[@argument] p_cfg 源尺寸、行步长、裁剪区域和输出尺寸配置
 *[@argument] p_destination_rgb888 RGB888输出缓冲区首地址
 *[@argument] destination_size RGB888输出缓冲区容量，单位为字节
 *[@return] 转换成功返回FSP_SUCCESS，否则返回参数、尺寸或缓冲区错误码
 */
fsp_err_t ImageJpeg_ConvertRgb565ToRgb888Helium(
    const uint16_t * p_source_rgb565,
    const image_jpeg_encode_cfg_t * p_cfg,
    uint8_t * p_destination_rgb888,
    size_t destination_size)
{
    /* RGB888 每像素占 3 字节，此表给出连续八个像素相同颜色通道的字节偏移 */
    static const uint16_t rgb888_byte_offsets[IMAGE_MVE_PIXELS_PER_VECTOR]
        BSP_ALIGN_VARIABLE(16) =
    {
        0U, 3U, 6U, 9U, 12U, 15U, 18U, 21U
    };

    size_t required_size;

    /* 检查所有必需指针，防止访问空地址 */
    //***** 检查必需指针，防止访问空地址。 *****
    if ((NULL == p_source_rgb565) ||
        (NULL == p_cfg) ||
        (NULL == p_destination_rgb888))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* 检查尺寸和步长，同时避免后续缩放计算除以零 */
    if ((0U == p_cfg->source_width) ||
        (0U == p_cfg->source_height) ||
        (0U == p_cfg->source_stride_pixels) ||
        (0U == p_cfg->crop_width) ||
        (0U == p_cfg->crop_height) ||
        (0U == p_cfg->output_width) ||
        (0U == p_cfg->output_height))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* 每行内存跨度不能小于一行有效像素数量 */
    if (p_cfg->source_stride_pixels < p_cfg->source_width)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* 确保裁剪区域完全位于源图像内部 */
    if ((((uint32_t) p_cfg->crop_x + p_cfg->crop_width) > p_cfg->source_width) ||
        (((uint32_t) p_cfg->crop_y + p_cfg->crop_height) > p_cfg->source_height))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* 在乘法前检查 width × height 是否会超出 size_t 范围
     * required_size = output_width × output_height × 3
     * if:a > SIZE_MAX / b; else: a * b > SIZE_MAX >>溢出
    */
    if ((size_t) p_cfg->output_width >
        (SIZE_MAX / (size_t) p_cfg->output_height))
    {
        return FSP_ERR_INVALID_SIZE;
    }

    required_size = (size_t) p_cfg->output_width * (size_t) p_cfg->output_height;

    /* RGB888 每像素占 3 字节，检查乘以 3 是否溢出 */
    if (required_size > (SIZE_MAX / 3U))
    {
        return FSP_ERR_INVALID_SIZE;
    }

    required_size *= 3U;

    /* 确保输出缓冲区可以容纳完整 RGB888 图像 */
    if (destination_size < required_size)
    {
        return FSP_ERR_INVALID_SIZE;
    }

    /* 将八个 RGB888 字节偏移一次装入 128 位 MVE 向量寄存器 */
    const uint16x8_t output_offset_vector = vld1q_u16(rgb888_byte_offsets);

     for (uint32_t destination_y = 0U;
         destination_y < p_cfg->output_height;
         destination_y++)
    {
        /* 使用最近邻公式计算当前目标行对应的源图像行 */
        uint32_t source_y =
            (uint32_t) p_cfg->crop_y +
            ((destination_y * p_cfg->crop_height) /
             p_cfg->output_height);

        /* 定位到裁剪区域内当前源图像行的第一个 RGB565 像素 */
        const uint16_t * p_source_row =
            p_source_rgb565 +
            ((size_t) source_y *
             p_cfg->source_stride_pixels) +
            p_cfg->crop_x;

        for (uint32_t destination_x = 0U;
             destination_x < p_cfg->output_width;
             destination_x += IMAGE_MVE_PIXELS_PER_VECTOR)
        {
            /* 保存本轮最多八个目标像素对应的源像素横向索引 */
            uint16_t source_offsets[IMAGE_MVE_PIXELS_PER_VECTOR]
                BSP_ALIGN_VARIABLE(16) = {0U};

            /* 最后一轮不足八个像素时，只启用实际剩余的向量通道 */
            uint32_t active_lanes =
                (uint32_t) p_cfg->output_width -
                destination_x;

            if (active_lanes > IMAGE_MVE_PIXELS_PER_VECTOR)
            {
                active_lanes = IMAGE_MVE_PIXELS_PER_VECTOR;
            }

            /* 使用最近邻公式计算每个目标像素对应的源像素索引 */
            for (uint32_t lane = 0U;
                 lane < active_lanes;
                 lane++)
            {
                source_offsets[lane] =
                    (uint16_t)
                    ((((destination_x + lane) *
                       p_cfg->crop_width)) /
                     p_cfg->output_width);
            }

            /* 生成 16 位通道谓词，关闭尾部无效通道，防止越界读写 */
            mve_pred16_t predicate =
                vctp16q(active_lanes);

            /* 将八个源像素索引装入 MVE 向量寄存器 */
            uint16x8_t source_offset_vector =
                vld1q_u16(source_offsets);

            /*
             * Gather load 会从八个不同的源像素位置读取 RGB565
             * shifted offset 会自动把 uint16_t 索引乘以 2
             */
            uint16x8_t rgb565_vector =
                vldrhq_gather_shifted_offset_z_u16(
                    p_source_row,
                    source_offset_vector,
                    predicate);

            /* 提取 RGB565 中的 R5、G6、B5 分量 */
            uint16x8_t red_5_vector =
                vandq_u16(
                    vshrq_n_u16(rgb565_vector, 11),
                    vdupq_n_u16(0x1FU));

            uint16x8_t green_6_vector =
                vandq_u16(
                    vshrq_n_u16(rgb565_vector, 5),
                    vdupq_n_u16(0x3FU));

            uint16x8_t blue_5_vector =
                vandq_u16(
                    rgb565_vector,
                    vdupq_n_u16(0x1FU));

            /* R5 -> R8 */
            uint16x8_t red_8_vector =
                vorrq_u16(
                    vshlq_n_u16(red_5_vector, 3),
                    vshrq_n_u16(red_5_vector, 2));

            /* G6 -> G8 */
            uint16x8_t green_8_vector =
                vorrq_u16(
                    vshlq_n_u16(green_6_vector, 2),
                    vshrq_n_u16(green_6_vector, 4));

            /* B5 -> B8 */
            uint16x8_t blue_8_vector =
                vorrq_u16(
                    vshlq_n_u16(blue_5_vector, 3),
                    vshrq_n_u16(blue_5_vector, 2));

            /* 定位到本轮八个目标像素在 RGB888 输出缓冲区中的起点 */
            uint8_t * p_destination_block =
                p_destination_rgb888 +
                ((((size_t) destination_y *
                   p_cfg->output_width) +
                  destination_x) * 3U);

            /*
             * 分别写入 RGB888 中的 R、G、B 字节
             * scatter offset 的偏移单位是字节
             */
            vstrbq_scatter_offset_p_u16(
                p_destination_block + 0U,
                output_offset_vector,
                red_8_vector,
                predicate);

            /* 基地址加 1，分散写入八个绿色分量 */
            vstrbq_scatter_offset_p_u16(
                p_destination_block + 1U,
                output_offset_vector,
                green_8_vector,
                predicate);

            /* 基地址加 2，分散写入八个蓝色分量 */
            vstrbq_scatter_offset_p_u16(
                p_destination_block + 2U,
                output_offset_vector,
                blue_8_vector,
                predicate);
        }
    }

    return FSP_SUCCESS;
}


/*
 *[@name] ImageJpeg_EncodeCapturedSnapshot
 *[@type] function
 *[@usage] 将模块私有RGB888快照编码到模块私有JPEG缓冲区
 *[@argument] quality JPEG质量，范围1到100
 *[@argument] p_jpeg_size 返回有效JPEG字节数
 *[@return] 成功返回FSP_SUCCESS，否则返回对应错误码
 */
fsp_err_t ImageJpeg_EncodeCapturedSnapshot(
    uint8_t quality,
    size_t * p_jpeg_size)
{
    image_jpeg_write_context_t write_context;
    int encode_result;

    if(NULL == p_jpeg_size)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    *p_jpeg_size = 0U;
    g_upload_jpeg_size = 0U;

    if(!g_rgb888_snapshot_valid)
    {
        return FSP_ERR_NOT_INITIALIZED;
    }

    if((quality < IMAGE_JPEG_QUALITY_MIN) ||
       (quality > IMAGE_JPEG_QUALITY_MAX))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    write_context.p_buffer = g_upload_jpeg;
    write_context.capacity = sizeof(g_upload_jpeg);
    write_context.size = 0U;
    write_context.overflow = false;

    encode_result = stbi_write_jpg_to_func(
        image_jpeg_write_callback,
        &write_context,
        (int) IMAGE_UPLOAD_WIDTH,
        (int) IMAGE_UPLOAD_HEIGHT,
        IMAGE_JPEG_RGB_COMPONENTS,
        g_upload_rgb888,
        (int) quality);

    if(0 == encode_result)
    {
        return FSP_ERR_INTERNAL;
    }

    if(write_context.overflow)
    {
        return FSP_ERR_INVALID_SIZE;
    }

    if(write_context.size < 4U)
    {
        return FSP_ERR_INTERNAL;
    }

    if((0xFFU != g_upload_jpeg[0]) ||
       (0xD8U != g_upload_jpeg[1]) ||
       (0xFFU != g_upload_jpeg[write_context.size - 2U]) ||
       (0xD9U != g_upload_jpeg[write_context.size - 1U]))
    {
        return FSP_ERR_INTERNAL;
    }

    g_upload_jpeg_size = write_context.size;
    *p_jpeg_size = write_context.size;
    return FSP_SUCCESS;
}

/*
 *[@name] ImageJpeg_EncodeRgb565
 *[@type] function
 *[@usage] 将RGB565指定区域转换为RGB888，并阻塞编码到调用者提供的JPEG缓冲区
 *[@argument] p_source_rgb565 RGB565源帧缓冲区首地址
 *[@argument] p_cfg 裁剪、输出尺寸和JPEG质量配置
 *[@argument] p_rgb888_workspace RGB888中间工作缓冲区
 *[@argument] rgb888_workspace_size RGB888工作缓冲区容量，单位为字节
 *[@argument] p_jpeg_output JPEG输出缓冲区
 *[@argument] jpeg_output_capacity JPEG输出缓冲区容量，单位为字节
 *[@argument] p_jpeg_size 返回实际生成的JPEG字节数
 *[@return] 编码成功返回FSP_SUCCESS，否则返回对应错误码
 */
fsp_err_t ImageJpeg_EncodeRgb565(
    const uint16_t * p_source_rgb565,
    const image_jpeg_encode_cfg_t * p_cfg,
    uint8_t * p_rgb888_workspace,
    size_t rgb888_workspace_size,
    uint8_t * p_jpeg_output,
    size_t jpeg_output_capacity,
    size_t * p_jpeg_size)
{
    image_jpeg_write_context_t write_context;
    fsp_err_t err;
    int encode_result;

    /* 检查所有必需指针 */
    if ((NULL == p_source_rgb565) ||
        (NULL == p_cfg) ||
        (NULL == p_rgb888_workspace) ||
        (NULL == p_jpeg_output) ||
        (NULL == p_jpeg_size))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* 失败时默认返回零长度 */
    *p_jpeg_size = 0U;

    if(p_cfg->quality < IMAGE_JPEG_QUALITY_MIN ||
       p_cfg->quality > IMAGE_JPEG_QUALITY_MAX)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* 输出区非空 */
    if(0U == jpeg_output_capacity)
    {
        return FSP_ERR_INVALID_SIZE;
    }

    /* 生成 RGB888 中间图像 */
    err = ImageJpeg_ConvertRgb565ToRgb888Helium(
        p_source_rgb565,//源地址
        p_cfg,//JPEG配置
        p_rgb888_workspace,//RGB888缓冲区
        rgb888_workspace_size);
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    /* 初始化 stb 多次回调共同使用的内存写入状态 */
    write_context.p_buffer = p_jpeg_output;
    write_context.capacity = jpeg_output_capacity;
    write_context.size     = 0U;
    write_context.overflow = false;

    /* 将 RGB888 编码为 JPEG，并通过回调写入内存 */
    encode_result = stbi_write_jpg_to_func(
        image_jpeg_write_callback,
        &write_context,
        (int) p_cfg->output_width,
        (int) p_cfg->output_height,
        IMAGE_JPEG_RGB_COMPONENTS,
        p_rgb888_workspace,
        (int) p_cfg->quality);

    /* stb 返回 0 表示 JPEG 编码失败 */
    if (0 == encode_result)
    {
        return FSP_ERR_INTERNAL;
    }

    /* 回调检测到容量不足时，禁止使用不完整 JPEG */
    if (write_context.overflow)
    {
        return FSP_ERR_INVALID_SIZE;
    }

    /* 完整 JPEG 至少需要 SOI 和 EOI 两组标记 */
    if (write_context.size < 4U)
    {
        return FSP_ERR_INTERNAL;
    }

    /* 检查 JPEG 开头的 SOI 标记 FF D8 */
    if ((0xFFU != p_jpeg_output[0]) ||
        (0xD8U != p_jpeg_output[1]))
    {
        return FSP_ERR_INTERNAL;
    }

    /* 检查 JPEG 结尾的 EOI 标记 FF D9 */
    if ((0xFFU != p_jpeg_output[write_context.size - 2U]) ||
        (0xD9U != p_jpeg_output[write_context.size - 1U]))
    {
        return FSP_ERR_INTERNAL;
    }

    /* 只有全部检查通过后才返回有效 JPEG 长度。 */
    *p_jpeg_size = write_context.size;

    return FSP_SUCCESS;
}

/*
 *[@name] ImageJpeg_EncodeAndPublishRgb565
 *[@type] function
 *[@usage] 将RGB565图像阻塞编码到模块内部缓冲区并发布只读JPEG数据
 *[@argument] p_source_rgb565 RGB565源帧缓冲区首地址
 *[@argument] p_cfg 裁剪、缩放、输出尺寸和JPEG质量配置
 *[@argument] p_jpeg_size 返回实际生成并发布的JPEG字节数
 *[@return] 编码和格式检查成功返回FSP_SUCCESS，否则返回对应错误码
 */
fsp_err_t ImageJpeg_EncodeAndPublishRgb565(
    const uint16_t * p_source_rgb565,
    const image_jpeg_encode_cfg_t * p_cfg,
    size_t * p_jpeg_size)
{
    size_t encoded_size = 0U;
    fsp_err_t err;

    if ((NULL == p_source_rgb565) ||
        (NULL == p_cfg) ||
        (NULL == p_jpeg_size))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    *p_jpeg_size       = 0U;
    g_upload_jpeg_size = 0U;

    err = ImageJpeg_EncodeRgb565(
        p_source_rgb565,
        p_cfg,
        g_upload_rgb888,
        sizeof(g_upload_rgb888),
        g_upload_jpeg,
        sizeof(g_upload_jpeg),
        &encoded_size);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    g_upload_jpeg_size = encoded_size;
    *p_jpeg_size       = encoded_size;
    return FSP_SUCCESS;
}

/*
 *[@name] image_jpeg_write_callback
 *[@type] static callback function
 *[@usage] 接收stb_image_write分批产生的JPEG数据并顺序写入内存缓冲区，容量不足时只设置溢出标志
 *[@argument] p_context JPEG内存写入状态
 *[@argument] p_data 本次产生的JPEG数据块
 *[@argument] data_size 本次数据块长度，单位为字节
 *[@return] none
 */
static void image_jpeg_write_callback(
    void * p_context,
    void * p_data,
    int data_size)
{
    image_jpeg_write_context_t * p_write_context;

    if((NULL == p_context) ||
       (NULL == p_data) ||
       (data_size <= 0))
    {
        return;
    }

    p_write_context = (image_jpeg_write_context_t *) p_context;
    /* 检查前面是否已经发生溢出 */
    if(p_write_context->overflow)
    {
        return;
    }

    size_t chunk_size = (size_t) data_size;
    /* 检查当前长度是否有效，并确认剩余空间足够 */
    if((p_write_context->size > p_write_context->capacity) || //写入容量大于缓冲区容量
       (chunk_size > (p_write_context->capacity - p_write_context->size))) //剩余空间不足
       {
        p_write_context->overflow = true;
        return;
       }

    /* 将本次 JPEG 数据追加到已经写入的数据后 */
    memcpy(p_write_context->p_buffer + p_write_context->size,
           p_data,
           chunk_size);

    /* 更新已写入数据的总长度 */
    p_write_context->size += chunk_size;
}


/*
 *[@name] ImageJpeg_GetEncodedData
 *[@type] function
 *[@usage] 获取最近一次成功编码并发布的只读JPEG数据，数据在下一次编码开始前保持有效
 *[@argument] pp_jpeg_data 返回模块内部JPEG缓冲区的只读首地址
 *[@argument] p_jpeg_size 返回有效JPEG数据长度，单位为字节
 *[@return] 数据有效返回FSP_SUCCESS，否则返回参数或未初始化错误码
 */
fsp_err_t ImageJpeg_GetEncodedData(
    const uint8_t ** pp_jpeg_data,
    size_t * p_jpeg_size)
{
    if ((NULL == pp_jpeg_data) ||
        (NULL == p_jpeg_size))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    *pp_jpeg_data = NULL;
    *p_jpeg_size  = 0U;

    /*
     * 长度为零表示尚未成功编码，或者新一轮编码正在进行。
     */
    if (0U == g_upload_jpeg_size)
    {
        return FSP_ERR_NOT_INITIALIZED;
    }

    *pp_jpeg_data = g_upload_jpeg;
    *p_jpeg_size  = g_upload_jpeg_size;

    return FSP_SUCCESS;
}
