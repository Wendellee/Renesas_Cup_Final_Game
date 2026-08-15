/* generated configuration header file - do not edit */
#ifndef BSP_PIN_CFG_H_
#define BSP_PIN_CFG_H_
#include "r_ioport.h"

/* Common macro for FSP header files. There is also a corresponding FSP_FOOTER macro at the end of this file. */
FSP_HEADER

#define CAMERA_RESET (BSP_IO_PORT_00_PIN_12)
#define COMMAND_TX_CE (BSP_IO_PORT_01_PIN_04)
#define user_led (BSP_IO_PORT_01_PIN_10)
#define TCH_INT (BSP_IO_PORT_01_PIN_11)
#define LCD_BL (BSP_IO_PORT_05_PIN_13)
#define TCH_RST (BSP_IO_PORT_06_PIN_06)
#define SPI0_MISO (BSP_IO_PORT_07_PIN_00)
#define SPI0_MOSI (BSP_IO_PORT_07_PIN_01)
#define SPI0_CLK (BSP_IO_PORT_07_PIN_02)
#define SPI0_CSN (BSP_IO_PORT_07_PIN_03)
#define VIDEO_RX_CE (BSP_IO_PORT_07_PIN_04)
#define VIDEO_RX_IRQ (BSP_IO_PORT_07_PIN_05)

extern const ioport_cfg_t g_bsp_pin_cfg; /* R7KA8P1KFLCAC.pincfg */

void BSP_PinConfigSecurityInit();

/* Common macro for FSP header files. There is also a corresponding FSP_HEADER macro at the top of this file. */
FSP_FOOTER
#endif /* BSP_PIN_CFG_H_ */
