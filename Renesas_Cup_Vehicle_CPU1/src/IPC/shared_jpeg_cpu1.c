#include "IPC/shared_jpeg_cpu1.h"

#include "FreeRTOS.h"
#include "ipc_thread.h"
#include "task.h"
#include <string.h>

typedef struct st_shared_jpeg_cpu1_context
{
    bool initialized;
    volatile bool data_ready_pending;
    bool reply_retry_pending;
    uint32_t reply_message;
    bool upload_in_flight;
    uint32_t upload_frame_sequence;
    volatile bool video_ready_pending;
    bool video_in_flight;
    uint8_t video_slot_index;
    uint32_t video_frame_sequence;
} shared_jpeg_cpu1_context_t;

static volatile shared_jpeg_control_t * const gp_shared_jpeg_control =
    (volatile shared_jpeg_control_t *) SHARED_JPEG_BASE_ADDRESS;
static const uint8_t * const gp_shared_jpeg_payload =
    (const uint8_t *) (SHARED_JPEG_BASE_ADDRESS + SHARED_JPEG_PAYLOAD_OFFSET);
static shared_jpeg_cpu1_context_t g_shared_jpeg_cpu1_context;
static volatile shared_video_control_t * const gp_shared_video_control =
    (volatile shared_video_control_t *) SHARED_VIDEO_BASE_ADDRESS;

static const uint8_t * shared_video_slot_payload(uint32_t slot_index)
{
    return (const uint8_t *) (SHARED_VIDEO_BASE_ADDRESS + SHARED_VIDEO_HEADER_SIZE +
                              (slot_index * SHARED_VIDEO_SLOT_CAPACITY));
}

static void shared_video_control_invalidate(void)
{
#if BSP_CFG_DCACHE_ENABLED
    SCB_InvalidateDCache_by_Addr((void *) SHARED_VIDEO_BASE_ADDRESS,
                                (int32_t) SHARED_VIDEO_HEADER_SIZE);
#endif
    __DMB();
}

static void shared_video_control_clean(void)
{
#if BSP_CFG_DCACHE_ENABLED
    SCB_CleanDCache_by_Addr((uint32_t *) SHARED_VIDEO_BASE_ADDRESS,
                           (int32_t) SHARED_VIDEO_HEADER_SIZE);
#endif
    __DMB();
}

/*
 *[@name] shared_jpeg_cpu1_control_invalidate
 *[@type] static function
 *[@usage] 丢弃CPU1缓存中的控制块副本，以读取CPU0最后发布的控制字段和READY状态
 *[@argument] none
 *[@return] none
 */
static void shared_jpeg_cpu1_control_invalidate(void)
{
#if BSP_CFG_DCACHE_ENABLED
    SCB_InvalidateDCache_by_Addr((void *) SHARED_JPEG_BASE_ADDRESS,
                                (int32_t) SHARED_JPEG_HEADER_SIZE);
#endif
    __DMB();
}

/*
 *[@name] shared_jpeg_cpu1_control_clean
 *[@type] static function
 *[@usage] 将CPU1修改的共享控制块写回SDRAM并执行内存屏障
 *[@argument] none
 *[@return] none
 */
static void shared_jpeg_cpu1_control_clean(void)
{
#if BSP_CFG_DCACHE_ENABLED
    SCB_CleanDCache_by_Addr((uint32_t *) SHARED_JPEG_BASE_ADDRESS,
                           (int32_t) SHARED_JPEG_HEADER_SIZE);
#endif
    __DMB();
}

/*
 *[@name] shared_jpeg_cpu1_payload_invalidate
 *[@type] static function
 *[@usage] 丢弃CPU1缓存中的JPEG载荷副本，以读取CPU0已经写回SDRAM的完整JPEG
 *[@argument] jpeg_length 有效JPEG载荷长度，单位为字节
 *[@return] none
 */
static void shared_jpeg_cpu1_payload_invalidate(size_t jpeg_length)
{
#if BSP_CFG_DCACHE_ENABLED
    SCB_InvalidateDCache_by_Addr((void *) gp_shared_jpeg_payload,
                                (int32_t) jpeg_length);
#else
    FSP_PARAMETER_NOT_USED(jpeg_length);
#endif
    __DMB();
}

/*
 *[@name] shared_jpeg_cpu1_finish
 *[@type] static function
 *[@usage] 发布CPU1处理状态并通过IPC向CPU0发送DONE或ERROR回执
 *[@argument] succeeded 校验成功时为true，失败时为false
 *[@argument] error_code CPU1校验错误码
 *[@return] 回执发送成功返回SUCCESS，否则返回IPC_ERROR并保留重试状态
 */
static shared_jpeg_cpu1_result_t shared_jpeg_cpu1_finish(
    bool succeeded,
    shared_jpeg_error_t error_code)
{
    uint32_t const reply_message = succeeded ?
        SHARED_JPEG_IPC_DATA_DONE : SHARED_JPEG_IPC_DATA_ERROR;

    gp_shared_jpeg_control->consumer_error = (uint32_t) error_code;
    gp_shared_jpeg_control->message_type = reply_message;
    __DMB();
    gp_shared_jpeg_control->state = succeeded ?
        (uint32_t) SHARED_JPEG_STATE_DONE :
        (uint32_t) SHARED_JPEG_STATE_ERROR;
    shared_jpeg_cpu1_control_clean();

    if(FSP_SUCCESS != g_ipc1.p_api->messageSend(g_ipc1.p_ctrl, reply_message))
    {
        g_shared_jpeg_cpu1_context.reply_retry_pending = true;
        g_shared_jpeg_cpu1_context.reply_message = reply_message;
        return SHARED_JPEG_CPU1_IPC_ERROR;
    }

    return SHARED_JPEG_CPU1_SUCCESS;
}

/*
 *[@name] shared_jpeg_cpu1_fixed_fields_check
 *[@type] static function
 *[@usage] 校验共享控制块的magic、版本、头长度和载荷偏移
 *[@argument] none
 *[@return] 返回第一个发现的协议错误码，全部正确返回NONE
 */
static shared_jpeg_error_t shared_jpeg_cpu1_fixed_fields_check(void)
{
    if(SHARED_JPEG_MAGIC != gp_shared_jpeg_control->magic)
    {
        return SHARED_JPEG_ERROR_BAD_MAGIC;
    }
    if(SHARED_JPEG_PROTOCOL_VERSION != gp_shared_jpeg_control->protocol_version)
    {
        return SHARED_JPEG_ERROR_BAD_VERSION;
    }
    if((SHARED_JPEG_HEADER_SIZE != gp_shared_jpeg_control->header_size) ||
       (SHARED_JPEG_PAYLOAD_OFFSET != gp_shared_jpeg_control->payload_offset))
    {
        return SHARED_JPEG_ERROR_BAD_HEADER;
    }

    return SHARED_JPEG_ERROR_NONE;
}

/*
 *[@name] shared_jpeg_cpu1_init
 *[@type] function
 *[@usage] 打开CPU1 IPC Channel 0并准备接收共享JPEG门铃
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，否则返回对应FSP错误码
 */
fsp_err_t shared_jpeg_cpu1_init(void)
{
    fsp_err_t err;

    if(g_shared_jpeg_cpu1_context.initialized)
    {
        return FSP_SUCCESS;
    }

    memset(&g_shared_jpeg_cpu1_context, 0, sizeof(g_shared_jpeg_cpu1_context));
    err = g_ipc1.p_api->open(g_ipc1.p_ctrl, g_ipc1.p_cfg);
    if(FSP_SUCCESS == err)
    {
        g_shared_jpeg_cpu1_context.initialized = true;
    }

    return err;
}

/*
 *[@name] shared_jpeg_cpu1_on_ipc_message_isr
 *[@type] IPC interrupt service function
 *[@usage] 保存CPU0发来的DATA_READY短消息，不在中断中读取共享SDRAM
 *[@argument] message IPC接收到的32位短消息
 *[@return] none
 */
void shared_jpeg_cpu1_on_ipc_message_isr(uint32_t message)
{
    if(SHARED_JPEG_IPC_DATA_READY == message)
    {
        g_shared_jpeg_cpu1_context.data_ready_pending = true;
        __DMB();
    }
    else if(SHARED_VIDEO_IPC_FRAME_READY == message)
    {
        g_shared_jpeg_cpu1_context.video_ready_pending = true;
        __DMB();
    }
}

shared_jpeg_cpu1_result_t shared_video_cpu1_process(
    shared_video_cpu1_report_t * p_report)
{
    uint32_t selected_slot = SHARED_VIDEO_SLOT_COUNT;
    uint32_t newest_sequence = 0U;

    if(NULL == p_report)
    {
        return SHARED_JPEG_CPU1_PROTOCOL_ERROR;
    }
    memset(p_report, 0, sizeof(*p_report));
    if(!g_shared_jpeg_cpu1_context.initialized)
    {
        return SHARED_JPEG_CPU1_NOT_INITIALIZED;
    }
    if(g_shared_jpeg_cpu1_context.video_in_flight)
    {
        return SHARED_JPEG_CPU1_NO_DATA;
    }

    shared_video_control_invalidate();
    g_shared_jpeg_cpu1_context.video_ready_pending = false;
    if((SHARED_VIDEO_MAGIC != gp_shared_video_control->magic) ||
       (SHARED_VIDEO_PROTOCOL_VERSION != gp_shared_video_control->protocol_version) ||
       (SHARED_VIDEO_HEADER_SIZE != gp_shared_video_control->header_size) ||
       (SHARED_VIDEO_SLOT_CAPACITY != gp_shared_video_control->slot_capacity))
    {
        return SHARED_JPEG_CPU1_PROTOCOL_ERROR;
    }

    for(uint32_t index = 0U; index < SHARED_VIDEO_SLOT_COUNT; index++)
    {
        volatile shared_video_slot_t const * const p_slot =
            &gp_shared_video_control->slots[index];
        if((SHARED_VIDEO_SLOT_READY == p_slot->state) &&
           ((SHARED_VIDEO_SLOT_COUNT == selected_slot) ||
            ((int32_t) (p_slot->frame_sequence - newest_sequence) > 0)))
        {
            selected_slot = index;
            newest_sequence = p_slot->frame_sequence;
        }
    }
    if(SHARED_VIDEO_SLOT_COUNT == selected_slot)
    {
        return SHARED_JPEG_CPU1_NO_DATA;
    }

    /* 丢掉其他 READY 旧帧，保证低延迟而不是积压。 */
    for(uint32_t index = 0U; index < SHARED_VIDEO_SLOT_COUNT; index++)
    {
        if((index != selected_slot) &&
           (SHARED_VIDEO_SLOT_READY == gp_shared_video_control->slots[index].state))
        {
            gp_shared_video_control->slots[index].state = SHARED_VIDEO_SLOT_FREE;
        }
    }

    volatile shared_video_slot_t * const p_slot =
        &gp_shared_video_control->slots[selected_slot];
    uint32_t const payload_length = p_slot->payload_length;
    uint16_t const width = (uint16_t) p_slot->dimensions;
    uint16_t const height = (uint16_t) (p_slot->dimensions >> 16U);
    const uint8_t * const p_payload = shared_video_slot_payload(selected_slot);

    if((payload_length < 4U) || (payload_length > SHARED_VIDEO_SLOT_CAPACITY) ||
       (0U == width) || (0U == height))
    {
        p_slot->state = SHARED_VIDEO_SLOT_FREE;
        shared_video_control_clean();
        return SHARED_JPEG_CPU1_PROTOCOL_ERROR;
    }
    p_slot->state = SHARED_VIDEO_SLOT_IN_USE;
    shared_video_control_clean();
#if BSP_CFG_DCACHE_ENABLED
    SCB_InvalidateDCache_by_Addr((void *) p_payload, (int32_t) payload_length);
#endif
    __DMB();
    if((0xFFU != p_payload[0]) || (0xD8U != p_payload[1]) ||
       (0xFFU != p_payload[payload_length - 2U]) ||
       (0xD9U != p_payload[payload_length - 1U]) ||
       (p_slot->payload_crc32 != shared_jpeg_crc32(p_payload, payload_length)))
    {
        p_slot->state = SHARED_VIDEO_SLOT_FREE;
        shared_video_control_clean();
        return SHARED_JPEG_CPU1_PROTOCOL_ERROR;
    }

    p_report->frame_ready = true;
    p_report->p_payload = p_payload;
    p_report->frame_sequence = p_slot->frame_sequence;
    p_report->payload_length = payload_length;
    p_report->payload_crc32 = p_slot->payload_crc32;
    p_report->width = width;
    p_report->height = height;
    p_report->slot_index = (uint8_t) selected_slot;
    g_shared_jpeg_cpu1_context.video_in_flight = true;
    g_shared_jpeg_cpu1_context.video_slot_index = (uint8_t) selected_slot;
    g_shared_jpeg_cpu1_context.video_frame_sequence = p_slot->frame_sequence;
    return SHARED_JPEG_CPU1_SUCCESS;
}

shared_jpeg_cpu1_result_t shared_video_cpu1_complete(
    uint32_t frame_sequence,
    bool succeeded)
{
    FSP_PARAMETER_NOT_USED(succeeded);
    if((!g_shared_jpeg_cpu1_context.video_in_flight) ||
       (frame_sequence != g_shared_jpeg_cpu1_context.video_frame_sequence))
    {
        return SHARED_JPEG_CPU1_PROTOCOL_ERROR;
    }

    shared_video_control_invalidate();
    uint8_t const slot_index = g_shared_jpeg_cpu1_context.video_slot_index;
    if((slot_index >= SHARED_VIDEO_SLOT_COUNT) ||
       (SHARED_VIDEO_SLOT_IN_USE != gp_shared_video_control->slots[slot_index].state) ||
       (frame_sequence != gp_shared_video_control->slots[slot_index].frame_sequence))
    {
        return SHARED_JPEG_CPU1_PROTOCOL_ERROR;
    }
    gp_shared_video_control->slots[slot_index].state = SHARED_VIDEO_SLOT_FREE;
    shared_video_control_clean();
    g_shared_jpeg_cpu1_context.video_in_flight = false;
    g_shared_jpeg_cpu1_context.video_slot_index = 0U;
    g_shared_jpeg_cpu1_context.video_frame_sequence = 0U;
    return SHARED_JPEG_CPU1_SUCCESS;
}

/*
 *[@name] shared_jpeg_cpu1_process
 *[@type] function
 *[@usage] 在CPU1 IPC任务中校验控制块、JPEG边界和CRC并向CPU0回执
 *[@argument] p_report 返回帧序号、长度、CRC和处理结果
 *[@return] 返回CPU1共享JPEG模块状态
 */
shared_jpeg_cpu1_result_t shared_jpeg_cpu1_process(shared_jpeg_cpu1_report_t * p_report)
{
    bool data_ready_pending;
    uint32_t payload_length;
    shared_jpeg_error_t error_code;

    if(NULL == p_report)
    {
        return SHARED_JPEG_CPU1_PROTOCOL_ERROR;
    }

    memset(p_report, 0, sizeof(*p_report));

    if(!g_shared_jpeg_cpu1_context.initialized)
    {
        return SHARED_JPEG_CPU1_NOT_INITIALIZED;
    }

    if(g_shared_jpeg_cpu1_context.reply_retry_pending)
    {
        if(FSP_SUCCESS != g_ipc1.p_api->messageSend(
            g_ipc1.p_ctrl,
            g_shared_jpeg_cpu1_context.reply_message))
        {
            return SHARED_JPEG_CPU1_IPC_ERROR;
        }

        g_shared_jpeg_cpu1_context.reply_retry_pending = false;
        g_shared_jpeg_cpu1_context.reply_message = 0U;
        return SHARED_JPEG_CPU1_SUCCESS;
    }

    taskENTER_CRITICAL();
    data_ready_pending = g_shared_jpeg_cpu1_context.data_ready_pending;
    g_shared_jpeg_cpu1_context.data_ready_pending = false;
    taskEXIT_CRITICAL();

    shared_jpeg_cpu1_control_invalidate();
    if((!data_ready_pending) &&
       ((uint32_t) SHARED_JPEG_STATE_READY_FOR_M33 != gp_shared_jpeg_control->state))
    {
        return SHARED_JPEG_CPU1_NO_DATA;
    }

    p_report->frame_sequence = gp_shared_jpeg_control->frame_sequence;
    p_report->payload_length = gp_shared_jpeg_control->payload_length;
    p_report->expected_crc32 = gp_shared_jpeg_control->payload_crc32;
    p_report->confidence_milli = (uint16_t)
        gp_shared_jpeg_control->reserved[SHARED_JPEG_CONFIDENCE_INDEX];

    if(g_shared_jpeg_cpu1_context.upload_in_flight)
    {
        return SHARED_JPEG_CPU1_NO_DATA;
    }

    error_code = shared_jpeg_cpu1_fixed_fields_check();
    if(SHARED_JPEG_ERROR_NONE == error_code)
    {
        if((uint32_t) SHARED_JPEG_STATE_READY_FOR_M33 != gp_shared_jpeg_control->state)
        {
            error_code = SHARED_JPEG_ERROR_BAD_STATE;
        }
        else if((SHARED_JPEG_IPC_DATA_READY != gp_shared_jpeg_control->message_type) ||
                (gp_shared_jpeg_control->payload_length < 4U) ||
                (gp_shared_jpeg_control->payload_length > SHARED_JPEG_PAYLOAD_CAPACITY))
        {
            error_code = SHARED_JPEG_ERROR_BAD_LENGTH;
        }
        else if(gp_shared_jpeg_control->reserved[SHARED_JPEG_CONFIDENCE_INDEX] > 1000U)
        {
            error_code = SHARED_JPEG_ERROR_BAD_HEADER;
        }
    }

    if(SHARED_JPEG_ERROR_NONE != error_code)
    {
        p_report->completed = true;
        p_report->error_code = (uint32_t) error_code;
        (void) shared_jpeg_cpu1_finish(false, error_code);
        return SHARED_JPEG_CPU1_PROTOCOL_ERROR;
    }

    gp_shared_jpeg_control->state = (uint32_t) SHARED_JPEG_STATE_M33_PROCESSING;
    shared_jpeg_cpu1_control_clean();

    payload_length = gp_shared_jpeg_control->payload_length;
    shared_jpeg_cpu1_payload_invalidate(payload_length);
    if((0xFFU != gp_shared_jpeg_payload[0]) ||
       (0xD8U != gp_shared_jpeg_payload[1]))
    {
        error_code = SHARED_JPEG_ERROR_BAD_SOI;
    }
    else if((0xFFU != gp_shared_jpeg_payload[payload_length - 2U]) ||
            (0xD9U != gp_shared_jpeg_payload[payload_length - 1U]))
    {
        error_code = SHARED_JPEG_ERROR_BAD_EOI;
    }
    else
    {
        p_report->actual_crc32 = shared_jpeg_crc32(gp_shared_jpeg_payload,
                                                    payload_length);
        if(p_report->actual_crc32 != p_report->expected_crc32)
        {
            error_code = SHARED_JPEG_ERROR_BAD_CRC;
        }
    }

    p_report->completed = true;
    p_report->succeeded = (SHARED_JPEG_ERROR_NONE == error_code);
    p_report->error_code = (uint32_t) error_code;

    if(SHARED_JPEG_ERROR_NONE != error_code)
    {
        return shared_jpeg_cpu1_finish(false, error_code);
    }

    p_report->completed = false;
    p_report->upload_ready = true;
    p_report->p_payload = gp_shared_jpeg_payload;
    g_shared_jpeg_cpu1_context.upload_in_flight = true;
    g_shared_jpeg_cpu1_context.upload_frame_sequence = p_report->frame_sequence;

    return SHARED_JPEG_CPU1_SUCCESS;
}

/*
 *[@name] shared_jpeg_cpu1_complete_upload
 *[@type] function
 *[@usage] 在Wi-Fi Upload Thread完成或放弃上传后发布DONE或ERROR状态，并向CPU0发送最终IPC回执
 *[@argument] frame_sequence 当前上传作业对应的摄像头帧序号
 *[@argument] succeeded 网页端TCP上传完整成功时为true，否则为false
 *[@argument] error_code 上传失败原因，成功时必须为SHARED_JPEG_ERROR_NONE
 *[@return] 返回CPU1共享JPEG模块状态
 */
shared_jpeg_cpu1_result_t shared_jpeg_cpu1_complete_upload(
    uint32_t frame_sequence,
    bool succeeded,
    shared_jpeg_error_t error_code)
{
    shared_jpeg_cpu1_result_t result;

    if(!g_shared_jpeg_cpu1_context.initialized)
    {
        return SHARED_JPEG_CPU1_NOT_INITIALIZED;
    }

    if((!g_shared_jpeg_cpu1_context.upload_in_flight) ||
       (frame_sequence != g_shared_jpeg_cpu1_context.upload_frame_sequence))
    {
        return SHARED_JPEG_CPU1_PROTOCOL_ERROR;
    }

    if(succeeded)
    {
        error_code = SHARED_JPEG_ERROR_NONE;
    }
    else if(SHARED_JPEG_ERROR_NONE == error_code)
    {
        error_code = SHARED_JPEG_ERROR_TCP_SEND;
    }

    result = shared_jpeg_cpu1_finish(succeeded, error_code);
    g_shared_jpeg_cpu1_context.upload_in_flight = false;
    g_shared_jpeg_cpu1_context.upload_frame_sequence = 0U;

    return result;
}
