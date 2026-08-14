/* generated thread source file - do not edit */
#include "camera_thread.h"

#if 1
static StaticTask_t camera_thread_memory;
#if defined(__ARMCC_VERSION)           /* AC6 compiler */
                static uint8_t camera_thread_stack[0x1800] BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".stack.thread") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
                #else
static uint8_t camera_thread_stack[0x1800] BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".stack.camera_thread") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
#endif
#endif
TaskHandle_t camera_thread;
void camera_thread_create(void);
static void camera_thread_func(void *pvParameters);
void rtos_startup_err_callback(void *p_instance, void *p_data);
void rtos_startup_common_init(void);
dtc_instance_ctrl_t g_transfer1_ctrl;

#if (BSP_CFG_DCACHE_ENABLED) && (1 == 1)
const transfer_info_t g_transfer1_user_config_info =
{
    .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_INCREMENTED,
    .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_DESTINATION,
    .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
    .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
    .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_FIXED,
    .transfer_settings_word_b.size           = TRANSFER_SIZE_1_BYTE,
    .transfer_settings_word_b.mode           = TRANSFER_MODE_NORMAL,
    .p_dest                                  = (void *) NULL,
    .p_src                                   = (void const *) NULL,
    .num_blocks                              = (uint16_t) 0,
    .length                                  = (uint16_t) 0,
};
#endif

#if BSP_CFG_DCACHE_ENABLED
    #if (1 > 0)
    transfer_info_t g_transfer1_info_fsp_nocache[1] DTC_TRANSFER_INFO_ALIGNMENT;
    #else
    /* User must call api::reconfigure before enable DTC transfer. */
    #endif
#else
#if (1 == 1)
transfer_info_t g_transfer1_info DTC_TRANSFER_INFO_ALIGNMENT =
		{ .transfer_settings_word_b.dest_addr_mode =
				TRANSFER_ADDR_MODE_INCREMENTED,
				.transfer_settings_word_b.repeat_area =
						TRANSFER_REPEAT_AREA_DESTINATION,
				.transfer_settings_word_b.irq = TRANSFER_IRQ_END,
				.transfer_settings_word_b.chain_mode =
						TRANSFER_CHAIN_MODE_DISABLED,
				.transfer_settings_word_b.src_addr_mode =
						TRANSFER_ADDR_MODE_FIXED,
				.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE,
				.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL, .p_dest =
						(void*) NULL, .p_src = (void const*) NULL, .num_blocks =
						(uint16_t) 0, .length = (uint16_t) 0, };
#elif (1 > 1)
    /* User is responsible to initialize the array. */
    transfer_info_t g_transfer1_info[1] DTC_TRANSFER_INFO_ALIGNMENT;
    #else
    /* User must call api::reconfigure before enable DTC transfer. */
    #endif
#endif

const dtc_extended_cfg_t g_transfer1_cfg_extend = { .activation_source =
		VECTOR_NUMBER_IIC1_RXI,

#if BSP_CFG_DCACHE_ENABLED
    #if (1 == 1)
        .p_user_config_info =  &g_transfer1_user_config_info,
    #else
        .p_user_config_info = NULL,
    #endif
#else
		/* p_user_config_info not present. */
#endif
		};

const transfer_cfg_t g_transfer1_cfg = {
#if BSP_CFG_DCACHE_ENABLED
    #if (1 > 0)
        .p_info              = g_transfer1_info_fsp_nocache,
    #else
        .p_info = NULL,
    #endif
#else
#if (1 == 1)
		.p_info = &g_transfer1_info,
#elif (1 > 1)
        .p_info              = g_transfer1_info,
    #else
        .p_info = NULL,
    #endif
#endif
		.p_extend = &g_transfer1_cfg_extend, };

/* Instance structure to use this module. */
const transfer_instance_t g_transfer1 = { .p_ctrl = &g_transfer1_ctrl, .p_cfg =
		&g_transfer1_cfg, .p_api = &g_transfer_on_dtc };
dtc_instance_ctrl_t g_transfer0_ctrl;

#if (BSP_CFG_DCACHE_ENABLED) && (1 == 1)
const transfer_info_t g_transfer0_user_config_info =
{
    .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
    .transfer_settings_word_b.repeat_area    = TRANSFER_REPEAT_AREA_SOURCE,
    .transfer_settings_word_b.irq            = TRANSFER_IRQ_END,
    .transfer_settings_word_b.chain_mode     = TRANSFER_CHAIN_MODE_DISABLED,
    .transfer_settings_word_b.src_addr_mode  = TRANSFER_ADDR_MODE_INCREMENTED,
    .transfer_settings_word_b.size           = TRANSFER_SIZE_1_BYTE,
    .transfer_settings_word_b.mode           = TRANSFER_MODE_NORMAL,
    .p_dest                                  = (void *) NULL,
    .p_src                                   = (void const *) NULL,
    .num_blocks                              = (uint16_t) 0,
    .length                                  = (uint16_t) 0,
};
#endif

#if BSP_CFG_DCACHE_ENABLED
    #if (1 > 0)
    transfer_info_t g_transfer0_info_fsp_nocache[1] DTC_TRANSFER_INFO_ALIGNMENT;
    #else
    /* User must call api::reconfigure before enable DTC transfer. */
    #endif
#else
#if (1 == 1)
transfer_info_t g_transfer0_info DTC_TRANSFER_INFO_ALIGNMENT =
		{ .transfer_settings_word_b.dest_addr_mode = TRANSFER_ADDR_MODE_FIXED,
				.transfer_settings_word_b.repeat_area =
						TRANSFER_REPEAT_AREA_SOURCE,
				.transfer_settings_word_b.irq = TRANSFER_IRQ_END,
				.transfer_settings_word_b.chain_mode =
						TRANSFER_CHAIN_MODE_DISABLED,
				.transfer_settings_word_b.src_addr_mode =
						TRANSFER_ADDR_MODE_INCREMENTED,
				.transfer_settings_word_b.size = TRANSFER_SIZE_1_BYTE,
				.transfer_settings_word_b.mode = TRANSFER_MODE_NORMAL, .p_dest =
						(void*) NULL, .p_src = (void const*) NULL, .num_blocks =
						(uint16_t) 0, .length = (uint16_t) 0, };
#elif (1 > 1)
    /* User is responsible to initialize the array. */
    transfer_info_t g_transfer0_info[1] DTC_TRANSFER_INFO_ALIGNMENT;
    #else
    /* User must call api::reconfigure before enable DTC transfer. */
    #endif
#endif

const dtc_extended_cfg_t g_transfer0_cfg_extend = { .activation_source =
		VECTOR_NUMBER_IIC1_TXI,

#if BSP_CFG_DCACHE_ENABLED
    #if (1 == 1)
        .p_user_config_info =  &g_transfer0_user_config_info,
    #else
        .p_user_config_info = NULL,
    #endif
#else
		/* p_user_config_info not present. */
#endif
		};

const transfer_cfg_t g_transfer0_cfg = {
#if BSP_CFG_DCACHE_ENABLED
    #if (1 > 0)
        .p_info              = g_transfer0_info_fsp_nocache,
    #else
        .p_info = NULL,
    #endif
#else
#if (1 == 1)
		.p_info = &g_transfer0_info,
#elif (1 > 1)
        .p_info              = g_transfer0_info,
    #else
        .p_info = NULL,
    #endif
#endif
		.p_extend = &g_transfer0_cfg_extend, };

/* Instance structure to use this module. */
const transfer_instance_t g_transfer0 = { .p_ctrl = &g_transfer0_ctrl, .p_cfg =
		&g_transfer0_cfg, .p_api = &g_transfer_on_dtc };
iic_master_instance_ctrl_t g_i2c_master_for_peripheral_ctrl;
const iic_master_extended_cfg_t g_i2c_master_for_peripheral_extend =
		{ .timeout_mode = IIC_MASTER_TIMEOUT_MODE_SHORT, .timeout_scl_low =
				IIC_MASTER_TIMEOUT_SCL_LOW_ENABLED, .smbus_operation = 0,
				/* Actual calculated bitrate: 393082. Actual calculated duty cycle: 50%. */.clock_settings.brl_value =
						15, .clock_settings.brh_value = 15,
				.clock_settings.cks_value = 2, .clock_settings.sddl_value = 0,
				.clock_settings.dlcs_value = 0, };
const i2c_master_cfg_t g_i2c_master_for_peripheral_cfg = { .channel = 1, .rate =
		I2C_MASTER_RATE_FAST, .slave = 0x00, .addr_mode =
		I2C_MASTER_ADDR_MODE_7BIT,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == g_transfer0)
                .p_transfer_tx       = NULL,
#else
		.p_transfer_tx = &g_transfer0,
#endif
#if (RA_NOT_DEFINED == g_transfer1)
                .p_transfer_rx       = NULL,
#else
		.p_transfer_rx = &g_transfer1,
#endif
#undef RA_NOT_DEFINED
		.p_callback = g_i2c_master_for_peripheral_callback, .p_context = NULL,
#if defined(VECTOR_NUMBER_IIC1_RXI)
    .rxi_irq             = VECTOR_NUMBER_IIC1_RXI,
#else
		.rxi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC1_TXI)
    .txi_irq             = VECTOR_NUMBER_IIC1_TXI,
#else
		.txi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC1_TEI)
    .tei_irq             = VECTOR_NUMBER_IIC1_TEI,
#else
		.tei_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_IIC1_ERI)
    .eri_irq             = VECTOR_NUMBER_IIC1_ERI,
#else
		.eri_irq = FSP_INVALID_VECTOR,
#endif
		.ipl = (0), .p_extend = &g_i2c_master_for_peripheral_extend, };
/* Instance structure to use this module. */
const i2c_master_instance_t g_i2c_master_for_peripheral = { .p_ctrl =
		&g_i2c_master_for_peripheral_ctrl, .p_cfg =
		&g_i2c_master_for_peripheral_cfg, .p_api = &g_i2c_master_on_iic };
extern uint32_t g_fsp_common_thread_count;

const rm_freertos_port_parameters_t camera_thread_parameters = { .p_context =
		(void*) NULL, };

void camera_thread_create(void) {
	/* Increment count so we will know the number of threads created in the RA Configuration editor. */
	g_fsp_common_thread_count++;

	/* Initialize each kernel object. */

#if 1
	camera_thread = xTaskCreateStatic(
#else
                    BaseType_t camera_thread_create_err = xTaskCreate(
                    #endif
			camera_thread_func, (const char*) "Camera Thread", 0x1800 / 4, // In words, not bytes
			(void*) &camera_thread_parameters, //pvParameters
			3,
#if 1
			(StackType_t*) &camera_thread_stack,
			(StaticTask_t*) &camera_thread_memory
#else
                        & camera_thread
                        #endif
			);

#if 1
	if (NULL == camera_thread) {
		rtos_startup_err_callback(camera_thread, 0);
	}
#else
                    if (pdPASS != camera_thread_create_err)
                    {
                        rtos_startup_err_callback(camera_thread, 0);
                    }
                    #endif
}
static void camera_thread_func(void *pvParameters) {
	/* Initialize common components */
	rtos_startup_common_init();

	/* Initialize each module instance. */

#if (1 == BSP_TZ_NONSECURE_BUILD) && (1 == 1)
                    /* When FreeRTOS is used in a non-secure TrustZone application, portALLOCATE_SECURE_CONTEXT must be called prior
                     * to calling any non-secure callable function in a thread. The parameter is unused in the FSP implementation.
                     * If no slots are available then configASSERT() will be called from vPortSVCHandler_C(). If this occurs, the
                     * application will need to either increase the value of the "Process Stack Slots" Property in the rm_tz_context
                     * module in the secure project or decrease the number of threads in the non-secure project that are allocating
                     * a secure context. Users can control which threads allocate a secure context via the Properties tab when
                     * selecting each thread. Note that the idle thread in FreeRTOS requires a secure context so the application
                     * will need at least 1 secure context even if no user threads make secure calls. */
                     portALLOCATE_SECURE_CONTEXT(0);
                    #endif

	/* Enter user code for this thread. Pass task handle. */
	camera_thread_entry(pvParameters);
}
