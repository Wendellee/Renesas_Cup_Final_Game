#include "navigation_thread.h"
#include "ipc_thread.h"
#include "IPC/navigation_ipc_protocol.h"
#include "Navigation/navigation_runtime.h"
#include "SEGGER_RTT/bsp_print.h"

/* 图像和阈值参数均集中在此处，后续实测只需调整这些宏。 */
#define NAV_GRAY_WIDTH                    (200U)  /* Gray8图像宽度。 */
#define NAV_GRAY_HEIGHT                   (112U)  /* Gray8图像高度。 */
#define NAV_ROI_HEIGHT                    (24U)   /* 只统计底部ROI行数。 */
#define NAV_GRAY_THRESHOLD                (128U)  /* 小于该灰度值视为深色板面像素。 */
#define NAV_CENTER_DANGER_PERCENT         (65U)   /* 中央窗口强危险阈值。 */
#define NAV_CENTER_SAFE_PERCENT           (80U)   /* 中央窗口安全阈值。 */
#define NAV_STARTUP_SAFE_FRAMES           (3U)    /* 上电安全预热帧数。 */
#define NAV_TRANSITION_FRAMES             (2U)    /* 过渡区进入危险的连续帧数。 */
#define NAV_RECOVERY_SAFE_FRAMES          (3U)    /* 危险解除所需连续安全帧数。 */
#define NAV_LOG_INTERVAL_FRAMES           (10U)   /* 状态不变时的RTT输出间隔。 */

typedef struct st_nav_roi_stats
{
    uint32_t sum;
    uint32_t below_threshold;
    uint32_t count;
} nav_roi_stats_t;

typedef enum e_nav_intent
{
    NAV_INTENT_UNARMED = 0,
    NAV_INTENT_SAFE,
    NAV_INTENT_CAUTION,
    NAV_INTENT_TURN_LEFT_LATCHED,
    NAV_INTENT_RECOVERY,
    NAV_INTENT_COUNT,
} nav_intent_t;

static const uint8_t * volatile gp_navigation_gray;
static volatile uint32_t g_navigation_frame_sequence;
extern TaskHandle_t navigation_thread;

void navigation_frame_submit(const uint8_t * p_gray, uint32_t frame_sequence)
{
    if(NULL == p_gray)
    {
        return;
    }

    taskENTER_CRITICAL();
    gp_navigation_gray = p_gray;
    g_navigation_frame_sequence = frame_sequence;
    __DMB();
    taskEXIT_CRITICAL();
    xTaskNotifyGive(navigation_thread);
}

static nav_intent_t navigation_intent_update(uint32_t center_percent)
{
    static bool armed;
    static bool danger_latched;
    static uint32_t startup_safe_frames;
    static uint32_t transition_frames;
    static uint32_t recovery_frames;
    bool const safe_sample = center_percent >= NAV_CENTER_SAFE_PERCENT;
    bool const danger_sample = center_percent <= NAV_CENTER_DANGER_PERCENT;

    if(!armed)
    {
        if(safe_sample)
        {
            if(startup_safe_frames < NAV_STARTUP_SAFE_FRAMES)
            {
                startup_safe_frames++;
            }
            if(startup_safe_frames >= NAV_STARTUP_SAFE_FRAMES)
            {
                armed = true;
                startup_safe_frames = 0U;
                return NAV_INTENT_SAFE;
            }
        }
        else
        {
            startup_safe_frames = 0U;
        }
        return NAV_INTENT_UNARMED;
    }

    if(!danger_latched)
    {
        recovery_frames = 0U;
        if(safe_sample)
        {
            transition_frames = 0U;
            return NAV_INTENT_SAFE;
        }

        if(danger_sample)
        {
            transition_frames = NAV_TRANSITION_FRAMES;
        }
        else if(transition_frames < NAV_TRANSITION_FRAMES)
        {
            transition_frames++;
        }

        if(transition_frames >= NAV_TRANSITION_FRAMES)
        {
            danger_latched = true;
            transition_frames = 0U;
            return NAV_INTENT_TURN_LEFT_LATCHED;
        }
        return NAV_INTENT_CAUTION;
    }

    transition_frames = 0U;
    if(safe_sample)
    {
        if(recovery_frames < NAV_RECOVERY_SAFE_FRAMES)
        {
            recovery_frames++;
        }
        if(recovery_frames >= NAV_RECOVERY_SAFE_FRAMES)
        {
            danger_latched = false;
            recovery_frames = 0U;
            return NAV_INTENT_SAFE;
        }
        return NAV_INTENT_RECOVERY;
    }

    recovery_frames = 0U;
    return NAV_INTENT_TURN_LEFT_LATCHED;
}

static nav_ipc_action_t navigation_action_get(nav_intent_t intent)
{
    if(NAV_INTENT_SAFE == intent)
    {
        return NAV_IPC_ACTION_FORWARD;
    }
    if(NAV_INTENT_TURN_LEFT_LATCHED == intent)
    {
        return NAV_IPC_ACTION_TURN_LEFT;
    }
    return NAV_IPC_ACTION_STOP;
}

static const char * navigation_intent_name(nav_intent_t intent)
{
    switch(intent)
    {
        case NAV_INTENT_SAFE:              return "SAFE";
        case NAV_INTENT_CAUTION:           return "CAUTION";
        case NAV_INTENT_TURN_LEFT_LATCHED: return "TURN_LEFT_LATCHED";
        case NAV_INTENT_RECOVERY:          return "RECOVERY";
        case NAV_INTENT_UNARMED:
        default:                           return "UNARMED";
    }
}

static const char * navigation_action_name(nav_ipc_action_t action)
{
    switch(action)
    {
        case NAV_IPC_ACTION_FORWARD:   return "FORWARD";
        case NAV_IPC_ACTION_TURN_LEFT: return "TURN_LEFT";
        case NAV_IPC_ACTION_STOP:
        default:                       return "STOP";
    }
}

static void navigation_roi_analyze(const uint8_t * p_gray,
                                   uint32_t confidence_percent[3],
                                   uint32_t average[3])
{
    nav_roi_stats_t stats[3] = {0};
    uint32_t const first_y = NAV_GRAY_HEIGHT - NAV_ROI_HEIGHT;

    for(uint32_t y = first_y; y < NAV_GRAY_HEIGHT; y++)
    {
        uint32_t const row_offset = y * NAV_GRAY_WIDTH;
        for(uint32_t x = 0U; x < NAV_GRAY_WIDTH; x++)
        {
            uint32_t const window = (x < 67U) ? 0U : ((x < 134U) ? 1U : 2U);
            uint8_t const gray = p_gray[row_offset + x];
            stats[window].sum += gray;
            stats[window].count++;
            stats[window].below_threshold += (gray < NAV_GRAY_THRESHOLD) ? 1U : 0U;
        }
    }

    for(uint32_t window = 0U; window < 3U; window++)
    {
        confidence_percent[window] =
            (stats[window].below_threshold * 100U) / stats[window].count;
        average[window] = stats[window].sum / stats[window].count;
    }
}

void navigation_thread_entry(void * pvParameters)
{
    uint32_t last_frame_sequence = 0U;
    uint32_t log_frame_counter = 0U;
    nav_intent_t last_logged_intent = NAV_INTENT_COUNT;
    bool ipc_error_logged = false;

    FSP_PARAMETER_NOT_USED(pvParameters);
    g_printf("[NAV] Thread ready; motor command is gated on M33.\r\n");

    for(;;)
    {
        (void) ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        const uint8_t * p_gray;
        uint32_t frame_sequence;
        taskENTER_CRITICAL();
        __DMB();
        p_gray = gp_navigation_gray;
        frame_sequence = g_navigation_frame_sequence;
        taskEXIT_CRITICAL();

        if((NULL == p_gray) || (frame_sequence == last_frame_sequence))
        {
            continue;
        }
        last_frame_sequence = frame_sequence;

        uint32_t confidence_percent[3];
        uint32_t average[3];
        navigation_roi_analyze(p_gray, confidence_percent, average);
        nav_intent_t const intent = navigation_intent_update(confidence_percent[1]);
        nav_ipc_action_t const action = navigation_action_get(intent);
        uint32_t const message = nav_ipc_message_encode(action, (uint8_t) frame_sequence);
        fsp_err_t const send_result =
            g_ipc0.p_api->messageSend(g_ipc0.p_ctrl, message);

        if(FSP_SUCCESS != send_result)
        {
            if(!ipc_error_logged)
            {
                g_printf("[NAV][ERR] IPC send=%u; next frame will retry.\r\n",
                         (unsigned int) send_result);
                ipc_error_logged = true;
            }
        }
        else
        {
            ipc_error_logged = false;
        }

        log_frame_counter++;
        bool const intent_changed = intent != last_logged_intent;
        if(intent_changed || (log_frame_counter >= NAV_LOG_INTERVAL_FRAMES))
        {
            g_printf("[NAV] frame=%u intent=%s ipc=%s d128(L/C/R)=%u/%u/%u%% avg=%u/%u/%u.\r\n",
                     (unsigned int) frame_sequence,
                     navigation_intent_name(intent),
                     navigation_action_name(action),
                     (unsigned int) confidence_percent[0],
                     (unsigned int) confidence_percent[1],
                     (unsigned int) confidence_percent[2],
                     (unsigned int) average[0],
                     (unsigned int) average[1],
                     (unsigned int) average[2]);
            last_logged_intent = intent;
            log_frame_counter = 0U;
        }
    }
}
