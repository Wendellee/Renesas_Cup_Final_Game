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

#if LV_USE_GUIDER_SIMULATOR && LV_USE_FREEMASTER
#include "freemaster_client.h"
#endif

bool run_stop = 0; // 0: run, 1: stop
#include "custom.h"

bool manual_auto = 0; // 0: manual, 1: automatic
#include "custom.h"

bool LED = 0; // 0: OFF, 1: OFF

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

        run_stop = !run_stop;
        lv_obj_set_style_bg_color(
            ui->main_btn_model,
            lv_color_hex(run_stop ? 0xFF0000 : 0x00FF00),
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
        lv_label_set_text(ui->main_btn_model_label, run_stop ? "STOP" : "RUN");
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
        lv_obj_t *button = (lv_obj_t *)lv_event_get_target(e);

        /* 每按一次，在手动和自动模式之间切换。 */
        manual_auto = !manual_auto;
        lv_obj_set_style_bg_color(
            button,
            lv_color_hex(manual_auto ? 0x1EC5FF : 0xA1A1A1),
            LV_PART_MAIN | LV_STATE_DEFAULT
        );
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
        break;
    }
    default:
        break;
    }
}

void events_init_main (lv_ui *ui)
{
    lv_obj_add_event_cb(ui->main, main_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_btn_model, main_btn_model_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_btn_model1, main_btn_model1_event_handler, LV_EVENT_ALL, ui);
    lv_obj_add_event_cb(ui->main_btn_1, main_btn_1_event_handler, LV_EVENT_ALL, ui);
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
