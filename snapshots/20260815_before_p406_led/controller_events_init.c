/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "events_init.h"
#include <stdio.h>
#include "lvgl.h"
#include "common_utils.h"
#include "nrf24/wireless_touch_tx.h"

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif

#include "custom.h"

bool run_stop = 0;   // 0: stop, 1: run
bool manual_auto = 0; // 0: manual, 1: automatic
bool LED = 0;         // 0: off, 1: on
bool fan = 0;         // 0: stop, 1: run

static void wireless_touch_send(wireless_touch_control_t control,
                                wireless_touch_action_t action,
                                uint16_t value)
{
    nrf24_result_t result = WirelessTouchTx_Send(control, action, value);

    if (NRF24_RESULT_SUCCESS != result) {
        APP_PRINT("[NRF24] send failed control=%u action=%u value=%u result=%u\r\n",
                  (uint32_t) control,
                  (uint32_t) action,
                  (uint32_t) value,
                  (uint32_t) result);
    }
}

static wireless_touch_direction_t direction_value(lv_ui const * ui, lv_obj_t const * button)
{
    if (button == ui->main_btn_left) {
        return WIRELESS_TOUCH_DIRECTION_LEFT;
    }
    if (button == ui->main_btn_right) {
        return WIRELESS_TOUCH_DIRECTION_RIGHT;
    }
    if (button == ui->main_btn_forward) {
        return WIRELESS_TOUCH_DIRECTION_FORWARD;
    }
    if (button == ui->main_btn_back) {
        return WIRELESS_TOUCH_DIRECTION_BACK;
    }

    return WIRELESS_TOUCH_DIRECTION_STOP;
}

static const char * direction_name(lv_ui const * ui, lv_obj_t const * button)
{
    if (button == ui->main_btn_left) {
        return "left";
    }
    if (button == ui->main_btn_right) {
        return "right";
    }
    if (button == ui->main_btn_forward) {
        return "forward";
    }
    if (button == ui->main_btn_back) {
        return "back";
    }

    return "unknown";
}

static void main_direction_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_ui * ui = (lv_ui *) lv_event_get_user_data(e);
    lv_obj_t * button = (lv_obj_t *) lv_event_get_target(e);

    if (LV_EVENT_PRESSED == code) {
        APP_PRINT("[LVGL] control=direction value=%s state=pressed\r\n", direction_name(ui, button));
        wireless_touch_send(WIRELESS_TOUCH_CONTROL_DIRECTION,
                            WIRELESS_TOUCH_ACTION_PRESSED,
                            (uint16_t) direction_value(ui, button));
    }
    else if ((LV_EVENT_RELEASED == code) || (LV_EVENT_PRESS_LOST == code)) {
        APP_PRINT("[LVGL] control=direction value=%s state=released\r\n", direction_name(ui, button));
        wireless_touch_send(WIRELESS_TOUCH_CONTROL_DIRECTION,
                            WIRELESS_TOUCH_ACTION_RELEASED,
                            (uint16_t) direction_value(ui, button));
    }
}

static void main_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        switch(dir) {
        case LV_DIR_BOTTOM:
        {
            lv_indev_wait_release(lv_indev_active());
            APP_PRINT("[LVGL] control=page value=about\r\n");
            wireless_touch_send(WIRELESS_TOUCH_CONTROL_PAGE,
                                WIRELESS_TOUCH_ACTION_CHANGED,
                                1U);
            PageToAbout();
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

static void main_btn_model_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);

        if(manual_auto) {
            break;
        }
        run_stop = !run_stop;
        lv_obj_set_style_bg_color(
            ui->main_btn_model,
            lv_color_hex(run_stop ? 0xFF0000 : 0x00FF00),
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
        lv_label_set_text(ui->main_btn_model_label, run_stop ? "STOP" : "RUN");
        lv_label_set_text(ui->main_label_run_stop_z, run_stop ? "Run" : "Stop");
        APP_PRINT("[LVGL] control=run_stop value=%s\r\n", run_stop ? "run" : "stop");
        wireless_touch_send(WIRELESS_TOUCH_CONTROL_RUN_STOP,
                            WIRELESS_TOUCH_ACTION_CHANGED,
                            run_stop ? 1U : 0U);
        break;
    }
    default:
        break;
    }
}

static void main_roller_speed_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_VALUE_CHANGED:
    {
        lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
        char selected_speed[16];

        lv_roller_get_selected_str(ui->main_roller_speed, selected_speed, sizeof(selected_speed));
        lv_label_set_text(ui->main_label_speed_z, selected_speed);
        APP_PRINT("[LVGL] control=speed index=%u value=%s\r\n",
                  (uint32_t) lv_roller_get_selected(ui->main_roller_speed),
                  selected_speed);
        wireless_touch_send(WIRELESS_TOUCH_CONTROL_SPEED,
                            WIRELESS_TOUCH_ACTION_CHANGED,
                            (uint16_t) lv_roller_get_selected(ui->main_roller_speed));
        break;
    }
    default:
        break;
    }
}

static void main_btn_model1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
        lv_obj_t *button = (lv_obj_t *)lv_event_get_target(e);

        manual_auto = !manual_auto;
        lv_obj_set_style_bg_color(
            button,
            lv_color_hex(manual_auto ? 0x1EC5FF : 0xA1A1A1),
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
        lv_label_set_text(ui->main_label_model_z, manual_auto ? "automatic" : "manual");
        lv_label_set_text(ui->main_label_run_stop_z, (manual_auto || run_stop) ? "Run" : "Stop");
        APP_PRINT("[LVGL] control=mode value=%s\r\n", manual_auto ? "automatic" : "manual");
        wireless_touch_send(WIRELESS_TOUCH_CONTROL_MODE,
                            WIRELESS_TOUCH_ACTION_CHANGED,
                            manual_auto ? 1U : 0U);
        break;
    }
    default:
        break;
    }
}

static void main_btn_1_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_obj_t *button = (lv_obj_t *)lv_event_get_target(e);

        /* 每按一次，在手动和自动模式之间切换。 */
        LED = !LED;
        lv_obj_set_style_bg_color(
            button,
            lv_color_hex(LED ? 0x1EC5FF : 0xA1A1A1),
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
        APP_PRINT("[LVGL] control=led value=%s\r\n", LED ? "on" : "off");
        wireless_touch_send(WIRELESS_TOUCH_CONTROL_LED,
                            WIRELESS_TOUCH_ACTION_CHANGED,
                            LED ? 1U : 0U);
        break;
    }
    default:
        break;
    }
}

static void main_btn_2_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_CLICKED:
    {
        lv_ui *ui = (lv_ui *)lv_event_get_user_data(e);
        lv_obj_t *button = (lv_obj_t *)lv_event_get_target(e);

        /* 每按一次，在手动和自动模式之间切换。 */
        fan = !fan;
        lv_obj_set_style_bg_color(
            button,
            lv_color_hex(fan ? 0x1EC5FF : 0xA1A1A1),
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
        lv_label_set_text(ui->main_label_fan_z, fan ? "Run" : "Stop");
        APP_PRINT("[LVGL] control=fan value=%s\r\n", fan ? "run" : "stop");
        wireless_touch_send(WIRELESS_TOUCH_CONTROL_FAN,
                            WIRELESS_TOUCH_ACTION_CHANGED,
                            fan ? 1U : 0U);
        break;
    }
    default:
        break;
    }
}

static void main_ddlist_wifi_event_handler(lv_event_t * e)
{
    if (LV_EVENT_VALUE_CHANGED == lv_event_get_code(e)) {
        lv_ui * ui = (lv_ui *) lv_event_get_user_data(e);
        char selected_wifi[32];

        lv_dropdown_get_selected_str(ui->main_ddlist_wifi, selected_wifi, sizeof(selected_wifi));
        APP_PRINT("[LVGL] control=wifi index=%u value=%s\r\n",
                  (uint32_t) lv_dropdown_get_selected(ui->main_ddlist_wifi),
                  selected_wifi);
        wireless_touch_send(WIRELESS_TOUCH_CONTROL_WIFI,
                            WIRELESS_TOUCH_ACTION_CHANGED,
                            (uint16_t) lv_dropdown_get_selected(ui->main_ddlist_wifi));
    }
}

void events_init_main (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->main, main_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_btn_left, main_direction_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_btn_right, main_direction_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_btn_forward, main_direction_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_btn_back, main_direction_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_btn_model, main_btn_model_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_roller_speed, main_roller_speed_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_ddlist_wifi, main_ddlist_wifi_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_btn_model1, main_btn_model1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_btn_1, main_btn_1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_btn_2, main_btn_2_event_handler, LV_EVENT_ALL, ui);
}

static void about_event_handler (lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code) {
    case LV_EVENT_GESTURE:
    {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
        switch(dir) {
        case LV_DIR_TOP:
        {
            lv_indev_wait_release(lv_indev_active());
            APP_PRINT("[LVGL] control=page value=main\r\n");
            wireless_touch_send(WIRELESS_TOUCH_CONTROL_PAGE,
                                WIRELESS_TOUCH_ACTION_CHANGED,
                                0U);
            PageToMain();

            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

void events_init_about (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->about, about_event_handler, LV_EVENT_ALL, ui);
}


void events_init(lv_ui *ui)
{

}
