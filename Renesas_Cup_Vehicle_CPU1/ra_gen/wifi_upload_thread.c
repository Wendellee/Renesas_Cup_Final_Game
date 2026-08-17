/* generated thread source file - do not edit */
#include "wifi_upload_thread.h"

#if 1
static StaticTask_t wifi_upload_thread_memory;
#if defined(__ARMCC_VERSION)           /* AC6 compiler */
                static uint8_t wifi_upload_thread_stack[6144] BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".stack.thread") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
                #else
static uint8_t wifi_upload_thread_stack[6144] BSP_PLACE_IN_SECTION(BSP_UNINIT_SECTION_PREFIX ".stack.wifi_upload_thread") BSP_ALIGN_VARIABLE(BSP_STACK_ALIGNMENT);
#endif
#endif
TaskHandle_t wifi_upload_thread;
void wifi_upload_thread_create(void);
static void wifi_upload_thread_func(void *pvParameters);
void rtos_startup_err_callback(void *p_instance, void *p_data);
void rtos_startup_common_init(void);
sci_b_uart_instance_ctrl_t g_uart0_ctrl;

sci_b_baud_setting_t g_uart0_baud_setting = {
/* Baud rate calculated with 0.160% error. */.baudrate_bits_b.abcse = 0,
		.baudrate_bits_b.abcs = 0, .baudrate_bits_b.bgdm = 1,
		.baudrate_bits_b.cks = 0, .baudrate_bits_b.brr = 64,
		.baudrate_bits_b.mddr = (uint8_t) 256, .baudrate_bits_b.brme = false };

/** UART extended configuration for UARTonSCI HAL driver */
const sci_b_uart_extended_cfg_t g_uart0_cfg_extend = { .clock =
		SCI_B_UART_CLOCK_INT,
		.rx_edge_start = SCI_B_UART_START_BIT_FALLING_EDGE, .noise_cancel =
				SCI_B_UART_NOISE_CANCELLATION_DISABLE, .rx_fifo_trigger =
				SCI_B_UART_RX_FIFO_TRIGGER_MAX, .p_baud_setting =
				&g_uart0_baud_setting, .flow_control =
				SCI_B_UART_FLOW_CONTROL_RTS,
#if 0xFF != 0xFF
                .flow_control_pin       = BSP_IO_PORT_FF_PIN_0xFF,
                #else
		.flow_control_pin = (bsp_io_port_pin_t) UINT16_MAX,
#endif
		.rs485_setting = { .enable = SCI_B_UART_RS485_DISABLE, .polarity =
				SCI_B_UART_RS485_DE_POLARITY_HIGH, .assertion_time = 1,
				.negation_time = 1, }, .delay_cycles = 0, };

/** UART interface configuration */
const uart_cfg_t g_uart0_cfg = { .channel = 0, .data_bits = UART_DATA_BITS_8,
		.parity = UART_PARITY_OFF, .stop_bits = UART_STOP_BITS_1, .p_callback =
				UART0_CallBack, .p_context = NULL, .p_extend =
				&g_uart0_cfg_extend,
#define RA_NOT_DEFINED (1)
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
		.p_transfer_tx = NULL,
#else
                .p_transfer_tx       = &RA_NOT_DEFINED,
#endif
#if (RA_NOT_DEFINED == RA_NOT_DEFINED)
		.p_transfer_rx = NULL,
#else
                .p_transfer_rx       = &RA_NOT_DEFINED,
#endif
#undef RA_NOT_DEFINED
		.rxi_ipl = (12), .txi_ipl = (12), .tei_ipl = (12), .eri_ipl = (12),
#if defined(VECTOR_NUMBER_SCI0_RXI)
                .rxi_irq             = VECTOR_NUMBER_SCI0_RXI,
#else
		.rxi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI0_TXI)
                .txi_irq             = VECTOR_NUMBER_SCI0_TXI,
#else
		.txi_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI0_TEI)
                .tei_irq             = VECTOR_NUMBER_SCI0_TEI,
#else
		.tei_irq = FSP_INVALID_VECTOR,
#endif
#if defined(VECTOR_NUMBER_SCI0_ERI)
                .eri_irq             = VECTOR_NUMBER_SCI0_ERI,
#else
		.eri_irq = FSP_INVALID_VECTOR,
#endif
		};

/* Instance structure to use this module. */
const uart_instance_t g_uart0 = { .p_ctrl = &g_uart0_ctrl,
		.p_cfg = &g_uart0_cfg, .p_api = &g_uart_on_sci_b };
extern uint32_t g_fsp_common_thread_count;

const rm_freertos_port_parameters_t wifi_upload_thread_parameters = {
		.p_context = (void*) NULL, };

void wifi_upload_thread_create(void) {
	/* Increment count so we will know the number of threads created in the RA Configuration editor. */
	g_fsp_common_thread_count++;

	/* Initialize each kernel object. */

#if 1
	wifi_upload_thread = xTaskCreateStatic(
#else
                    BaseType_t wifi_upload_thread_create_err = xTaskCreate(
                    #endif
			wifi_upload_thread_func, (const char*) "WIFI Upload Thread",
			6144 / 4, // In words, not bytes
			(void*) &wifi_upload_thread_parameters, //pvParameters
			3,
#if 1
			(StackType_t*) &wifi_upload_thread_stack,
			(StaticTask_t*) &wifi_upload_thread_memory
#else
                        & wifi_upload_thread
                        #endif
			);

#if 1
	if (NULL == wifi_upload_thread) {
		rtos_startup_err_callback(wifi_upload_thread, 0);
	}
#else
                    if (pdPASS != wifi_upload_thread_create_err)
                    {
                        rtos_startup_err_callback(wifi_upload_thread, 0);
                    }
                    #endif
}
static void wifi_upload_thread_func(void *pvParameters) {
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
	wifi_upload_thread_entry(pvParameters);
}
