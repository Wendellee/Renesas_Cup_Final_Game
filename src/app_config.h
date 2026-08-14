#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

/* Production remote-controller topology:
 *
 *   LVGL -> Command TX -> nRF24 channel 76  -> vehicle
 *   LVGL <- Video RX   <- nRF24 channel 100 <- vehicle
 *
 * Keep the former on-board camera and dual-radio video loopback source in the
 * repository for diagnostics, but remove it from the production build path.
 */
#define APP_VIDEO_RX_ENABLE                 (1U)
#define APP_CAMERA_CAPTURE_ENABLE           (0U)
#define APP_LOCAL_VIDEO_LOOPBACK_ENABLE     (0U)
/* Historical Video RX control-packet parser is retained for diagnostics only. */
#define APP_VIDEO_RX_CONTROL_COMPAT_ENABLE  (0U)

#if APP_LOCAL_VIDEO_LOOPBACK_ENABLE && !APP_CAMERA_CAPTURE_ENABLE
#error "Local video loopback requires APP_CAMERA_CAPTURE_ENABLE"
#endif

#endif /* APP_CONFIG_H_ */
