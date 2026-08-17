#include "Radio/adapters/rtos/video_frame_mailbox.h"

#include "FreeRTOS.h"
#include "task.h"

static video_frame_t g_frame;
static bool g_occupied;
static bool g_acquired;
static bool g_completion_pending;
static bool g_completion_success;
static uint16_t g_completed_frame_id;

bool VideoFrameMailbox_Publish(video_frame_t const * p_frame)
{
    if ((NULL == p_frame) || (NULL == p_frame->p_jpeg) ||
        (0U == p_frame->jpeg_size) ||
        (0U == VideoProtocol_ChunkCountGet(p_frame->jpeg_size)))
    {
        return false;
    }

    bool accepted = false;
    taskENTER_CRITICAL();
    if (!g_occupied)
    {
        g_frame = *p_frame;
        g_occupied = true;
        g_acquired = false;
        accepted = true;
    }
    taskEXIT_CRITICAL();
    return accepted;
}

bool VideoFrameMailbox_Acquire(video_frame_t * p_frame)
{
    if (NULL == p_frame)
    {
        return false;
    }

    bool available = false;
    taskENTER_CRITICAL();
    if (g_occupied && !g_acquired)
    {
        *p_frame = g_frame;
        g_acquired = true;
        available = true;
    }
    taskEXIT_CRITICAL();
    return available;
}

void VideoFrameMailbox_Complete(uint16_t frame_id, bool success)
{
    taskENTER_CRITICAL();
    if (g_occupied && g_acquired && (frame_id == g_frame.frame_id))
    {
        g_completed_frame_id = frame_id;
        g_completion_success = success;
        g_completion_pending = true;
        g_occupied = false;
        g_acquired = false;
    }
    taskEXIT_CRITICAL();
}

bool VideoFrameMailbox_CompletionTake(uint16_t * p_frame_id, bool * p_success)
{
    if ((NULL == p_frame_id) || (NULL == p_success))
    {
        return false;
    }

    bool available = false;
    taskENTER_CRITICAL();
    if (g_completion_pending)
    {
        *p_frame_id = g_completed_frame_id;
        *p_success = g_completion_success;
        g_completion_pending = false;
        available = true;
    }
    taskEXIT_CRITICAL();
    return available;
}
