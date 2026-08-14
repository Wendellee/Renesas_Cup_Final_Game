#include "camera_thread.h"
/* Camera Thread entry function */
/* pvParameters contains TaskHandle_t */
void camera_thread_entry(void *pvParameters) {
	FSP_PARAMETER_NOT_USED(pvParameters);

	/* TODO: add your own code here */
	while (1) {
		vTaskDelay(1);
	}
}
