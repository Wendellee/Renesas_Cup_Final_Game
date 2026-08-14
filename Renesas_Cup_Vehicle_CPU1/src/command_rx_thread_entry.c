#include "command_rx_thread.h"
#include "app_runtime.h"
#include "Radio/application/command_radio.h"
#include "Radio/platform/fsp_nrf24_port.h"
#include "SEGGER_RTT/bsp_print.h"

#define COMMAND_IRQ_FALLBACK_MS (20U)
#define COMMAND_INIT_RETRY_MS   (5000U)

/* Command RX Thread entry function */
/* pvParameters contains TaskHandle_t */
void command_rx_thread_entry(void *pvParameters) {
	FSP_PARAMETER_NOT_USED(pvParameters);

	if (!app_runtime_init()) {
		g_printf("[SYSTEM][FATAL] Command RX runtime initialization failed.\r\n");
		vTaskSuspend(NULL);
	}

	app_runtime_wait_for_start();

	nrf24_result_t result;
	do {
		result = CommandRadio_Init();
		if (NRF24_RESULT_SUCCESS != result) {
			g_printf("[CMD NRF] init failed=%u; retry in %u ms\r\n",
			         (uint32_t) result,
			         (uint32_t) COMMAND_INIT_RETRY_MS);
			vTaskDelay(pdMS_TO_TICKS(COMMAND_INIT_RETRY_MS));
		}
	} while (NRF24_RESULT_SUCCESS != result);

	g_printf("[CMD NRF] ready SPI0 ch=76 IRQ=P705/IRQ19\r\n");
	while (1) {
		/* IRQ负责低延迟唤醒；20ms超时轮询用于接线/边沿丢失时的安全兜底。 */
		(void) FspNrf24Port_CommandIrqWait(COMMAND_IRQ_FALLBACK_MS);

		uint32_t processed = 0U;
		result = CommandRadio_Service(&processed);
		if ((NRF24_RESULT_SUCCESS != result) && (NRF24_RESULT_NO_DATA != result)) {
			g_printf("[CMD NRF] service failed=%u\r\n", (uint32_t) result);
			vTaskDelay(pdMS_TO_TICKS(10U));
		}
	}
}
