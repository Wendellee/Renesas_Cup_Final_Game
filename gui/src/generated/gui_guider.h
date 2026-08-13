/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"


typedef struct
{
  
	lv_obj_t *main;
	bool main_del;
	lv_obj_t *main_btn_left;
	lv_obj_t *main_btn_left_label;
	lv_obj_t *main_btn_right;
	lv_obj_t *main_btn_right_label;
	lv_obj_t *main_btn_forward;
	lv_obj_t *main_btn_forward_label;
	lv_obj_t *main_btn_back;
	lv_obj_t *main_btn_back_label;
	lv_obj_t *main_btn_model;
	lv_obj_t *main_btn_model_label;
	lv_obj_t *main_speed;
	lv_obj_t *main_roller_speed;
	lv_obj_t *main_label_speed;
	lv_obj_t *main_log;
	lv_obj_t *main_label_run_stop_z;
	lv_obj_t *main_label_speed_z;
	lv_obj_t *main_label_model_z;
	lv_obj_t *main_label_model;
	lv_obj_t *main_label_run_stop;
	lv_obj_t *main_label_spee;
	lv_obj_t *main_label_fan_z;
	lv_obj_t *main_label_fan_mode;
	lv_obj_t *main_ddlist_wifi;
	lv_obj_t *main_label_wifi;
	lv_obj_t *main_btn_model1;
	lv_obj_t *main_btn_model1_label;
	lv_obj_t *main_btn_1;
	lv_obj_t *main_btn_1_label;
	lv_obj_t *main_btn_2;
	lv_obj_t *main_btn_2_label;
	lv_obj_t *main_img_1;
	lv_obj_t *about;
	bool about_del;
	lv_obj_t *about_qrcode_1;
	lv_obj_t *about_permit;
	lv_obj_t *about_webaddress;
	lv_obj_t *about_title;
}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void ui_init_style(lv_style_t * style);

void ui_load_scr_animation(lv_ui *ui, lv_obj_t ** new_scr, bool new_scr_del, bool * old_scr_del, ui_setup_scr_t setup_scr,
                           lv_screen_load_anim_t anim_type, uint32_t time, uint32_t delay, bool is_clean, bool auto_del);

void ui_animation(void * var, uint32_t duration, int32_t delay, int32_t start_value, int32_t end_value, lv_anim_path_cb_t path_cb,
                  uint32_t repeat_cnt, uint32_t repeat_delay, uint32_t playback_time, uint32_t playback_delay,
                  lv_anim_exec_xcb_t exec_cb, lv_anim_start_cb_t start_cb, lv_anim_completed_cb_t ready_cb, lv_anim_deleted_cb_t deleted_cb);


void init_scr_del_flag(lv_ui *ui);

void setup_bottom_layer(void);

void setup_ui(lv_ui *ui);

void video_play(lv_ui *ui);

void init_keyboard(lv_ui *ui);

extern lv_ui guider_ui;


void setup_scr_main(lv_ui *ui);
void setup_scr_about(lv_ui *ui);
LV_IMAGE_DECLARE(_45_RGB565A8_480x272);

LV_FONT_DECLARE(lv_font_montserratMedium_50)
LV_FONT_DECLARE(lv_font_montserratMedium_25)
LV_FONT_DECLARE(lv_font_arial_19)
LV_FONT_DECLARE(lv_font_arial_18)
LV_FONT_DECLARE(lv_font_montserratMedium_18)
LV_FONT_DECLARE(lv_font_montserratMedium_20)
LV_FONT_DECLARE(lv_font_montserratMedium_12)
LV_FONT_DECLARE(lv_font_montserratMedium_16)
LV_FONT_DECLARE(lv_font_montserratMedium_27)


#ifdef __cplusplus
}
#endif
#endif
