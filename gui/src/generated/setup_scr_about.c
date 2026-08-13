/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "events_init.h"
#include "widgets_init.h"
#include "custom.h"



void setup_scr_about(lv_ui *ui)
{
    //Write codes about
    ui->about = lv_obj_create(NULL);
    lv_obj_set_size(ui->about, 1024, 600);
    lv_obj_set_scrollbar_mode(ui->about, LV_SCROLLBAR_MODE_OFF);

    //Write style for about, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->about, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->about, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->about, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes about_qrcode_1
    ui->about_qrcode_1 = lv_qrcode_create(ui->about);
    lv_obj_set_pos(ui->about_qrcode_1, 781, 376);
    lv_obj_set_size(ui->about_qrcode_1, 150, 150);
    lv_qrcode_set_size(ui->about_qrcode_1, 150);
    lv_qrcode_set_dark_color(ui->about_qrcode_1, lv_color_hex(0x2C3224));
    lv_qrcode_set_light_color(ui->about_qrcode_1, lv_color_hex(0xffffff));
    const char * about_qrcode_1_data = "https://github.com/Wendellee/Renesas_Cup_Final_Game";
    lv_qrcode_update(ui->about_qrcode_1, about_qrcode_1_data, 51);

    //Write codes about_permit
    ui->about_permit = lv_label_create(ui->about);
    lv_obj_set_pos(ui->about_permit, 156, 460);
    lv_obj_set_size(ui->about_permit, 519, 25);
    lv_label_set_text(ui->about_permit, "Scan the QR code to get code on Github");
    lv_label_set_long_mode(ui->about_permit, LV_LABEL_LONG_WRAP);

    //Write style for about_permit, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->about_permit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->about_permit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->about_permit, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->about_permit, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->about_permit, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->about_permit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->about_permit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->about_permit, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->about_permit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->about_permit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->about_permit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->about_permit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->about_permit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->about_permit, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes about_webaddress
    ui->about_webaddress = lv_label_create(ui->about);
    lv_obj_set_pos(ui->about_webaddress, 112, 505);
    lv_obj_set_size(ui->about_webaddress, 646, 26);
    lv_label_set_text(ui->about_webaddress, "https://github.com/Wendellee/Renesas_Cup_Final_Game");
    lv_label_set_long_mode(ui->about_webaddress, LV_LABEL_LONG_WRAP);

    //Write style for about_webaddress, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->about_webaddress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->about_webaddress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->about_webaddress, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->about_webaddress, &lv_font_montserratMedium_20, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->about_webaddress, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->about_webaddress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->about_webaddress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->about_webaddress, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->about_webaddress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->about_webaddress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->about_webaddress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->about_webaddress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->about_webaddress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->about_webaddress, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes about_title
    ui->about_title = lv_label_create(ui->about);
    lv_obj_set_pos(ui->about_title, 432, 46);
    lv_obj_set_size(ui->about_title, 173, 32);
    lv_label_set_text(ui->about_title, "About us");
    lv_label_set_long_mode(ui->about_title, LV_LABEL_LONG_WRAP);

    //Write style for about_title, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->about_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->about_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->about_title, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->about_title, &lv_font_montserratMedium_27, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->about_title, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->about_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->about_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->about_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->about_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->about_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->about_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->about_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->about_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->about_title, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of about.


    //Update current screen layout.
    lv_obj_update_layout(ui->about);

    //Init events for screen.
    events_init_about(ui);
}
