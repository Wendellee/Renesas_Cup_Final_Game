#include "app_config.h"
#include "CAMERA/camera_capture.h"

#if APP_CAMERA_CAPTURE_ENABLE

#include "CAMERA/camera_sensor.h"
#include <string.h>

static volatile uint8_t * gp_camera_latest_frame;
static volatile uint32_t  g_camera_frame_count;
static volatile uint32_t  g_camera_error_count;

/* The OV5640 SCCB/I2C interface is clocked from the sensor's external XCLK.
 * P05_01 is already configured as GPT12/GTIOCA by the FSP pin configuration,
 * but this project has no GPT12 stack instance.  Start GPT12 here without
 * changing configuration.xml.  PCLKD is 250 MHz, so 10 timer clocks generate
 * a 25 MHz XCLK (within the OV5640 6..27 MHz input range). */
static void camera_xclk_start(void)
{
    uint32_t const channel_mask = (1UL << 12U);

    /* Select PCLKD as the GPT count clock, matching BSP_CFG_GPT_COUNT_CLOCK_SOURCE. */
    R_GPT_GTCLK->GTCLKCR = 1U;
    R_BSP_MODULE_START(FSP_IP_GPT, 12U);

    /* Disable GPT register protection and stop channel 12 before configuring it. */
    R_GPT12->GTWP  = 0xA500U;
    R_GPT12->GTSTP = channel_mask;
    R_GPT12->GTCR  = 0U;
    R_GPT12->GTST  = 0U;
    R_GPT12->GTCNT = 0U;

    /* Saw-wave PWM, 10 PCLKD ticks per period, 50 percent duty on GTIOCA. */
    R_GPT12->GTSSR   = 0U;
    R_GPT12->GTPSR   = 0U;
    R_GPT12->GTCSR   = 0U;
    R_GPT12->GTUPSR  = 0U;
    R_GPT12->GTDNSR  = 0U;
    R_GPT12->GTPR    = 9U;
    R_GPT12->GTPBR   = 9U;
    R_GPT12->GTCCR[2] = 4U;       /* GTCCRC: duty-cycle buffer for GTIOCA. */
    R_GPT12->GTBER   = 0x550000U; /* Enable buffer and force transfer. */
    R_GPT12->GTIOR   = 0x00000106U; /* OAE=1; low at compare, high at cycle end. */
    R_GPT12->GTINTAD = 0U;
    R_GPT12->GTUDDTYC = 3U;
    R_GPT12->GTUDDTYC = 1U;       /* Force up-counting. */
    R_GPT12->GTCLR   = channel_mask;
    R_GPT12->GTWP    = 0xA501U;
    R_GPT12->GTSTR   = channel_mask;

    R_BSP_SoftwareDelay(2U, BSP_DELAY_UNITS_MILLISECONDS);
}

static void camera_bus_diagnostic(void)
{
    bsp_io_level_t sda = BSP_IO_LEVEL_LOW;
    bsp_io_level_t scl = BSP_IO_LEVEL_LOW;
    bsp_io_level_t rst = BSP_IO_LEVEL_LOW;
    uint32_t       count_a;
    uint32_t       count_b;

    (void) R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_05_PIN_11, &sda);
    (void) R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_05_PIN_12, &scl);
    (void) R_IOPORT_PinRead(&g_ioport_ctrl, BSP_IO_PORT_00_PIN_12, &rst);
    count_a = R_GPT12->GTCNT;
    __NOP();
    __NOP();
    count_b = R_GPT12->GTCNT;

    APP_PRINT("[CAM HW] xclk=25MHz gpt12_run=%lu period=%lu count=%lu->%lu sda=%u scl=%u rst=%u\r\n",
              (uint32_t) R_GPT12->GTCR_b.CST,
              R_GPT12->GTPR + 1U,
              count_a,
              count_b,
              (unsigned int) sda,
              (unsigned int) scl,
              (unsigned int) rst);
}

fsp_err_t CameraCapture_Init(void)
{
    fsp_err_t err = FSP_ERR_TRANSFER_ABORTED;

    gp_camera_latest_frame = NULL;
    g_camera_frame_count   = 0U;
    g_camera_error_count   = 0U;

    memset(vin_image_buffer_1, 0, VIN_BYTES_PER_FRAME);
    memset(vin_image_buffer_2, 0, VIN_BYTES_PER_FRAME);
    memset(vin_image_buffer_3, 0, VIN_BYTES_PER_FRAME);

    /* XCLK must be stable before the OV5640 reset is released and before the
     * first SCCB/I2C transaction.  Without it the sensor NACKs address 0x3c. */
    camera_xclk_start();
    camera_bus_diagnostic();

    /* OV5640 can NACK the first SCCB transaction while its internal clock and
     * reset state settle.  A single failed attempt used to disable capture for
     * the entire boot.  Recover the IIC peripheral and repeat the complete
     * address-select + hardware-reset + register-programming sequence. */
    for (uint32_t attempt = 1U; attempt <= 3U; attempt++)
    {
        if (1U != attempt)
        {
            (void) R_IIC_MASTER_Abort(&g_i2c_master_for_peripheral_ctrl);
            R_BSP_SoftwareDelay(20U * attempt, BSP_DELAY_UNITS_MILLISECONDS);
            APP_PRINT("[CAM] retry=%lu after I2C NACK/abort\r\n", attempt);
        }

        /* camera_open selects OV5640 I2C address 0x3c, resets P012 and
         * programs the 1024x600 YUV422 MIPI stream used by the VIN config. */
        err = camera_open();
        if (FSP_SUCCESS == err)
        {
            break;
        }
    }
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = R_VIN_Open(&g_vin_ctrl, &g_vin_cfg);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = R_VIN_CaptureStart(&g_vin_ctrl, NULL);
    if (FSP_SUCCESS != err)
    {
        (void) R_VIN_Close(&g_vin_ctrl);
        return err;
    }

    err = camera_stream_on();
    if (FSP_SUCCESS != err)
    {
        (void) R_VIN_Close(&g_vin_ctrl);
        return err;
    }

    return FSP_SUCCESS;
}

bool CameraCapture_GetLatestFrame(uint8_t const ** pp_frame, uint32_t * p_sequence)
{
    if ((NULL == pp_frame) || (NULL == p_sequence))
    {
        return false;
    }

    __DMB();
    *pp_frame  = (uint8_t const *) gp_camera_latest_frame;
    *p_sequence = g_camera_frame_count;
    __DMB();

    return (NULL != *pp_frame) && (0U != *p_sequence);
}

uint32_t CameraCapture_FrameCountGet(void)
{
    return g_camera_frame_count;
}

uint32_t CameraCapture_ErrorCountGet(void)
{
    return g_camera_error_count;
}

void vin_callback(capture_callback_args_t * p_args)
{
    vin_interrupt_status_t status = (vin_interrupt_status_t) p_args->interrupt_status;

    if ((VIN_EVENT_NOTIFY == p_args->event) && status.bits.frame_complete && (NULL != p_args->p_buffer))
    {
        gp_camera_latest_frame = p_args->p_buffer;
        __DMB();
        g_camera_frame_count++;
    }
    else if (VIN_EVENT_ERROR == p_args->event)
    {
        g_camera_error_count++;
    }
}

void mipi_csi0_callback(mipi_csi_callback_args_t * p_args)
{
    /* VIN owns frame delivery. MIPI events are informational here; treating
     * normal RX-active/EoTp notifications as capture errors would be wrong. */
    FSP_PARAMETER_NOT_USED(p_args);
}

#else

/* configuration.xml still owns the dormant VIN/MIPI stack, so its generated
 * configuration keeps these callback symbols.  The production build never
 * opens the local capture peripheral; the stubs only satisfy that generated
 * linkage without compiling the camera pipeline. */
void vin_callback(capture_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
}

void mipi_csi0_callback(mipi_csi_callback_args_t * p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
}

#endif /* APP_CAMERA_CAPTURE_ENABLE */
