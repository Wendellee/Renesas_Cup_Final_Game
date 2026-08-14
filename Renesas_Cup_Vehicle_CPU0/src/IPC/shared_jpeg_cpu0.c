#include "IPC/shared_jpeg_cpu0.h"

#include "FreeRTOS.h"
#include "ipc_thread.h"
#include "task.h"
#include <string.h>

#define SHARED_JPEG_CPU1_ACK_TIMEOUT_MS    (120000U)

typedef struct st_shared_jpeg_cpu0_context
{
    bool initialized;
    volatile bool reply_pending;
    volatile uint32_t received_message;
    bool transfer_in_flight;
    bool notify_retry_pending;
    uint32_t in_flight_sequence;
    TickType_t publish_tick;
} shared_jpeg_cpu0_context_t;

static volatile shared_jpeg_control_t * const gp_shared_jpeg_control =
    (volatile shared_jpeg_control_t *) SHARED_JPEG_BASE_ADDRESS;
static uint8_t * const gp_shared_jpeg_payload =
    (uint8_t *) (SHARED_JPEG_BASE_ADDRESS + SHARED_JPEG_PAYLOAD_OFFSET);
static shared_jpeg_cpu0_context_t g_shared_jpeg_cpu0_context;
static volatile shared_video_control_t * const gp_shared_video_control =
    (volatile shared_video_control_t *) SHARED_VIDEO_BASE_ADDRESS;

static uint8_t * shared_video_slot_payload(uint32_t slot_index)
{
    return (uint8_t *) (SHARED_VIDEO_BASE_ADDRESS + SHARED_VIDEO_HEADER_SIZE +
                        (slot_index * SHARED_VIDEO_SLOT_CAPACITY));
}

static void shared_video_control_clean(void)
{
#if BSP_CFG_DCACHE_ENABLED
    SCB_CleanDCache_by_Addr((uint32_t *) SHARED_VIDEO_BASE_ADDRESS,
                           (int32_t) SHARED_VIDEO_HEADER_SIZE);
#endif
    __DMB();
}

static void shared_video_control_invalidate(void)
{
#if BSP_CFG_DCACHE_ENABLED
    SCB_InvalidateDCache_by_Addr((void *) SHARED_VIDEO_BASE_ADDRESS,
                                (int32_t) SHARED_VIDEO_HEADER_SIZE);
#endif
    __DMB();
}

static void shared_video_control_reset(void)
{
    gp_shared_video_control->magic = SHARED_VIDEO_MAGIC;
    gp_shared_video_control->protocol_version = SHARED_VIDEO_PROTOCOL_VERSION;
    gp_shared_video_control->header_size = SHARED_VIDEO_HEADER_SIZE;
    gp_shared_video_control->slot_capacity = SHARED_VIDEO_SLOT_CAPACITY;
    for(uint32_t index = 0U; index < SHARED_VIDEO_SLOT_COUNT; index++)
    {
        gp_shared_video_control->slots[index].state = SHARED_VIDEO_SLOT_FREE;
        gp_shared_video_control->slots[index].frame_sequence = 0U;
        gp_shared_video_control->slots[index].payload_length = 0U;
        gp_shared_video_control->slots[index].payload_crc32 = 0U;
        gp_shared_video_control->slots[index].dimensions = 0U;
        gp_shared_video_control->slots[index].reserved = 0U;
    }
    shared_video_control_clean();
}

/*
 *[@name] shared_jpeg_cpu0_control_clean
 *[@type] static function
 *[@usage] 将CPU0修改的64字节共享控制块写回SDRAM并执行内存屏障
 *[@argument] none
 *[@return] none
 */
static void shared_jpeg_cpu0_control_clean(void)
{
#if BSP_CFG_DCACHE_ENABLED
    SCB_CleanDCache_by_Addr((uint32_t *) SHARED_JPEG_BASE_ADDRESS,
                           (int32_t) SHARED_JPEG_HEADER_SIZE);
#endif
    __DMB();
}

/*
 *[@name] shared_jpeg_cpu0_control_invalidate
 *[@type] static function
 *[@usage] 丢弃CPU0缓存中的控制块副本，以读取CPU1写入的状态和错误码
 *[@argument] none
 *[@return] none
 */
static void shared_jpeg_cpu0_control_invalidate(void)
{
#if BSP_CFG_DCACHE_ENABLED
    SCB_InvalidateDCache_by_Addr((void *) SHARED_JPEG_BASE_ADDRESS,
                                (int32_t) SHARED_JPEG_HEADER_SIZE);
#endif
    __DMB();
}

/*
 *[@name] shared_jpeg_cpu0_payload_clean
 *[@type] static function
 *[@usage] 将有效JPEG载荷从CPU0 D-Cache写回共享SDRAM
 *[@argument] jpeg_length 有效JPEG载荷长度，单位为字节
 *[@return] none
 */
static void shared_jpeg_cpu0_payload_clean(size_t jpeg_length)
{
#if BSP_CFG_DCACHE_ENABLED
    SCB_CleanDCache_by_Addr((uint32_t *) gp_shared_jpeg_payload,
                           (int32_t) jpeg_length);
#else
    FSP_PARAMETER_NOT_USED(jpeg_length);
#endif
    __DMB();
}

/*
 *[@name] shared_jpeg_cpu0_protocol_is_valid
 *[@type] static function
 *[@usage] 校验共享控制块固定字段是否与当前协议版本一致
 *[@argument] none
 *[@return] 固定字段全部正确返回true，否则返回false
 */
static bool shared_jpeg_cpu0_protocol_is_valid(void)
{
    return (SHARED_JPEG_MAGIC == gp_shared_jpeg_control->magic) &&
           (SHARED_JPEG_PROTOCOL_VERSION == gp_shared_jpeg_control->protocol_version) &&
           (SHARED_JPEG_HEADER_SIZE == gp_shared_jpeg_control->header_size) &&
           (SHARED_JPEG_PAYLOAD_OFFSET == gp_shared_jpeg_control->payload_offset);
}

/*
 *[@name] shared_jpeg_cpu0_control_reset
 *[@type] static function
 *[@usage] 重新初始化共享控制块并在所有字段写回后最后发布FREE状态
 *[@argument] none
 *[@return] none
 */
static void shared_jpeg_cpu0_control_reset(void)
{
    gp_shared_jpeg_control->state = (uint32_t) SHARED_JPEG_STATE_M85_FILLING;
    gp_shared_jpeg_control->magic = SHARED_JPEG_MAGIC;
    gp_shared_jpeg_control->protocol_version = SHARED_JPEG_PROTOCOL_VERSION;
    gp_shared_jpeg_control->header_size = SHARED_JPEG_HEADER_SIZE;
    gp_shared_jpeg_control->message_type = 0U;
    gp_shared_jpeg_control->frame_sequence = 0U;
    gp_shared_jpeg_control->payload_offset = SHARED_JPEG_PAYLOAD_OFFSET;
    gp_shared_jpeg_control->payload_length = 0U;
    gp_shared_jpeg_control->payload_crc32 = 0U;
    gp_shared_jpeg_control->producer_error = (uint32_t) SHARED_JPEG_ERROR_NONE;
    gp_shared_jpeg_control->consumer_error = (uint32_t) SHARED_JPEG_ERROR_NONE;

    for(uint32_t index = 0U; index < 5U; index++)
    {
        gp_shared_jpeg_control->reserved[index] = 0U;
    }

    shared_jpeg_cpu0_control_clean();
    gp_shared_jpeg_control->state = (uint32_t) SHARED_JPEG_STATE_FREE;
    shared_jpeg_cpu0_control_clean();
}

/*
 *[@name] shared_jpeg_cpu0_transfer_release
 *[@type] static function
 *[@usage] 将共享控制块和CPU0本地传输上下文恢复为空闲状态，用于正常完成和超时恢复
 *[@argument] none
 *[@return] none
 */
static void shared_jpeg_cpu0_transfer_release(void)
{
    shared_jpeg_cpu0_control_reset();

    taskENTER_CRITICAL();
    g_shared_jpeg_cpu0_context.reply_pending = false;
    g_shared_jpeg_cpu0_context.received_message = 0U;
    taskEXIT_CRITICAL();

    g_shared_jpeg_cpu0_context.transfer_in_flight = false;
    g_shared_jpeg_cpu0_context.notify_retry_pending = false;
    g_shared_jpeg_cpu0_context.in_flight_sequence = 0U;
    g_shared_jpeg_cpu0_context.publish_tick = 0U;
}

/*
 *[@name] shared_jpeg_cpu0_init
 *[@type] function
 *[@usage] 初始化CPU0共享JPEG控制块并打开IPC Channel 0
 *[@argument] none
 *[@return] 成功返回FSP_SUCCESS，否则返回对应FSP错误码
 */
fsp_err_t shared_jpeg_cpu0_init(void)
{
    fsp_err_t err;

    if(g_shared_jpeg_cpu0_context.initialized)
    {
        return FSP_SUCCESS;
    }

    memset(&g_shared_jpeg_cpu0_context, 0, sizeof(g_shared_jpeg_cpu0_context));
    shared_jpeg_cpu0_control_reset();
    shared_video_control_reset();

    err = g_ipc0.p_api->open(g_ipc0.p_ctrl, g_ipc0.p_cfg);
    if(FSP_SUCCESS == err)
    {
        g_shared_jpeg_cpu0_context.initialized = true;
    }

    return err;
}

shared_jpeg_cpu0_result_t shared_video_cpu0_publish(
    const uint8_t * p_jpeg_data,
    size_t jpeg_length,
    uint32_t frame_sequence,
    uint16_t width,
    uint16_t height)
{
    uint32_t selected_slot = SHARED_VIDEO_SLOT_COUNT;

    if(!g_shared_jpeg_cpu0_context.initialized)
    {
        return SHARED_JPEG_CPU0_NOT_INITIALIZED;
    }
    if((NULL == p_jpeg_data) || (jpeg_length < 4U) ||
       (jpeg_length > SHARED_VIDEO_SLOT_CAPACITY) ||
       (0U == width) || (0U == height))
    {
        return (jpeg_length > SHARED_VIDEO_SLOT_CAPACITY) ?
               SHARED_JPEG_CPU0_TOO_LARGE : SHARED_JPEG_CPU0_INVALID_ARGUMENT;
    }
    if((0xFFU != p_jpeg_data[0]) || (0xD8U != p_jpeg_data[1]) ||
       (0xFFU != p_jpeg_data[jpeg_length - 2U]) ||
       (0xD9U != p_jpeg_data[jpeg_length - 1U]))
    {
        return SHARED_JPEG_CPU0_INVALID_ARGUMENT;
    }

    shared_video_control_invalidate();
    if((SHARED_VIDEO_MAGIC != gp_shared_video_control->magic) ||
       (SHARED_VIDEO_PROTOCOL_VERSION != gp_shared_video_control->protocol_version) ||
       (SHARED_VIDEO_HEADER_SIZE != gp_shared_video_control->header_size) ||
       (SHARED_VIDEO_SLOT_CAPACITY != gp_shared_video_control->slot_capacity))
    {
        shared_video_control_reset();
    }

    /* 优先使用 FREE 槽；没有空槽时覆盖尚未被 M33 取走的旧 READY 槽。 */
    for(uint32_t index = 0U; index < SHARED_VIDEO_SLOT_COUNT; index++)
    {
        if(SHARED_VIDEO_SLOT_FREE == gp_shared_video_control->slots[index].state)
        {
            selected_slot = index;
            break;
        }
        if((SHARED_VIDEO_SLOT_COUNT == selected_slot) &&
           (SHARED_VIDEO_SLOT_READY == gp_shared_video_control->slots[index].state))
        {
            selected_slot = index;
        }
    }
    if(SHARED_VIDEO_SLOT_COUNT == selected_slot)
    {
        return SHARED_JPEG_CPU0_BUSY;
    }

    gp_shared_video_control->slots[selected_slot].state = SHARED_VIDEO_SLOT_WRITING;
    shared_video_control_clean();

    uint8_t * const p_destination = shared_video_slot_payload(selected_slot);
    memcpy(p_destination, p_jpeg_data, jpeg_length);
#if BSP_CFG_DCACHE_ENABLED
    SCB_CleanDCache_by_Addr((uint32_t *) p_destination, (int32_t) jpeg_length);
#endif
    gp_shared_video_control->slots[selected_slot].frame_sequence = frame_sequence;
    gp_shared_video_control->slots[selected_slot].payload_length = (uint32_t) jpeg_length;
    gp_shared_video_control->slots[selected_slot].payload_crc32 =
        shared_jpeg_crc32(p_destination, jpeg_length);
    gp_shared_video_control->slots[selected_slot].dimensions =
        (uint32_t) width | ((uint32_t) height << 16U);
    gp_shared_video_control->slots[selected_slot].state = SHARED_VIDEO_SLOT_READY;
    shared_video_control_clean();

    return (FSP_SUCCESS == g_ipc0.p_api->messageSend(g_ipc0.p_ctrl,
                                                      SHARED_VIDEO_IPC_FRAME_READY)) ?
           SHARED_JPEG_CPU0_SUCCESS : SHARED_JPEG_CPU0_NOTIFY_PENDING;
}

/*
 *[@name] shared_jpeg_cpu0_on_ipc_message_isr
 *[@type] IPC interrupt service function
 *[@usage] 保存CPU1发来的DONE或ERROR短消息，不在中断中读取JPEG或计算CRC
 *[@argument] message IPC接收到的32位短消息
 *[@return] none
 */
void shared_jpeg_cpu0_on_ipc_message_isr(uint32_t message)
{
    if((SHARED_JPEG_IPC_DATA_DONE == message) ||
       (SHARED_JPEG_IPC_DATA_ERROR == message))
    {
        g_shared_jpeg_cpu0_context.received_message = message;
        __DMB();
        g_shared_jpeg_cpu0_context.reply_pending = true;
    }
}

/*
 *[@name] shared_jpeg_cpu0_publish
 *[@type] function
 *[@usage] 将完整JPEG复制到共享SDRAM，生成CRC并通过IPC通知CPU1
 *[@argument] p_jpeg_data CPU0私有JPEG只读首地址
 *[@argument] jpeg_length JPEG有效长度，单位为字节
 *[@argument] frame_sequence JPEG对应的摄像头帧序号
 *[@argument] confidence_milli 异常检测置信度千分值，范围0到1000
 *[@return] 返回CPU0共享JPEG模块状态
 */
shared_jpeg_cpu0_result_t shared_jpeg_cpu0_publish(
    const uint8_t * p_jpeg_data,
    size_t jpeg_length,
    uint32_t frame_sequence,
    uint16_t confidence_milli)
{
    fsp_err_t err;

    if(!g_shared_jpeg_cpu0_context.initialized)
    {
        return SHARED_JPEG_CPU0_NOT_INITIALIZED;
    }

    if(g_shared_jpeg_cpu0_context.transfer_in_flight)
    {
        return SHARED_JPEG_CPU0_BUSY;
    }

    if((NULL == p_jpeg_data) ||
       (jpeg_length < 4U) ||
       (confidence_milli > 1000U))
    {
        return SHARED_JPEG_CPU0_INVALID_ARGUMENT;
    }

    if(jpeg_length > SHARED_JPEG_PAYLOAD_CAPACITY)
    {
        return SHARED_JPEG_CPU0_TOO_LARGE;
    }

    if((0xFFU != p_jpeg_data[0]) || (0xD8U != p_jpeg_data[1]) ||
       (0xFFU != p_jpeg_data[jpeg_length - 2U]) ||
       (0xD9U != p_jpeg_data[jpeg_length - 1U]))
    {
        return SHARED_JPEG_CPU0_INVALID_ARGUMENT;
    }

    shared_jpeg_cpu0_control_invalidate();
    if(!shared_jpeg_cpu0_protocol_is_valid())
    {
        return SHARED_JPEG_CPU0_PROTOCOL_ERROR;
    }

    if((uint32_t) SHARED_JPEG_STATE_FREE != gp_shared_jpeg_control->state)
    {
        return SHARED_JPEG_CPU0_BUSY;
    }

    gp_shared_jpeg_control->state = (uint32_t) SHARED_JPEG_STATE_M85_FILLING;
    shared_jpeg_cpu0_control_clean();

    memcpy(gp_shared_jpeg_payload, p_jpeg_data, jpeg_length);
    gp_shared_jpeg_control->message_type = SHARED_JPEG_IPC_DATA_READY;
    gp_shared_jpeg_control->frame_sequence = frame_sequence;
    gp_shared_jpeg_control->payload_offset = SHARED_JPEG_PAYLOAD_OFFSET;
    gp_shared_jpeg_control->payload_length = (uint32_t) jpeg_length;
    gp_shared_jpeg_control->payload_crc32 = shared_jpeg_crc32(gp_shared_jpeg_payload, jpeg_length);
    gp_shared_jpeg_control->producer_error = (uint32_t) SHARED_JPEG_ERROR_NONE;
    gp_shared_jpeg_control->consumer_error = (uint32_t) SHARED_JPEG_ERROR_NONE;
    gp_shared_jpeg_control->reserved[SHARED_JPEG_CONFIDENCE_INDEX] =
        (uint32_t) confidence_milli;

    shared_jpeg_cpu0_payload_clean(jpeg_length);
    shared_jpeg_cpu0_control_clean();
    gp_shared_jpeg_control->state = (uint32_t) SHARED_JPEG_STATE_READY_FOR_M33;
    shared_jpeg_cpu0_control_clean();

    g_shared_jpeg_cpu0_context.transfer_in_flight = true;
    g_shared_jpeg_cpu0_context.in_flight_sequence = frame_sequence;
    g_shared_jpeg_cpu0_context.publish_tick = xTaskGetTickCount();

    err = g_ipc0.p_api->messageSend(g_ipc0.p_ctrl, SHARED_JPEG_IPC_DATA_READY);
    if(FSP_SUCCESS != err)
    {
        g_shared_jpeg_cpu0_context.notify_retry_pending = true;
        return SHARED_JPEG_CPU0_NOTIFY_PENDING;
    }

    return SHARED_JPEG_CPU0_SUCCESS;
}

/*
 *[@name] shared_jpeg_cpu0_poll
 *[@type] function
 *[@usage] 在任务上下文重发门铃、处理CPU1回执、执行超时恢复并释放共享缓冲区
 *[@argument] p_completion 返回完成状态、帧序号和CPU1错误码
 *[@return] 返回CPU0共享JPEG模块状态
 */
shared_jpeg_cpu0_result_t shared_jpeg_cpu0_poll(shared_jpeg_completion_t * p_completion)
{
    bool reply_pending;
    uint32_t received_message;

    if(NULL == p_completion)
    {
        return SHARED_JPEG_CPU0_INVALID_ARGUMENT;
    }

    memset(p_completion, 0, sizeof(*p_completion));

    if(!g_shared_jpeg_cpu0_context.initialized)
    {
        return SHARED_JPEG_CPU0_NOT_INITIALIZED;
    }

    if(g_shared_jpeg_cpu0_context.transfer_in_flight &&
       ((xTaskGetTickCount() - g_shared_jpeg_cpu0_context.publish_tick) >=
        pdMS_TO_TICKS(SHARED_JPEG_CPU1_ACK_TIMEOUT_MS)))
    {
        p_completion->completed = true;
        p_completion->succeeded = false;
        p_completion->frame_sequence = g_shared_jpeg_cpu0_context.in_flight_sequence;
        p_completion->error_code = (uint32_t) SHARED_JPEG_ERROR_TIMEOUT;
        shared_jpeg_cpu0_transfer_release();
        return SHARED_JPEG_CPU0_TIMEOUT;
    }

    taskENTER_CRITICAL();
    reply_pending = g_shared_jpeg_cpu0_context.reply_pending;
    received_message = g_shared_jpeg_cpu0_context.received_message;
    if(reply_pending)
    {
        g_shared_jpeg_cpu0_context.reply_pending = false;
    }
    taskEXIT_CRITICAL();

    if(!reply_pending)
    {
        if(g_shared_jpeg_cpu0_context.transfer_in_flight &&
           g_shared_jpeg_cpu0_context.notify_retry_pending)
        {
            fsp_err_t const err = g_ipc0.p_api->messageSend(g_ipc0.p_ctrl,
                                                            SHARED_JPEG_IPC_DATA_READY);
            if(FSP_SUCCESS != err)
            {
                return SHARED_JPEG_CPU0_NOTIFY_PENDING;
            }

            g_shared_jpeg_cpu0_context.notify_retry_pending = false;
        }

        return SHARED_JPEG_CPU0_SUCCESS;
    }

    if(!g_shared_jpeg_cpu0_context.transfer_in_flight)
    {
        return SHARED_JPEG_CPU0_SUCCESS;
    }

    shared_jpeg_cpu0_control_invalidate();
    if((!shared_jpeg_cpu0_protocol_is_valid()) ||
       (gp_shared_jpeg_control->frame_sequence !=
        g_shared_jpeg_cpu0_context.in_flight_sequence))
    {
        shared_jpeg_cpu0_transfer_release();
        return SHARED_JPEG_CPU0_PROTOCOL_ERROR;
    }

    p_completion->completed = true;
    p_completion->frame_sequence = gp_shared_jpeg_control->frame_sequence;
    p_completion->error_code = gp_shared_jpeg_control->consumer_error;

    if((SHARED_JPEG_IPC_DATA_DONE == received_message) &&
       ((uint32_t) SHARED_JPEG_STATE_DONE == gp_shared_jpeg_control->state) &&
       ((uint32_t) SHARED_JPEG_ERROR_NONE == gp_shared_jpeg_control->consumer_error))
    {
        p_completion->succeeded = true;
    }
    else if((SHARED_JPEG_IPC_DATA_ERROR == received_message) &&
            ((uint32_t) SHARED_JPEG_STATE_ERROR == gp_shared_jpeg_control->state))
    {
        p_completion->succeeded = false;
    }
    else
    {
        shared_jpeg_cpu0_transfer_release();
        return SHARED_JPEG_CPU0_PROTOCOL_ERROR;
    }

    shared_jpeg_cpu0_transfer_release();

    return SHARED_JPEG_CPU0_SUCCESS;
}
