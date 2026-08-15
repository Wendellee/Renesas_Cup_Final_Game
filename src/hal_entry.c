#include "hal_data.h"
#include "app_config.h"
#include "common_utils.h"
#include "generated/gui_guider.h"
#include "generated/events_init.h"
#include "misc/cache/instance/lv_image_cache.h"
#include "lv_port_indev.h"
#include "glcdc_display.h"
#include "i2c_control.h"
#include "touch_GT911.h"
#include "bsp/nrf24_port.h"
#include "nrf24/wireless_touch_tx.h"
#if APP_CAMERA_CAPTURE_ENABLE
#include "CAMERA/camera_capture.h"
#endif
#include "CAMERA/jpeg_codec.h"
#include "freertos_app.h"
#include "FreeRTOS.h"
#include "task.h"

#include <string.h>



#if (1 == BSP_MULTICORE_PROJECT) && BSP_TZ_SECURE_BUILD
bsp_ipc_semaphore_handle_t g_core_start_semaphore =
{
    .semaphore_num = 0
};
#endif


uint16_t g_hz_size, g_vr_size;
uint32_t g_buffer_size;
uint8_t * g_p_single_buffer, * g_p_double_buffer;
lv_ui guider_ui;

void R_BSP_SdramInit(bool init_memory);

#define LCD_FRAMEBUFFER_TEST_MODE    (0)
#define TOUCH_RTT_TEST_MODE          (0)

#define IMAGE_PACKET_MAGIC           (0x49U)
#define IMAGE_PACKET_VERSION         (1U)
#define IMAGE_PACKET_TYPE_START      (1U)
#define IMAGE_PACKET_TYPE_DATA       (2U)
#define IMAGE_PACKET_TYPE_END        (3U)
#define IMAGE_PACKET_SIZE            (32U)
#define IMAGE_PACKET_DATA_OFFSET     (4U)
#define IMAGE_PACKET_DATA_SIZE       (IMAGE_PACKET_SIZE - IMAGE_PACKET_DATA_OFFSET)
#define IMAGE_PACKET_CHECKSUM_INDEX  (31U)
#define IMAGE_CODEC_JPEG_RGB888      (2U)
#define IMAGE_SOURCE_WIDTH           JPEG_CODEC_WIDTH
#define IMAGE_SOURCE_HEIGHT          JPEG_CODEC_HEIGHT
#define IMAGE_DISPLAY_WIDTH          JPEG_DISPLAY_WIDTH
#define IMAGE_DISPLAY_HEIGHT         JPEG_DISPLAY_HEIGHT
#define IMAGE_COMPRESSED_BUFFER_SIZE JPEG_CODEC_MAX_ENCODED_SIZE
#define IMAGE_DECOMPRESSED_BUFFER_SIZE JPEG_CODEC_RGB888_SIZE
#define IMAGE_PROGRESS_INTERVAL      (1024U)
#define IMAGE_PATTERN_LVGL_SOURCE    (0xFFU)
#define IMAGE_PATTERN_CAMERA_SOURCE  (0xFEU)
#define NRF24_TX_BATCH_PACKETS_PER_LOOP (8U)
#define NRF24_RX_FIFO_DRAIN_INTERVAL    (3U)
#define IMAGE_RX_STALL_TIMEOUT_MS       (250U)

#if APP_VIDEO_RX_ENABLE
#if APP_LOCAL_VIDEO_LOOPBACK_ENABLE
typedef enum e_image_tx_stage
{
    IMAGE_TX_STAGE_IDLE = 0,
    IMAGE_TX_STAGE_START,
    IMAGE_TX_STAGE_DATA,
    IMAGE_TX_STAGE_END
} image_tx_stage_t;

typedef struct st_image_tx_state
{
    image_tx_stage_t stage;
    uint8_t const  * p_data;
    uint32_t         data_size;
    uint32_t         crc32;
    uint32_t         start_tick_ms;
    uint32_t         send_error_count;
    uint16_t         width;
    uint16_t         height;
    uint16_t         frame_id;
    uint16_t         chunk_index;
    uint16_t         chunk_count;
    uint8_t          color_format;
    uint8_t          pattern_id;
    nrf24_result_t   last_error;
} image_tx_state_t;
#endif

typedef struct st_image_rx_state
{
    bool             active;
    uint32_t         start_tick_ms;
    uint32_t         last_packet_tick_ms;
    uint32_t         data_size;
    uint32_t         expected_crc32;
    uint32_t         received_bytes;
    uint32_t         missing_chunks;
    uint32_t         duplicate_chunks;
    uint32_t         invalid_packets;
    uint16_t         width;
    uint16_t         height;
    uint16_t         frame_id;
    uint16_t         next_chunk;
    uint16_t         received_chunks;
    uint16_t         chunk_count;
    uint8_t          color_format;
    uint8_t          pattern_id;
} image_rx_state_t;

#if APP_LOCAL_VIDEO_LOOPBACK_ENABLE
static uint8_t          g_image_tx_gray8[JPEG_CODEC_GRAY8_SIZE] BSP_ALIGN_VARIABLE(32);
static uint8_t          g_image_tx_compressed[IMAGE_COMPRESSED_BUFFER_SIZE] BSP_ALIGN_VARIABLE(32);
#endif
static uint8_t          g_image_rx_compressed[IMAGE_COMPRESSED_BUFFER_SIZE] BSP_ALIGN_VARIABLE(32);
/* JPEG decoding and GLCDC/LVGL rendering run in different RTOS tasks.  Keep
 * two decoder buffers, plus one stable LVGL presentation buffer.  The image
 * object's source is installed only once; later frames update the stable
 * buffer and invalidate the object once.  This avoids a visible blank/old
 * source interval while lv_image_set_src() replaces the source every frame. */
static uint8_t          g_image_rx_buffer[2][IMAGE_DECOMPRESSED_BUFFER_SIZE] BSP_ALIGN_VARIABLE(32);
static uint8_t          g_image_present_buffer[IMAGE_DECOMPRESSED_BUFFER_SIZE] BSP_ALIGN_VARIABLE(32);
static lv_image_dsc_t   g_image_present_dsc;
#if APP_LOCAL_VIDEO_LOOPBACK_ENABLE
static image_tx_state_t g_image_tx_state;
static uint32_t         g_camera_last_queued_sequence;
static uint16_t         g_camera_next_frame_id = 1U;
#endif
static image_rx_state_t g_image_rx_state;
static volatile bool    g_image_display_pending;
static bool             g_image_present_source_installed;
static lv_obj_t        * g_image_present_owner;
static volatile uint8_t g_image_display_active_index;
static volatile uint8_t g_image_display_pending_index;
#endif

#if TOUCH_RTT_TEST_MODE
static void touch_i2c_pin_probe_and_select(void)
{
    bsp_io_level_t scl_p512 = BSP_IO_LEVEL_LOW;
    bsp_io_level_t sda_p511 = BSP_IO_LEVEL_LOW;
    bsp_io_level_t p409     = BSP_IO_LEVEL_LOW;
    bsp_io_level_t p410     = BSP_IO_LEVEL_LOW;

    /*
     * The generated pin table currently contains two IIC-looking pin pairs.
     * Keep the configured IIC1 pair active and move the other pair back to GPIO
     * input so the peripheral is not accidentally mapped to two places.
     */
    R_IOPORT_PinCfg(&g_ioport_ctrl,
                    BSP_IO_PORT_04_PIN_09,
                    (uint32_t) IOPORT_CFG_PORT_DIRECTION_INPUT | (uint32_t) IOPORT_CFG_PULLUP_ENABLE);
    R_IOPORT_PinCfg(&g_ioport_ctrl,
                    BSP_IO_PORT_04_PIN_10,
                    (uint32_t) IOPORT_CFG_PORT_DIRECTION_INPUT | (uint32_t) IOPORT_CFG_PULLUP_ENABLE);

    R_IOPORT_PinCfg(&g_ioport_ctrl,
                    BSP_IO_PORT_05_PIN_11,
                    (uint32_t) IOPORT_CFG_DRIVE_MID | (uint32_t) IOPORT_CFG_PERIPHERAL_PIN |
                    (uint32_t) IOPORT_CFG_PULLUP_ENABLE | (uint32_t) IOPORT_PERIPHERAL_IIC);
    R_IOPORT_PinCfg(&g_ioport_ctrl,
                    BSP_IO_PORT_05_PIN_12,
                    (uint32_t) IOPORT_CFG_DRIVE_MID | (uint32_t) IOPORT_CFG_PERIPHERAL_PIN |
                    (uint32_t) IOPORT_CFG_PULLUP_ENABLE | (uint32_t) IOPORT_PERIPHERAL_IIC);

    R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);

    (void) R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_05_PIN_12, &scl_p512);
    (void) R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_05_PIN_11, &sda_p511);
    (void) R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_04_PIN_09, &p409);
    (void) R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_04_PIN_10, &p410);

    APP_PRINT("[TOUCH_TEST] IIC1 select P512=SCL P511=SDA, idle SCL=%u SDA=%u, spare P409=%u P410=%u\r\n",
              (uint32_t) scl_p512,
              (uint32_t) sda_p511,
              (uint32_t) p409,
              (uint32_t) p410);
}

static void touch_rtt_test_loop(void)
{
    fsp_err_t err;
    TouchCoordinate_t coords[GT911_MAX_TOUCH_POINTS];
    uint8_t count = 0;
    touch_event_t event = TOUCH_EVENT_NONE;
    uint32_t tick = 0;

    APP_PRINT("\r\n[TOUCH_TEST] GT911 RTT test start\r\n");

    touch_i2c_pin_probe_and_select();

    err = i2c_control_init();
    APP_PRINT("[TOUCH_TEST] i2c_control_init err=0x%08lX\r\n", (uint32_t) err);
    if (FSP_SUCCESS != err)
    {
        while (1)
        {
            R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
        }
    }

    err = gt911_enable();
    APP_PRINT("[TOUCH_TEST] gt911_enable err=0x%08lX active_addr=0x%02X pid=%02X %02X %02X %02X last_error=0x%08lX try5d=0x%08lX try14=0x%08lX\r\n",
              (uint32_t) err,
              g_gt911_active_address,
              g_gt911_product_id[0],
              g_gt911_product_id[1],
              g_gt911_product_id[2],
              g_gt911_product_id[3],
              (uint32_t) g_gt911_last_error,
              (uint32_t) g_gt911_try_5d_error,
              (uint32_t) g_gt911_try_14_error);

    while (FSP_SUCCESS != err)
    {
        R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
        err = gt911_enable();
        APP_PRINT("[TOUCH_TEST] retry gt911_enable err=0x%08lX active_addr=0x%02X pid=%02X %02X %02X %02X last_error=0x%08lX try5d=0x%08lX try14=0x%08lX\r\n",
                  (uint32_t) err,
                  g_gt911_active_address,
                  g_gt911_product_id[0],
                  g_gt911_product_id[1],
                  g_gt911_product_id[2],
                  g_gt911_product_id[3],
                  (uint32_t) g_gt911_last_error,
                  (uint32_t) g_gt911_try_5d_error,
                  (uint32_t) g_gt911_try_14_error);
    }

    while (1)
    {
        err = gt911_read_touch(coords, &count, &event);

        if ((FSP_SUCCESS != err) || (count > 0U) || (0U == (tick % 20U)))
        {
            APP_PRINT("[TOUCH_TEST] tick=%lu err=0x%08lX addr=0x%02X pid=%02X %02X %02X %02X status=0x%02X count=%u event=%u x=%u y=%u ok=%lu fail=%lu irq=%lu try5d=0x%08lX try14=0x%08lX\r\n",
                      tick,
                      (uint32_t) err,
                      g_gt911_active_address,
                      g_gt911_product_id[0],
                      g_gt911_product_id[1],
                      g_gt911_product_id[2],
                      g_gt911_product_id[3],
                      g_gt911_last_status,
                      g_gt911_last_count,
                      (uint32_t) event,
                      g_gt911_last_x,
                      g_gt911_last_y,
                      g_gt911_read_ok_count,
                      g_gt911_read_error_count,
                      g_gt911_irq_count,
                      (uint32_t) g_gt911_try_5d_error,
                      (uint32_t) g_gt911_try_14_error);
        }

        tick++;
        R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
#endif

#if LCD_FRAMEBUFFER_TEST_MODE
typedef struct st_sdram_test_result
{
    uint32_t status;
    uint32_t failed_pattern;
    uint32_t failed_index;
    uint32_t expected;
    uint32_t actual;
} sdram_test_result_t;

volatile sdram_test_result_t g_sdram_test_result;

#define SDRAM_TEST_STATUS_PASS    (0x53445041U) /* "SDPA" */
#define SDRAM_TEST_STATUS_FAIL    (0x53444641U) /* "SDFA" */

static sdram_test_result_t sdram_framebuffer_self_test(void)
{
    static const uint32_t test_patterns[] =
    {
        0x00000000U, 0xFFFFFFFFU, 0xAAAAAAAAU, 0x55555555U
    };

    sdram_test_result_t result =
    {
        .status = SDRAM_TEST_STATUS_PASS
    };

    volatile uint32_t * p_words = (volatile uint32_t *) fb_background;
    uint32_t const      words   = (uint32_t) ((sizeof(fb_background)) / sizeof(uint32_t));

    for (uint32_t p = 0; p < (sizeof(test_patterns) / sizeof(test_patterns[0])); p++)
    {
        uint32_t pattern = test_patterns[p];

        for (uint32_t i = 0; i < words; i++)
        {
            p_words[i] = pattern;
        }

        for (uint32_t i = 0; i < words; i++)
        {
            uint32_t actual = p_words[i];
            if (actual != pattern)
            {
                result.status         = SDRAM_TEST_STATUS_FAIL;
                result.failed_pattern = pattern;
                result.failed_index   = i;
                result.expected       = pattern;
                result.actual         = actual;
                return result;
            }
        }
    }

    for (uint32_t i = 0; i < words; i++)
    {
        p_words[i] = 0xA5A50000U ^ i;
    }

    for (uint32_t i = 0; i < words; i++)
    {
        uint32_t expected = 0xA5A50000U ^ i;
        uint32_t actual   = p_words[i];
        if (actual != expected)
        {
            result.status         = SDRAM_TEST_STATUS_FAIL;
            result.failed_pattern = 0xA5A50000U;
            result.failed_index   = i;
            result.expected       = expected;
            result.actual         = actual;
            return result;
        }
    }

    return result;
}

static void lcd_framebuffer_test_pattern(void)
{
    static const uint32_t colors[] =
    {
        0x00FF0000U, 0x0000FF00U, 0x000000FFU, 0x00FFFFFFU,
        0x00000000U, 0x00FFFF00U, 0x00FF00FFU, 0x0000FFFFU,
        0x00808080U, 0x00404040U
    };

    for (uint32_t buffer = 0; buffer < 2; buffer++)
    {
        uint32_t * p_frame = (uint32_t *) fb_background[buffer];

        for (uint32_t y = 0; y < DISPLAY_VSIZE_INPUT0; y++)
        {
            for (uint32_t x = 0; x < DISPLAY_HSIZE_INPUT0; x++)
            {
                uint32_t color = colors[(x / 80U) % (sizeof(colors) / sizeof(colors[0]))];

                if ((0U == (x % 80U)) || (0U == (y % 60U)))
                {
                    color = 0x00FFFFFFU;
                }

                p_frame[(y * DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0) + x] = color;
            }
        }
    }
}

static void lcd_framebuffer_fail_pattern(sdram_test_result_t const * p_result)
{
    uint32_t failed_word = p_result->failed_index;

    for (uint32_t buffer = 0; buffer < 2; buffer++)
    {
        uint32_t * p_frame = (uint32_t *) fb_background[buffer];

        for (uint32_t y = 0; y < DISPLAY_VSIZE_INPUT0; y++)
        {
            for (uint32_t x = 0; x < DISPLAY_HSIZE_INPUT0; x++)
            {
                uint32_t color = 0x00FF0000U;

                if ((0U == (x % 32U)) || (0U == (y % 32U)))
                {
                    color = 0x00FFFFFFU;
                }
                else if (((x + (y * DISPLAY_HSIZE_INPUT0)) & 0xFFFFU) == (failed_word & 0xFFFFU))
                {
                    color = 0x000000FFU;
                }

                p_frame[(y * DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0) + x] = color;
            }
        }
    }
}
#endif

#if APP_VIDEO_RX_CONTROL_COMPAT_ENABLE
static char const * wireless_touch_control_name(uint8_t control)
{
    switch ((wireless_touch_control_t) control)
    {
        case WIRELESS_TOUCH_CONTROL_DIRECTION: return "direction";
        case WIRELESS_TOUCH_CONTROL_RUN_STOP:  return "run_stop";
        case WIRELESS_TOUCH_CONTROL_SPEED:     return "speed";
        case WIRELESS_TOUCH_CONTROL_MODE:      return "mode";
        case WIRELESS_TOUCH_CONTROL_LED:       return "led";
        case WIRELESS_TOUCH_CONTROL_FAN:       return "fan";
        case WIRELESS_TOUCH_CONTROL_WIFI:      return "wifi";
        case WIRELESS_TOUCH_CONTROL_PAGE:      return "page";
        default:                               return "unknown";
    }
}

static char const * wireless_touch_action_name(uint8_t action)
{
    switch ((wireless_touch_action_t) action)
    {
        case WIRELESS_TOUCH_ACTION_RELEASED: return "released";
        case WIRELESS_TOUCH_ACTION_PRESSED:  return "pressed";
        case WIRELESS_TOUCH_ACTION_CHANGED:  return "changed";
        default:                             return "unknown";
    }
}
#endif /* APP_VIDEO_RX_CONTROL_COMPAT_ENABLE */

#if APP_VIDEO_RX_ENABLE
#if APP_LOCAL_VIDEO_LOOPBACK_ENABLE
static void image_write_u16(uint8_t * p_data, uint16_t value)
{
    p_data[0] = (uint8_t) value;
    p_data[1] = (uint8_t) (value >> 8U);
}

static void image_write_u32(uint8_t * p_data, uint32_t value)
{
    p_data[0] = (uint8_t) value;
    p_data[1] = (uint8_t) (value >> 8U);
    p_data[2] = (uint8_t) (value >> 16U);
    p_data[3] = (uint8_t) (value >> 24U);
}
#endif

static uint16_t image_read_u16(uint8_t const * p_data)
{
    return (uint16_t) p_data[0] | (uint16_t) ((uint16_t) p_data[1] << 8U);
}

static uint32_t image_read_u32(uint8_t const * p_data)
{
    return (uint32_t) p_data[0] |
           ((uint32_t) p_data[1] << 8U) |
           ((uint32_t) p_data[2] << 16U) |
           ((uint32_t) p_data[3] << 24U);
}

static uint8_t image_packet_checksum(uint8_t const * p_packet)
{
    uint8_t checksum = 0U;

    for (uint32_t i = 0U; i < IMAGE_PACKET_CHECKSUM_INDEX; i++)
    {
        checksum ^= p_packet[i];
    }

    return checksum;
}

static uint32_t image_crc32_update_byte(uint32_t crc, uint8_t value)
{
    crc ^= value;
    for (uint32_t bit = 0U; bit < 8U; bit++)
    {
        uint32_t mask = 0U - (crc & 1U);
        crc = (crc >> 1U) ^ (0xEDB88320U & mask);
    }

    return crc;
}

static uint32_t image_crc32(uint8_t const * p_data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFU;

    for (uint32_t i = 0U; i < length; i++)
    {
        crc = image_crc32_update_byte(crc, p_data[i]);
    }

    return ~crc;
}

#if APP_LOCAL_VIDEO_LOOPBACK_ENABLE
static void image_packet_common_init(uint8_t * p_packet, uint8_t packet_type, uint16_t frame_id)
{
    (void) memset(p_packet, 0, IMAGE_PACKET_SIZE);
    p_packet[0] = IMAGE_PACKET_MAGIC;
    p_packet[1] = packet_type;
    image_write_u16(&p_packet[2], frame_id);
}
#endif

static void image_prepare_rgb888_descriptor(void)
{
    lv_image_dsc_t * p_descriptor = &g_image_present_dsc;

    (void) memset(p_descriptor, 0, sizeof(*p_descriptor));
    p_descriptor->header.magic  = LV_IMAGE_HEADER_MAGIC;
    p_descriptor->header.cf     = LV_COLOR_FORMAT_RGB888;
    p_descriptor->header.flags  = LV_IMAGE_FLAGS_MODIFIABLE;
    p_descriptor->header.stride = IMAGE_DISPLAY_WIDTH * 3U;
    p_descriptor->header.w      = IMAGE_DISPLAY_WIDTH;
    p_descriptor->header.h      = IMAGE_DISPLAY_HEIGHT;
    p_descriptor->data_size     = IMAGE_DECOMPRESSED_BUFFER_SIZE;
    p_descriptor->data          = g_image_present_buffer;
}

#if APP_LOCAL_VIDEO_LOOPBACK_ENABLE
static bool image_test_queue_frame(uint16_t frame_id,
                                   uint8_t pattern_id,
                                   uint32_t jpeg_size)
{
    uint32_t chunk_count;

    if ((0U == jpeg_size) || (IMAGE_COMPRESSED_BUFFER_SIZE < jpeg_size))
    {
        APP_PRINT("[IMAGE TEST] invalid JPEG size=%lu capacity=%lu\r\n",
                  jpeg_size,
                  (uint32_t) IMAGE_COMPRESSED_BUFFER_SIZE);
        return false;
    }

    if (!WirelessTouchTx_IsReady() || !WirelessTouchRx_IsReady())
    {
        APP_PRINT("[IMAGE TEST] start rejected spi1_tx_ready=%u spi0_rx_ready=%u\r\n",
                  WirelessTouchTx_IsReady() ? 1U : 0U,
                  WirelessTouchRx_IsReady() ? 1U : 0U);
        return false;
    }

    chunk_count = (jpeg_size + IMAGE_PACKET_DATA_SIZE - 1U) / IMAGE_PACKET_DATA_SIZE;
    if (UINT16_MAX < chunk_count)
    {
        APP_PRINT("[IMAGE TEST] start rejected chunk_count=%lu\r\n", chunk_count);
        return false;
    }

    (void) memset(&g_image_tx_state, 0, sizeof(g_image_tx_state));
    g_image_tx_state.p_data       = g_image_tx_compressed;
    g_image_tx_state.data_size    = jpeg_size;
    g_image_tx_state.width        = IMAGE_SOURCE_WIDTH;
    g_image_tx_state.height       = IMAGE_SOURCE_HEIGHT;
    g_image_tx_state.color_format = (uint8_t) LV_COLOR_FORMAT_RGB888;
    g_image_tx_state.frame_id     = frame_id;
    g_image_tx_state.chunk_count  = (uint16_t) chunk_count;
    g_image_tx_state.crc32        = image_crc32(g_image_tx_compressed, jpeg_size);
    g_image_tx_state.start_tick_ms = lv_tick_get();
    g_image_tx_state.pattern_id   = pattern_id;
    g_image_tx_state.last_error   = NRF24_RESULT_SUCCESS;
    __DMB();
    g_image_tx_state.stage        = IMAGE_TX_STAGE_START;

    APP_PRINT("[IMG TX] f=%u n=%lu chunks=%u crc=%08lX\r\n",
              (uint32_t) g_image_tx_state.frame_id,
              g_image_tx_state.data_size,
              (uint32_t) g_image_tx_state.chunk_count,
              g_image_tx_state.crc32);

    return true;
}

static void image_test_dynamic_service(void)
{
    uint8_t const * p_camera_frame;
    uint32_t camera_sequence;
    uint32_t jpeg_size = 0U;
    uint32_t compress_start_ms;

    if ((IMAGE_TX_STAGE_IDLE != g_image_tx_state.stage) ||
        (0U != WirelessRadioTx_RingCountGet()) ||
        g_image_rx_state.active ||
        g_image_display_pending)
    {
        return;
    }

    if (!CameraCapture_GetLatestFrame(&p_camera_frame, &camera_sequence) ||
        (camera_sequence == g_camera_last_queued_sequence))
    {
        return;
    }

    compress_start_ms = lv_tick_get();
    if (!JpegCodec_CameraRgb565ToGray8(p_camera_frame,
                                       g_image_tx_gray8,
                                       JPEG_CODEC_GRAY8_SIZE) ||
        !JpegCodec_EncodeGray8(g_image_tx_gray8,
                               g_image_tx_compressed,
                               IMAGE_COMPRESSED_BUFFER_SIZE,
                               &jpeg_size))
    {
        APP_PRINT("[CAMERA COMPRESS] rejected sequence=%lu frame=%p\r\n",
                  camera_sequence,
                  (void *) p_camera_frame);
        return;
    }

    APP_PRINT("[JPEG] seq=%lu gray=%ux%u jpg=%lu encode_ms=%lu crc=%08lX\r\n",
              camera_sequence,
              (uint32_t) IMAGE_SOURCE_WIDTH,
              (uint32_t) IMAGE_SOURCE_HEIGHT,
              jpeg_size,
              (uint32_t) (lv_tick_get() - compress_start_ms),
              image_crc32(g_image_tx_compressed, jpeg_size));

    if (image_test_queue_frame(g_camera_next_frame_id,
                               IMAGE_PATTERN_CAMERA_SOURCE,
                               jpeg_size))
    {
        g_camera_last_queued_sequence = camera_sequence;
        g_camera_next_frame_id++;
        if (0U == g_camera_next_frame_id)
        {
            g_camera_next_frame_id = 1U;
        }
    }
}

static void image_test_tx_service(void)
{
    uint8_t packet[IMAGE_PACKET_SIZE];
    nrf24_result_t result;

    if (IMAGE_TX_STAGE_IDLE == g_image_tx_state.stage)
    {
        return;
    }

    image_packet_common_init(packet, (uint8_t) g_image_tx_state.stage, g_image_tx_state.frame_id);

    if (IMAGE_TX_STAGE_START == g_image_tx_state.stage)
    {
        packet[4] = IMAGE_PACKET_VERSION;
        packet[5] = g_image_tx_state.color_format;
        image_write_u16(&packet[6], g_image_tx_state.width);
        image_write_u16(&packet[8], g_image_tx_state.height);
        image_write_u32(&packet[10], g_image_tx_state.data_size);
        image_write_u32(&packet[14], g_image_tx_state.crc32);
        image_write_u16(&packet[18], g_image_tx_state.chunk_count);
        packet[20] = IMAGE_CODEC_JPEG_RGB888;
        image_write_u16(&packet[21], IMAGE_SOURCE_WIDTH);
        image_write_u16(&packet[23], IMAGE_SOURCE_HEIGHT);
        packet[25] = g_image_tx_state.pattern_id;
    }
    else if (IMAGE_TX_STAGE_DATA == g_image_tx_state.stage)
    {
        uint32_t offset = (uint32_t) g_image_tx_state.chunk_index * IMAGE_PACKET_DATA_SIZE;
        uint32_t remaining = g_image_tx_state.data_size - offset;
        uint8_t data_length = (uint8_t) ((remaining < IMAGE_PACKET_DATA_SIZE) ?
                                        remaining : IMAGE_PACKET_DATA_SIZE);

        /* DATA packets rely on the nRF24 hardware CRC.  Keep only the packet
         * type and a 16-bit chunk index so 28 of 32 bytes carry JPEG data. */
        image_write_u16(&packet[2], g_image_tx_state.chunk_index);
        (void) memcpy(&packet[IMAGE_PACKET_DATA_OFFSET],
                      &g_image_tx_state.p_data[offset],
                      data_length);
    }
    else
    {
        image_write_u16(&packet[4], g_image_tx_state.chunk_count);
        image_write_u32(&packet[6], g_image_tx_state.data_size);
        image_write_u32(&packet[10], g_image_tx_state.crc32);
    }

    if (IMAGE_TX_STAGE_DATA != g_image_tx_state.stage)
    {
        packet[IMAGE_PACKET_CHECKSUM_INDEX] = image_packet_checksum(packet);
    }
    result = WirelessRadioTx_SendPayloadNoAck(packet, IMAGE_PACKET_SIZE);
    if (NRF24_RESULT_SUCCESS != result)
    {
        g_image_tx_state.send_error_count++;
        if ((g_image_tx_state.send_error_count <= 3U) ||
            (0U == (g_image_tx_state.send_error_count % 64U)))
        {
            APP_PRINT("[IMG TX ERR] f=%u stage=%u chunk=%u err=%u count=%lu\r\n",
                      (uint32_t) g_image_tx_state.frame_id,
                      (uint32_t) g_image_tx_state.stage,
                      (uint32_t) g_image_tx_state.chunk_index,
                      (uint32_t) result,
                      g_image_tx_state.send_error_count);
        }
        g_image_tx_state.last_error = result;
        return;
    }

    g_image_tx_state.last_error = NRF24_RESULT_SUCCESS;
    if (IMAGE_TX_STAGE_START == g_image_tx_state.stage)
    {
        g_image_tx_state.stage = IMAGE_TX_STAGE_DATA;
        APP_PRINT("[IMG TX] f=%u START\r\n", (uint32_t) g_image_tx_state.frame_id);
    }
    else if (IMAGE_TX_STAGE_DATA == g_image_tx_state.stage)
    {
        g_image_tx_state.chunk_index++;
        if (g_image_tx_state.chunk_index >= g_image_tx_state.chunk_count)
        {
            g_image_tx_state.stage = IMAGE_TX_STAGE_END;
        }
    }
    else
    {
        APP_PRINT("[IMG TX] f=%u END n=%lu chunks=%u crc=%08lX err=%lu\r\n",
                  (uint32_t) g_image_tx_state.frame_id,
                  g_image_tx_state.data_size,
                  (uint32_t) g_image_tx_state.chunk_count,
                  g_image_tx_state.crc32,
                  g_image_tx_state.send_error_count);
        g_image_tx_state.stage = IMAGE_TX_STAGE_IDLE;
    }
}
#endif /* APP_LOCAL_VIDEO_LOOPBACK_ENABLE */

static void image_test_rx_start(uint8_t const * p_packet)
{
    uint32_t data_size = image_read_u32(&p_packet[10]);
    uint16_t chunk_count = image_read_u16(&p_packet[18]);
    uint32_t calculated_chunks = (data_size + IMAGE_PACKET_DATA_SIZE - 1U) / IMAGE_PACKET_DATA_SIZE;

    if ((IMAGE_PACKET_VERSION != p_packet[4]) || (0U == data_size) ||
        (IMAGE_COMPRESSED_BUFFER_SIZE < data_size) || (0U == chunk_count) ||
        (calculated_chunks != chunk_count) ||
        (IMAGE_CODEC_JPEG_RGB888 != p_packet[20]) ||
        (IMAGE_SOURCE_WIDTH != image_read_u16(&p_packet[21])) ||
        (IMAGE_SOURCE_HEIGHT != image_read_u16(&p_packet[23])) ||
        (LV_COLOR_FORMAT_RGB888 != p_packet[5]))
    {
        APP_PRINT("[IMG RX BAD] ver=%u n=%lu chunks=%u calc=%lu codec=%u wh=%ux%u\r\n",
                  (uint32_t) p_packet[4], data_size, (uint32_t) chunk_count, calculated_chunks,
                  (uint32_t) p_packet[20],
                  (uint32_t) image_read_u16(&p_packet[21]),
                  (uint32_t) image_read_u16(&p_packet[23]));
        return;
    }

    (void) memset(&g_image_rx_state, 0, sizeof(g_image_rx_state));
    (void) memset(g_image_rx_compressed, 0, data_size);
    g_image_rx_state.active         = true;
    g_image_rx_state.start_tick_ms  = lv_tick_get();
    g_image_rx_state.last_packet_tick_ms = lv_tick_get();
    g_image_rx_state.frame_id       = image_read_u16(&p_packet[2]);
    g_image_rx_state.color_format   = p_packet[5];
    g_image_rx_state.width          = image_read_u16(&p_packet[6]);
    g_image_rx_state.height         = image_read_u16(&p_packet[8]);
    g_image_rx_state.data_size      = data_size;
    g_image_rx_state.expected_crc32 = image_read_u32(&p_packet[14]);
    g_image_rx_state.chunk_count    = chunk_count;
    g_image_rx_state.pattern_id     = p_packet[25];

    APP_PRINT("[IMG RX] START f=%u n=%lu chunks=%u crc=%08lX\r\n",
              (uint32_t) g_image_rx_state.frame_id,
              g_image_rx_state.data_size,
              (uint32_t) g_image_rx_state.chunk_count,
              g_image_rx_state.expected_crc32);
}

static void image_test_rx_data(uint8_t const * p_packet)
{
    uint16_t chunk_index = image_read_u16(&p_packet[2]);
    uint32_t offset = (uint32_t) chunk_index * IMAGE_PACKET_DATA_SIZE;
    uint32_t expected_length;

    if (!g_image_rx_state.active || (chunk_index >= g_image_rx_state.chunk_count) ||
        (offset >= g_image_rx_state.data_size))
    {
        g_image_rx_state.invalid_packets++;
        return;
    }

    expected_length = g_image_rx_state.data_size - offset;
    if (IMAGE_PACKET_DATA_SIZE < expected_length)
    {
        expected_length = IMAGE_PACKET_DATA_SIZE;
    }
    if (chunk_index < g_image_rx_state.next_chunk)
    {
        g_image_rx_state.duplicate_chunks++;
        return;
    }
    if (chunk_index > g_image_rx_state.next_chunk)
    {
        g_image_rx_state.missing_chunks += (uint32_t) chunk_index - g_image_rx_state.next_chunk;
    }

    (void) memcpy(&g_image_rx_compressed[offset],
                  &p_packet[IMAGE_PACKET_DATA_OFFSET],
                  expected_length);
    g_image_rx_state.last_packet_tick_ms = lv_tick_get();
    g_image_rx_state.next_chunk = (uint16_t) (chunk_index + 1U);
    g_image_rx_state.received_chunks++;
    g_image_rx_state.received_bytes += expected_length;

}

static void image_test_rx_end(uint8_t const * p_packet)
{
    uint16_t frame_id = image_read_u16(&p_packet[2]);
    uint16_t end_chunks = image_read_u16(&p_packet[4]);
    uint32_t end_size = image_read_u32(&p_packet[6]);
    uint32_t end_crc32 = image_read_u32(&p_packet[10]);
    uint32_t actual_crc32;
    bool metadata_ok;
    bool result_ok;

    if (!g_image_rx_state.active || (frame_id != g_image_rx_state.frame_id))
    {
        APP_PRINT("[IMG RX] END ignored f=%u active=%u expect=%u\r\n",
                  (uint32_t) frame_id,
                  g_image_rx_state.active ? 1U : 0U,
                  (uint32_t) g_image_rx_state.frame_id);
        return;
    }

    g_image_rx_state.last_packet_tick_ms = lv_tick_get();

    metadata_ok = ((end_chunks == g_image_rx_state.chunk_count) &&
                   (end_size == g_image_rx_state.data_size) &&
                   (end_crc32 == g_image_rx_state.expected_crc32));
    actual_crc32 = image_crc32(g_image_rx_compressed, g_image_rx_state.data_size);
    result_ok = metadata_ok &&
                (g_image_rx_state.received_chunks == g_image_rx_state.chunk_count) &&
                (g_image_rx_state.received_bytes == g_image_rx_state.data_size) &&
                (0U == g_image_rx_state.missing_chunks) &&
                (0U == g_image_rx_state.invalid_packets) &&
                (actual_crc32 == g_image_rx_state.expected_crc32);

    APP_PRINT("[IMG CHECK] f=%u %s tx=%08lX rx=%08lX bytes=%lu/%lu chunks=%u/%u miss=%lu dup=%lu bad=%lu meta=%s\r\n",
              (uint32_t) g_image_rx_state.frame_id,
              result_ok ? "PASS" : "FAIL",
              g_image_rx_state.expected_crc32,
              actual_crc32,
              g_image_rx_state.received_bytes,
              g_image_rx_state.data_size,
              (uint32_t) g_image_rx_state.received_chunks,
              (uint32_t) g_image_rx_state.chunk_count,
              g_image_rx_state.missing_chunks,
              g_image_rx_state.duplicate_chunks,
              g_image_rx_state.invalid_packets,
              metadata_ok ? "OK" : "FAIL");
    if (result_ok)
    {
        uint8_t decode_buffer_index = (uint8_t) (g_image_display_active_index ^ 1U);
        uint8_t * p_decode_buffer = g_image_rx_buffer[decode_buffer_index];
        uint32_t decoded_crc32;
        uint32_t decoder_result = 0U;
        uint16_t decoded_width = 0U;
        uint16_t decoded_height = 0U;
        bool decode_ok;
        bool display_updated;

        /* This is now a real remote receive path.  The former loopback-only
         * comparison against this board's TX buffer intentionally does not
         * participate in validation; length, chunk order and CRC32 are the
         * transport integrity checks shared with the vehicle. */
        decode_ok = JpegCodec_DecodeRgb888(g_image_rx_compressed,
                                           g_image_rx_state.data_size,
                                           p_decode_buffer,
                                           IMAGE_DECOMPRESSED_BUFFER_SIZE,
                                           &decoded_width,
                                           &decoded_height,
                                           &decoder_result);
        decoded_crc32 = decode_ok ?
                          image_crc32(p_decode_buffer, IMAGE_DECOMPRESSED_BUFFER_SIZE) : 0U;
        display_updated = decode_ok;
        if (display_updated)
        {
            __DMB();
            g_image_display_pending_index = decode_buffer_index;
            __DMB();
            g_image_display_pending = true;
        }
        APP_PRINT("[JPEG DEC] f=%u %s source=vehicle err=%lu jpg=%ux%u crc=%08lX gui=%s\r\n",
                  (uint32_t) g_image_rx_state.frame_id,
                  decode_ok ? "PASS" : "FAIL",
                  decoder_result,
                  (uint32_t) decoded_width,
                  (uint32_t) decoded_height,
                  decoded_crc32,
                  display_updated ? "queued-for-GUI-task" : "decode-failed");
        APP_PRINT("[FRAME] f=%u ms=%lu gui=%s\r\n",
                  (uint32_t) g_image_rx_state.frame_id,
                  (uint32_t) (lv_tick_get() - g_image_rx_state.start_tick_ms),
                  display_updated ? "queued-for-GUI-task" : "skipped");
    }
    g_image_rx_state.active = false;
}

static bool image_test_rx_process(uint8_t const * p_packet, uint8_t payload_length)
{
    if ((IMAGE_PACKET_SIZE != payload_length) || (IMAGE_PACKET_MAGIC != p_packet[0]))
    {
        return false;
    }

    if ((IMAGE_PACKET_TYPE_DATA != p_packet[1]) &&
        (p_packet[IMAGE_PACKET_CHECKSUM_INDEX] != image_packet_checksum(p_packet)))
    {
        g_image_rx_state.invalid_packets++;
        APP_PRINT("[IMAGE RX SPI0] packet checksum FAIL type=%u frame=%u invalid=%lu\r\n",
                  (uint32_t) p_packet[1],
                  (uint32_t) image_read_u16(&p_packet[2]),
                  g_image_rx_state.invalid_packets);
        return true;
    }

    switch (p_packet[1])
    {
        case IMAGE_PACKET_TYPE_START:
            image_test_rx_start(p_packet);
            break;
        case IMAGE_PACKET_TYPE_DATA:
            image_test_rx_data(p_packet);
            break;
        case IMAGE_PACKET_TYPE_END:
            image_test_rx_end(p_packet);
            break;
        default:
            g_image_rx_state.invalid_packets++;
            break;
    }

    return true;
}
#endif

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void AppRadioTask_Entry(void)
{
    /* TODO: add your own code here */
    SEGGER_RTT_Init();
    (void) SEGGER_RTT_SetFlagsUpBuffer(SEGGER_INDEX, SEGGER_RTT_MODE_NO_BLOCK_TRIM);

    g_ioport.p_api->open(g_ioport.p_ctrl,g_ioport.p_cfg);
    g_ioport.p_api->pinWrite(g_ioport.p_ctrl, BSP_IO_PORT_01_PIN_10, BSP_IO_LEVEL_HIGH);

#if TOUCH_RTT_TEST_MODE
    touch_rtt_test_loop();
#else
    /* Frame buffers are placed in .sdram_noinit, so SDRAM must be ready before GLCDC/LVGL use them. */
    R_BSP_SdramInit(true);

#if LCD_FRAMEBUFFER_TEST_MODE
    sdram_test_result_t sdram_test = sdram_framebuffer_self_test();
    g_sdram_test_result = sdram_test;
    if (SDRAM_TEST_STATUS_PASS == sdram_test.status)
    {
        APP_PRINT("SDRAM framebuffer self-test PASS\r\n");
    }
    else
    {
        APP_PRINT("SDRAM framebuffer self-test FAIL: pattern=0x%08lX index=%lu expected=0x%08lX actual=0x%08lX\r\n",
                  sdram_test.failed_pattern,
                  sdram_test.failed_index,
                  sdram_test.expected,
                  sdram_test.actual);
    }
#endif

    fsp_err_t error;
    /* Initialize LVGL. */
    lv_init();
    /* Initialize the LVGL port driver (GLCDC + display + framebuffer). */
    error = RM_LVGL_PORT_Open(&g_lvgl_port_ctrl, &g_lvgl_port_cfg);
    if (FSP_SUCCESS != error)
    {
        APP_PRINT("** RM_LVGL_PORT_Open FAILED **\r\n");
    }
    /* Keep LVGL's framebuffer format aligned with GLCDC 32-bit RGB888 input.
     * LV_COLOR_DEPTH=32 maps LVGL native rendering to XRGB8888.
     */
    lv_display_set_color_format(lv_display_get_default(), LV_COLOR_FORMAT_XRGB8888);

#if LCD_FRAMEBUFFER_TEST_MODE
    if (SDRAM_TEST_STATUS_PASS == sdram_test.status)
    {
        lcd_framebuffer_test_pattern();
    }
    else
    {
        lcd_framebuffer_fail_pattern(&sdram_test);
    }
    LCD_Backlight_ON();

    while(1)
    {
        R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);
    }
#endif

    /* IIC1 remains global because it is shared by board peripherals.  The
     * production remote build disables its local OV5640 source and uses IIC1
     * only for the GT911 touch controller. */
    error = i2c_control_init();
#if APP_CAMERA_CAPTURE_ENABLE
    if (FSP_SUCCESS == error)
    {
        error = CameraCapture_Init();
    }
    APP_PRINT("[CAM] init=%08lX OV5640 %ux%u stride=%u rst=P012\r\n",
              (uint32_t) error,
              (uint32_t) CAMERA_CAPTURE_WIDTH,
              (uint32_t) CAMERA_CAPTURE_HEIGHT,
              (uint32_t) CAMERA_CAPTURE_STRIDE);
#else
    APP_PRINT("[CAM] local capture disabled; video source is vehicle channel %u\r\n",
              (uint32_t) WIRELESS_VIDEO_RX_CHANNEL);
#endif

    /* Initialize input device (touch); i2c_control_init is idempotent. */
    lv_port_indev_init();

    /* Start SPI only after the touch/IIC interrupt path is live.  Besides avoiding
     * an overly early first transfer, this gives the radio's power-on reset time
     * to complete before the first register read. */
    nrf24_result_t radio_result = WirelessRemoteLinks_Init();
#if NRF24_PORT_SOFTWARE_SPI
    APP_PRINT("[NRF] init=%u rx0=%u/%u/%u ch=%u tx1=%u/%u/%u ch=%u soft done=%lu/%lu timeout=%lu fsp=%08lX\r\n",
              (uint32_t) radio_result,
              (uint32_t) WirelessTouchRx_GetInitResult(),
              WirelessTouchRx_IsConnected() ? 1U : 0U,
              WirelessTouchRx_IsReady() ? 1U : 0U,
              WIRELESS_VIDEO_RX_CHANNEL,
              (uint32_t) WirelessTouchTx_GetInitResult(),
              WirelessTouchTx_IsConnected() ? 1U : 0U,
              WirelessTouchTx_IsReady() ? 1U : 0U,
              WIRELESS_COMMAND_TX_CHANNEL,
              g_nrf24_spi0_transaction_count,
              g_nrf24_spi1_transaction_count,
              g_nrf24_spi_timeout_count,
              (uint32_t) g_nrf24_spi_last_fsp_error);
#if NRF24_RX_IRQ_NOTIFICATION_ENABLE
    APP_PRINT("[NRF RX] IRQ P105/IRQ0 cb=%lu\r\n",
              Nrf24Port_RxIrqCallbackCountGet());
#else
    APP_PRINT("[NRF RX] polling\r\n");
#endif
#else
    APP_PRINT("[NRF24] init=%u tx_ready=%u tx_ch=%u rx_ready=%u rx_ch=%u spi=hardware cb=%lu timeout=%lu event=%u fsp=0x%08lX tx=%lu rx=%lu count=%lu spcr=%08lX spsr=%08lX sppsr=%08lX ielsr_rxi=%08lX iser=%08lX primask=%lu\r\n",
              (uint32_t) radio_result,
              WirelessTouchTx_IsReady() ? 1U : 0U,
              WIRELESS_COMMAND_TX_CHANNEL,
              WirelessTouchRx_IsReady() ? 1U : 0U,
              WIRELESS_VIDEO_RX_CHANNEL,
              g_nrf24_spi_callback_count,
              g_nrf24_spi_timeout_count,
              (uint32_t) g_nrf24_spi_last_event,
              (uint32_t) g_nrf24_spi_last_fsp_error,
              g_spi0_ctrl.tx_count,
              g_spi0_ctrl.rx_count,
              g_spi0_ctrl.count,
              g_spi0_ctrl.p_regs->SPCR,
              g_spi0_ctrl.p_regs->SPSR,
              g_spi0_ctrl.p_regs->SPPSR,
              R_ICU->IELSR[(uint32_t) g_spi0_cfg.rxi_irq],
              NVIC->ISER[0],
              __get_PRIMASK());
#endif
    /* Create user defined UI using LVGL widgets. */
    setup_ui(&guider_ui);
    events_init(&guider_ui);
    /* Turn on LCD backlight. */
    LCD_Backlight_ON();

    FreeRtosApp_NotifyInitialized();
    APP_PRINT("[RTOS] GUI=P4 VIDEO_RX=P3 COMMAND_TX=P2 TOUCH=P1\r\n");
    APP_PRINT("[LINK] command_tx_ch=%u video_rx_ch=%u rate=2Mbps command_ack=on local_camera=%u loopback=%u\r\n",
              (uint32_t) WIRELESS_COMMAND_TX_CHANNEL,
              (uint32_t) WIRELESS_VIDEO_RX_CHANNEL,
              (uint32_t) APP_CAMERA_CAPTURE_ENABLE,
              (uint32_t) APP_LOCAL_VIDEO_LOOPBACK_ENABLE);
    APP_PRINT("[VIDEO RX] jpeg=gray-%ux%u display=RGB888-%ux%u payload=%u source=vehicle\r\n",
              (uint32_t) IMAGE_SOURCE_WIDTH,
              (uint32_t) IMAGE_SOURCE_HEIGHT,
              (uint32_t) IMAGE_DISPLAY_WIDTH,
              (uint32_t) IMAGE_DISPLAY_HEIGHT,
              (uint32_t) IMAGE_PACKET_DATA_SIZE);

    uint32_t rtt_heartbeat_count = 0U;
    uint32_t received_packet_count = 0U;
    nrf24_result_t last_rx_error = NRF24_RESULT_SUCCESS;
#if NRF24_RX_IRQ_NOTIFICATION_ENABLE
    fsp_err_t last_irq_take_error = FSP_SUCCESS;
#endif
    while(1)
    {
        bool service_rx_fifo = true;
#if APP_VIDEO_RX_ENABLE
        if (g_image_rx_state.active &&
            ((uint32_t) (lv_tick_get() - g_image_rx_state.last_packet_tick_ms) >=
             IMAGE_RX_STALL_TIMEOUT_MS))
        {
            APP_PRINT("[IMG RX] timeout-drop f=%u chunks=%u/%u miss=%lu; accepting next frame\r\n",
                      (uint32_t) g_image_rx_state.frame_id,
                      (uint32_t) g_image_rx_state.received_chunks,
                      (uint32_t) g_image_rx_state.chunk_count,
                      g_image_rx_state.missing_chunks);
            g_image_rx_state.active = false;
        }
#endif
#if NRF24_RX_IRQ_NOTIFICATION_ENABLE
        bool rx_irq_pending = false;
        fsp_err_t irq_take_error = Nrf24Port_RxIrqPendingTake(&rx_irq_pending);
        service_rx_fifo = rx_irq_pending;

        if (FSP_SUCCESS != irq_take_error)
        {
            if (last_irq_take_error != irq_take_error)
            {
                APP_PRINT("[NRF24 IRQ SPI0] pending_take_error=0x%08lX\r\n",
                          (uint32_t) irq_take_error);
            }
            service_rx_fifo = false;
        }
        else if (rx_irq_pending)
        {
            uint32_t irq_count = Nrf24Port_RxIrqCallbackCountGet();
            if ((irq_count <= 3U) || (0U == (irq_count % IMAGE_PROGRESS_INTERVAL)))
            {
                APP_PRINT("[NRF IRQ19] cb=%lu RX FIFO\r\n",
                          irq_count);
            }
        }
        last_irq_take_error = irq_take_error;
#endif

        /* Move the three-entry nRF hardware FIFO into the software RX ring first. */
        if (service_rx_fifo)
        {
            uint32_t queued_packets = 0U;
            nrf24_result_t rx_service_result = WirelessRadioRx_Service(3U, &queued_packets);

            if (NRF24_RESULT_SUCCESS != rx_service_result)
            {
                if (last_rx_error != rx_service_result)
                {
                    APP_PRINT("[RXQ ERR] service=%u n=%lu\r\n",
                              (uint32_t) rx_service_result,
                              WirelessRadioRx_RingCountGet());
                }
                last_rx_error = rx_service_result;
            }
            else
            {
                last_rx_error = NRF24_RESULT_SUCCESS;
            }
        }

        /* Protocol parsing is decoupled from the IRQ/SPI read and consumes the software RX ring. */
        for (uint32_t rx_index = 0U; rx_index < WIRELESS_RADIO_RX_RING_CAPACITY; rx_index++)
        {
            uint8_t payload[WIRELESS_RADIO_MAX_PAYLOAD_LENGTH] = {0U};
            uint8_t payload_length = 0U;
            bool packet_received = false;
            nrf24_result_t rx_result = WirelessRadioRx_DequeuePayload(payload,
                                                                      sizeof(payload),
                                                                      &payload_length,
                                                                      &packet_received);

            if (NRF24_RESULT_SUCCESS != rx_result)
            {
                if (last_rx_error != rx_result)
                {
                    APP_PRINT("[RXQ ERR] get=%u n=%lu\r\n",
                              (uint32_t) rx_result,
                              WirelessRadioRx_RingCountGet());
                }
                last_rx_error = rx_result;
                break;
            }

            last_rx_error = NRF24_RESULT_SUCCESS;
            if (!packet_received)
            {
                break;
            }

            received_packet_count++;
#if APP_VIDEO_RX_CONTROL_COMPAT_ENABLE
            /* Historical loopback parser retained but excluded from the
             * production build.  Video RX channel 100 must carry images only. */
            if ((WIRELESS_TOUCH_PAYLOAD_LENGTH == payload_length) &&
                (WIRELESS_TOUCH_MAGIC == payload[0]))
            {
                wireless_touch_packet_t packet;
                uint16_t value;

                (void) memcpy(&packet, payload, sizeof(packet));
                value = (uint16_t) packet.value_lsb |
                        (uint16_t) ((uint16_t) packet.value_msb << 8U);
                if (WirelessTouchPacket_IsValid(&packet))
                {
                    APP_PRINT("[CONTROL COMPAT RX] seq=%u control=%s(%u) action=%s(%u) value=%u\r\n",
                              (uint32_t) packet.sequence,
                              wireless_touch_control_name(packet.control),
                              (uint32_t) packet.control,
                              wireless_touch_action_name(packet.action),
                              (uint32_t) packet.action,
                              (uint32_t) value);
                }
            }
            else
#endif
#if APP_VIDEO_RX_ENABLE
            if (image_test_rx_process(payload, payload_length))
            {
                /* Video RX owns channel 100 and consumes image traffic only. */
            }
            else
#endif
            {
                APP_PRINT("[VIDEO RX] rejected non-image packet count=%lu length=%u magic=%02X\r\n",
                          received_packet_count,
                          (uint32_t) payload_length,
                          (uint32_t) payload[0]);
            }
        }

        rtt_heartbeat_count++;
        if (1000U <= rtt_heartbeat_count)
        {
            rtt_heartbeat_count = 0U;
            APP_PRINT("[VIDEO RX] ready=%u ch=%u packets=%lu rxq=%lu/%u frame=%u chunks=%u/%u\r\n",
                       WirelessTouchRx_IsReady() ? 1U : 0U,
                       (uint32_t) WIRELESS_VIDEO_RX_CHANNEL,
                       received_packet_count,
                       WirelessRadioRx_RingCountGet(),
                       (uint32_t) WIRELESS_RADIO_RX_RING_CAPACITY,
#if APP_VIDEO_RX_ENABLE
                       (uint32_t) g_image_rx_state.frame_id,
                       (uint32_t) g_image_rx_state.received_chunks,
                       (uint32_t) g_image_rx_state.chunk_count
#else
                       0U, 0U, 0U
#endif
                       );
        }
#if APP_VIDEO_RX_ENABLE
        if (g_image_rx_state.active)
        {
            /* Drain a frame without inserting a complete RTOS tick between
             * consecutive radio FIFO reads. */
            taskYIELD();
        }
        else
#endif
        {
            vTaskDelay(pdMS_TO_TICKS(1U));
        }
    }
#endif

    /* Wake up 2nd core if this is first core and we are inside a multicore project. */
#if (0 == _RA_CORE) && (1 == BSP_MULTICORE_PROJECT) && !BSP_TZ_NONSECURE_BUILD

#if BSP_TZ_SECURE_BUILD
    /* Take semaphore so 2nd core can clear it */
    R_BSP_IpcSemaphoreTake(&g_core_start_semaphore);
#endif

    R_BSP_SecondaryCoreStart();

#if BSP_TZ_SECURE_BUILD
    /* Wait for 2nd core to start and clear semaphore */
    while(FSP_ERR_IN_USE == R_BSP_IpcSemaphoreTake(&g_core_start_semaphore))
    {
        ;
    }
#endif
#endif

#if (1 == _RA_CORE) && (1 == BSP_MULTICORE_PROJECT) && BSP_TZ_SECURE_BUILD
    /* Signal to 1st core that 2nd core has started */
    R_BSP_IpcSemaphoreGive(&g_core_start_semaphore);
#endif

#if BSP_TZ_SECURE_BUILD
    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif
}

void AppGuiTask_Entry(void * pv_parameters)
{
    FSP_PARAMETER_NOT_USED(pv_parameters);
    FreeRtosApp_WaitInitialized();
    bool first_frame = true;

    APP_PRINT("[GUI] task started\r\n");

    while (1)
    {
#if APP_VIDEO_RX_ENABLE
        /* GUI Guider deletes the active main screen during PageToAbout() and
         * creates a new main_img_1 object during PageToMain().  Reattach the
         * stable image descriptor whenever the widget instance (or its source)
         * changes, even if no newly decoded frame is pending at that instant. */
        lv_obj_t * p_main_image = guider_ui.main_img_1;
        if (g_image_present_source_installed &&
            lv_obj_is_valid(p_main_image) &&
            ((g_image_present_owner != p_main_image) ||
             (lv_image_get_src(p_main_image) != &g_image_present_dsc)))
        {
            lv_image_set_src(p_main_image, &g_image_present_dsc);
            lv_obj_invalidate(p_main_image);
            g_image_present_owner = p_main_image;
            APP_PRINT("[DISPLAY] source reattached widget=%p after page recreation\r\n",
                      (void *) p_main_image);
        }

        if (g_image_display_pending)
        {
            __DMB();
            uint8_t display_buffer_index = g_image_display_pending_index;
            p_main_image = guider_ui.main_img_1;
            if (lv_obj_is_valid(p_main_image))
            {
                /* The previous lv_timer_handler() has completed its draw before
                 * this task reaches here, so the fixed presentation source can
                 * safely be updated.  Do not replace the LVGL source each frame:
                 * copy a complete frame, clean D-cache, then request one redraw. */
                if (g_image_present_source_installed)
                {
                    lv_image_cache_drop(&g_image_present_dsc);
                }
                (void) memcpy(g_image_present_buffer,
                              g_image_rx_buffer[display_buffer_index],
                              IMAGE_DECOMPRESSED_BUFFER_SIZE);
                SCB_CleanDCache_by_Addr((uint32_t *) (void *) g_image_present_buffer,
                                        (int32_t) IMAGE_DECOMPRESSED_BUFFER_SIZE);
                __DMB();
                if (!g_image_present_source_installed ||
                    (g_image_present_owner != p_main_image) ||
                    (lv_image_get_src(p_main_image) != &g_image_present_dsc))
                {
                    image_prepare_rgb888_descriptor();
                    lv_image_set_src(p_main_image, &g_image_present_dsc);
                    g_image_present_source_installed = true;
                    g_image_present_owner = p_main_image;
                    APP_PRINT("[DISPLAY] stable-source installed buffer=%p mode=single-invalidate+VSYNC\r\n",
                              (void *) g_image_present_buffer);
                }
                lv_obj_invalidate(p_main_image);
                g_image_display_active_index = display_buffer_index;
            }
            __DMB();
            g_image_display_pending = false;
        }
#endif
        lv_timer_handler();
        if (first_frame)
        {
            first_frame = false;
            APP_PRINT("[GUI] first frame complete\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(2U));
    }
}

void AppTouchTask_Entry(void * pv_parameters)
{
    FSP_PARAMETER_NOT_USED(pv_parameters);
    FreeRtosApp_WaitInitialized();

    /* GT911 is sampled by LVGL from the display thread.  Keep this FSP-created
     * thread available for future touch-side work without accessing LVGL from
     * a second task. */
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(20U));
    }
}

void AppCommandTxTask_Entry(void * pv_parameters)
{
    FSP_PARAMETER_NOT_USED(pv_parameters);
    FreeRtosApp_WaitInitialized();
    uint32_t transmitted_packet_count = 0U;
    uint32_t tx_service_error_count = 0U;
    nrf24_result_t last_tx_error = NRF24_RESULT_SUCCESS;

    APP_PRINT("[COMMAND TX] started ch=%u control-only\r\n",
              (uint32_t) WIRELESS_COMMAND_TX_CHANNEL);

    while (1)
    {
#if APP_LOCAL_VIDEO_LOOPBACK_ENABLE
        /* Retained diagnostic source: when explicitly enabled this task may
         * enqueue locally captured JPEG packets.  Production keeps it out of
         * compilation so Command TX owns touch-control traffic only. */
        image_test_dynamic_service();
        while ((WirelessRadioTx_RingCountGet() < 3U) &&
               (IMAGE_TX_STAGE_IDLE != g_image_tx_state.stage))
        {
            uint32_t ring_count_before = WirelessRadioTx_RingCountGet();
            image_test_tx_service();
            if (WirelessRadioTx_RingCountGet() == ring_count_before)
            {
                break;
            }
        }
#endif

        uint32_t packets_sent = 0U;
        nrf24_result_t result = WirelessRadioTx_Service(&packets_sent);
        if (NRF24_RESULT_SUCCESS != result)
        {
            tx_service_error_count++;
            if ((last_tx_error != result) ||
                (tx_service_error_count <= 3U) ||
                (0U == (tx_service_error_count % 64U)))
            {
                APP_PRINT("[COMMAND TX] error=%u count=%lu queued=%lu\r\n",
                          (uint32_t) result,
                          tx_service_error_count,
                          WirelessRadioTx_RingCountGet());
            }
            last_tx_error = result;
            vTaskDelay(pdMS_TO_TICKS(2U));
            continue;
        }

        last_tx_error = NRF24_RESULT_SUCCESS;
        transmitted_packet_count += packets_sent;
        if (0U == packets_sent)
        {
            vTaskDelay(pdMS_TO_TICKS(2U));
        }
        else if (0U == (transmitted_packet_count % 64U))
        {
            APP_PRINT("[COMMAND TX] sent=%lu queued=%lu\r\n",
                      transmitted_packet_count,
                      WirelessRadioTx_RingCountGet());
        }
    }
}

#if BSP_TZ_SECURE_BUILD

FSP_CPP_HEADER
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ();

/* Trustzone Secure Projects require at least one nonsecure callable function in order to build (Remove this if it is not required to build). */
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ()
{

}
FSP_CPP_FOOTER

#endif
