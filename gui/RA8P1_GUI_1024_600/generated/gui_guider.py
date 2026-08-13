# Copyright 2026 NXP
# NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
# accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
# activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
# comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
# terms, then you may not retain, install, activate or otherwise use the software.

import utime as time
import usys as sys
import lvgl as lv
import ustruct
import fs_driver

lv.init()

# Register display driver.
disp_drv = lv.sdl_window_create(1024, 600)
lv.sdl_window_set_resizeable(disp_drv, False)
lv.sdl_window_set_title(disp_drv, "Simulator (MicroPython)")

# Regsiter input driver
mouse = lv.sdl_mouse_create()

# Add default theme for bottom layer
bottom_layer = lv.layer_bottom()
lv.theme_apply(bottom_layer)

fs_drv = lv.fs_drv_t()
fs_driver.fs_register(fs_drv, 'Z')

def anim_x_cb(obj, v):
    obj.set_x(v)

def anim_y_cb(obj, v):
    obj.set_y(v)

def anim_width_cb(obj, v):
    obj.set_width(v)

def anim_height_cb(obj, v):
    obj.set_height(v)

def anim_img_zoom_cb(obj, v):
    obj.set_scale(v)

def anim_img_rotate_cb(obj, v):
    obj.set_rotation(v)

global_font_cache = {}
def test_font(font_family, font_size):
    global global_font_cache
    if font_family + str(font_size) in global_font_cache:
        return global_font_cache[font_family + str(font_size)]
    if font_size % 2:
        candidates = [
            (font_family, font_size),
            (font_family, font_size-font_size%2),
            (font_family, font_size+font_size%2),
            ("montserrat", font_size-font_size%2),
            ("montserrat", font_size+font_size%2),
            ("montserrat", 16)
        ]
    else:
        candidates = [
            (font_family, font_size),
            ("montserrat", font_size),
            ("montserrat", 16)
        ]
    for (family, size) in candidates:
        try:
            if eval(f'lv.font_{family}_{size}'):
                global_font_cache[font_family + str(font_size)] = eval(f'lv.font_{family}_{size}')
                if family != font_family or size != font_size:
                    print(f'WARNING: lv.font_{family}_{size} is used!')
                return eval(f'lv.font_{family}_{size}')
        except AttributeError:
            try:
                load_font = lv.binfont_create(f"Z:MicroPython/lv_font_{family}_{size}.fnt")
                global_font_cache[font_family + str(font_size)] = load_font
                return load_font
            except:
                if family == font_family and size == font_size:
                    print(f'WARNING: lv.font_{family}_{size} is NOT supported!')

global_image_cache = {}
def load_image(file):
    global global_image_cache
    if file in global_image_cache:
        return global_image_cache[file]
    try:
        with open(file,'rb') as f:
            data = f.read()
    except:
        print(f'Could not open {file}')
        sys.exit()

    img = lv.image_dsc_t({
        'data_size': len(data),
        'data': data
    })
    global_image_cache[file] = img
    return img

def calendar_event_handler(e,obj):
    code = e.get_code()

    if code == lv.EVENT.VALUE_CHANGED:
        source = lv.calendar.__cast__(e.get_current_target())
        date = lv.calendar_date_t()
        if source.get_pressed_date(date) == lv.RESULT.OK:
            source.set_highlighted_dates([date], 1)

def spinbox_increment_event_cb(e, obj):
    code = e.get_code()
    if code == lv.EVENT.SHORT_CLICKED or code == lv.EVENT.LONG_PRESSED_REPEAT:
        obj.increment()
def spinbox_decrement_event_cb(e, obj):
    code = e.get_code()
    if code == lv.EVENT.SHORT_CLICKED or code == lv.EVENT.LONG_PRESSED_REPEAT:
        obj.decrement()

def digital_clock_cb(timer, obj, current_time, show_second, use_ampm):
    hour = int(current_time[0])
    minute = int(current_time[1])
    second = int(current_time[2])
    ampm = current_time[3]
    second = second + 1
    if second == 60:
        second = 0
        minute = minute + 1
        if minute == 60:
            minute = 0
            hour = hour + 1
            if use_ampm:
                if hour == 12:
                    if ampm == 'AM':
                        ampm = 'PM'
                    elif ampm == 'PM':
                        ampm = 'AM'
                if hour > 12:
                    hour = hour % 12
    hour = hour % 24
    if use_ampm:
        if show_second:
            obj.set_text("%d:%02d:%02d %s" %(hour, minute, second, ampm))
        else:
            obj.set_text("%d:%02d %s" %(hour, minute, ampm))
    else:
        if show_second:
            obj.set_text("%d:%02d:%02d" %(hour, minute, second))
        else:
            obj.set_text("%d:%02d" %(hour, minute))
    current_time[0] = hour
    current_time[1] = minute
    current_time[2] = second
    current_time[3] = ampm

def analog_clock_cb(timer, obj):
    datetime = time.localtime()
    hour = datetime[3]
    if hour >= 12: hour = hour - 12
    obj.set_time(hour, datetime[4], datetime[5])

def datetext_event_handler(e, obj):
    code = e.get_code()
    datetext = lv.label.__cast__(e.get_target())
    if code == lv.EVENT.FOCUSED:
        if obj is None:
            bg = lv.layer_top()
            bg.add_flag(lv.obj.FLAG.CLICKABLE)
            obj = lv.calendar(bg)
            scr = lv.screen_active()
            scr_height = scr.get_height()
            scr_width = scr.get_width()
            obj.set_size(int(scr_width * 0.8), int(scr_height * 0.8))
            datestring = datetext.get_text()
            year = int(datestring.split('/')[0])
            month = int(datestring.split('/')[1])
            day = int(datestring.split('/')[2])
            obj.set_showed_date(year, month)
            highlighted_days=[lv.calendar_date_t({'year':year, 'month':month, 'day':day})]
            obj.set_highlighted_dates(highlighted_days, 1)
            obj.align(lv.ALIGN.CENTER, 0, 0)
            lv.calendar_header_arrow(obj)
            obj.add_event_cb(lambda e: datetext_calendar_event_handler(e, datetext), lv.EVENT.ALL, None)
            scr.update_layout()

def datetext_calendar_event_handler(e, obj):
    code = e.get_code()
    calendar = lv.calendar.__cast__(e.get_current_target())
    if code == lv.EVENT.VALUE_CHANGED:
        date = lv.calendar_date_t()
        if calendar.get_pressed_date(date) == lv.RESULT.OK:
            obj.set_text(f"{date.year}/{date.month}/{date.day}")
            bg = lv.layer_top()
            bg.remove_flag(lv.obj.FLAG.CLICKABLE)
            bg.set_style_bg_opa(lv.OPA.TRANSP, 0)
            calendar.delete()

# Create main
main = lv.obj()
main.set_size(1024, 600)
main.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for main, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
main.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main.set_style_bg_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
main.set_style_bg_grad_dir(lv.GRAD_DIR.NONE, lv.PART.MAIN|lv.STATE.DEFAULT)
main.set_style_bg_image_src(load_image(r"D:\ra8p1\RA8P1_GUI_1024_600\generated\MicroPython\beijing_1024_600.png"), lv.PART.MAIN|lv.STATE.DEFAULT)
main.set_style_bg_image_opa(150, lv.PART.MAIN|lv.STATE.DEFAULT)
main.set_style_bg_image_recolor_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create main_animimg_1
main_animimg_1 = lv.animimg(main)
main_animimg_1_imgs = [None]*1
main_animimg_1_imgs[0] = load_image(r"D:\ra8p1\RA8P1_GUI_1024_600\generated\MicroPython\abc_480_272.png")
main_animimg_1.set_src(main_animimg_1_imgs, 1, False)
main_animimg_1.set_duration(30*1)
main_animimg_1.set_repeat_count(lv.ANIM_REPEAT_INFINITE)
lv.image.__cast__(main_animimg_1).set_src(main_animimg_1_imgs[0])
main_animimg_1.set_pos(61, 79)
main_animimg_1.set_size(480, 272)

# Create main_btn_1
main_btn_1 = lv.button(main)
main_btn_1_label = lv.label(main_btn_1)
main_btn_1_label.set_text(""+lv.SYMBOL.LEFT+"")
main_btn_1_label.set_long_mode(lv.label.LONG.WRAP)
main_btn_1_label.set_width(lv.pct(100))
main_btn_1_label.align(lv.ALIGN.CENTER, 0, 0)
main_btn_1.set_style_pad_all(0, lv.STATE.DEFAULT)
main_btn_1.set_pos(602, 161)
main_btn_1.set_size(100, 100)
# Set style for main_btn_1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
main_btn_1.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_1.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_1.set_style_bg_grad_dir(lv.GRAD_DIR.NONE, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_1.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_1.set_style_radius(5, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_1.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_1.set_style_text_font(test_font("montserratMedium", 50), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_1.set_style_text_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_1.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create main_btn_2
main_btn_2 = lv.button(main)
main_btn_2_label = lv.label(main_btn_2)
main_btn_2_label.set_text(""+lv.SYMBOL.RIGHT+"")
main_btn_2_label.set_long_mode(lv.label.LONG.WRAP)
main_btn_2_label.set_width(lv.pct(100))
main_btn_2_label.align(lv.ALIGN.CENTER, 0, 0)
main_btn_2.set_style_pad_all(0, lv.STATE.DEFAULT)
main_btn_2.set_pos(829, 160)
main_btn_2.set_size(100, 100)
# Set style for main_btn_2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
main_btn_2.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_2.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_2.set_style_bg_grad_dir(lv.GRAD_DIR.NONE, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_2.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_2.set_style_radius(5, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_2.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_2.set_style_text_font(test_font("montserratMedium", 50), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_2.set_style_text_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_2.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create main_btn_3
main_btn_3 = lv.button(main)
main_btn_3_label = lv.label(main_btn_3)
main_btn_3_label.set_text(""+lv.SYMBOL.UP+"")
main_btn_3_label.set_long_mode(lv.label.LONG.WRAP)
main_btn_3_label.set_width(lv.pct(100))
main_btn_3_label.align(lv.ALIGN.CENTER, 0, 0)
main_btn_3.set_style_pad_all(0, lv.STATE.DEFAULT)
main_btn_3.set_pos(715, 48)
main_btn_3.set_size(100, 100)
# Set style for main_btn_3, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
main_btn_3.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_3.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_3.set_style_bg_grad_dir(lv.GRAD_DIR.NONE, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_3.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_3.set_style_radius(5, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_3.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_3.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_3.set_style_text_font(test_font("montserratMedium", 50), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_3.set_style_text_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_3.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create main_btn_4
main_btn_4 = lv.button(main)
main_btn_4_label = lv.label(main_btn_4)
main_btn_4_label.set_text(""+lv.SYMBOL.DOWN+"")
main_btn_4_label.set_long_mode(lv.label.LONG.WRAP)
main_btn_4_label.set_width(lv.pct(100))
main_btn_4_label.align(lv.ALIGN.CENTER, 0, 0)
main_btn_4.set_style_pad_all(0, lv.STATE.DEFAULT)
main_btn_4.set_pos(715, 276)
main_btn_4.set_size(100, 100)
# Set style for main_btn_4, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
main_btn_4.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_4.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_4.set_style_bg_grad_dir(lv.GRAD_DIR.NONE, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_4.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_4.set_style_radius(5, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_4.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_4.set_style_text_color(lv.color_hex(0x000000), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_4.set_style_text_font(test_font("montserratMedium", 50), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_4.set_style_text_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_4.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create main_btn_5
main_btn_5 = lv.button(main)
main_btn_5_label = lv.label(main_btn_5)
main_btn_5_label.set_text("stop")
main_btn_5_label.set_long_mode(lv.label.LONG.WRAP)
main_btn_5_label.set_width(lv.pct(100))
main_btn_5_label.align(lv.ALIGN.CENTER, 0, 0)
main_btn_5.set_style_pad_all(0, lv.STATE.DEFAULT)
main_btn_5.set_pos(715, 162)
main_btn_5.set_size(100, 100)
# Set style for main_btn_5, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
main_btn_5.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_bg_color(lv.color_hex(0xff0027), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_bg_grad_dir(lv.GRAD_DIR.NONE, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_radius(25, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_shadow_width(3, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_shadow_color(lv.color_hex(0x0d4b3b), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_shadow_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_shadow_spread(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_shadow_offset_x(1, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_shadow_offset_y(2, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_text_font(test_font("montserratMedium", 18), lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_text_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_btn_5.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create main_cont_1
main_cont_1 = lv.obj(main)
main_cont_1.set_pos(61, 403)
main_cont_1.set_size(140, 173)
main_cont_1.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for main_cont_1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
main_cont_1.set_style_border_width(2, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_1.set_style_border_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_1.set_style_border_color(lv.color_hex(0x2195f6), lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_1.set_style_border_side(lv.BORDER_SIDE.NONE, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_1.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_1.set_style_bg_opa(68, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_1.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_1.set_style_bg_grad_dir(lv.GRAD_DIR.NONE, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_1.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_1.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_1.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_1.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
# Create main_roller_1
main_roller_1 = lv.roller(main_cont_1)
main_roller_1.set_options("25%\n50%\n75%\n100%", lv.roller.MODE.NORMAL)
main_roller_1.set_pos(18, 39)
main_roller_1.set_width(100)
# Set style for main_roller_1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
main_roller_1.set_style_radius(5, lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_bg_grad_dir(lv.GRAD_DIR.NONE, lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_text_color(lv.color_hex(0x333333), lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_text_font(test_font("montserratMedium", 12), lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_text_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_border_width(2, lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_border_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_border_color(lv.color_hex(0xe6e6e6), lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_border_side(lv.BORDER_SIDE.FULL, lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_roller_1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Set style for main_roller_1, Part: lv.PART.SELECTED, State: lv.STATE.DEFAULT.
main_roller_1.set_style_bg_opa(255, lv.PART.SELECTED|lv.STATE.DEFAULT)
main_roller_1.set_style_bg_color(lv.color_hex(0x2195f6), lv.PART.SELECTED|lv.STATE.DEFAULT)
main_roller_1.set_style_bg_grad_dir(lv.GRAD_DIR.NONE, lv.PART.SELECTED|lv.STATE.DEFAULT)
main_roller_1.set_style_text_color(lv.color_hex(0xFFFFFF), lv.PART.SELECTED|lv.STATE.DEFAULT)
main_roller_1.set_style_text_font(test_font("montserratMedium", 12), lv.PART.SELECTED|lv.STATE.DEFAULT)
main_roller_1.set_style_text_opa(255, lv.PART.SELECTED|lv.STATE.DEFAULT)

main_roller_1.set_visible_row_count(4)

# Create main_label_1
main_label_1 = lv.label(main_cont_1)
main_label_1.set_text("speed")
main_label_1.set_long_mode(lv.label.LONG.WRAP)
main_label_1.set_width(lv.pct(100))
main_label_1.set_pos(17, 12)
main_label_1.set_size(100, 32)
# Set style for main_label_1, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
main_label_1.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_text_font(test_font("montserratMedium", 18), lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_text_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_text_letter_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_label_1.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create main_cont_2
main_cont_2 = lv.obj(main)
main_cont_2.set_pos(240, 403)
main_cont_2.set_size(709, 173)
main_cont_2.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for main_cont_2, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
main_cont_2.set_style_border_width(2, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_2.set_style_border_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_2.set_style_border_color(lv.color_hex(0x2195f6), lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_2.set_style_border_side(lv.BORDER_SIDE.NONE, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_2.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_2.set_style_bg_opa(75, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_2.set_style_bg_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_2.set_style_bg_grad_dir(lv.GRAD_DIR.NONE, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_2.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_2.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_2.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_2.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
main_cont_2.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

main.update_layout()
# Create about
about = lv.obj()
about.set_size(1024, 600)
about.set_scrollbar_mode(lv.SCROLLBAR_MODE.OFF)
# Set style for about, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
about.set_style_bg_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
about.set_style_bg_color(lv.color_hex(0x08283d), lv.PART.MAIN|lv.STATE.DEFAULT)
about.set_style_bg_grad_dir(lv.GRAD_DIR.NONE, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create about_qrcode_1
about_qrcode_1 = lv.qrcode(about)
about_qrcode_1.set_size(150)
about_qrcode_1.set_dark_color(lv.color_hex(0x2C3224))
about_qrcode_1.set_light_color(lv.color_hex(0xffffff))
about_qrcode_1_data = "https://github.com/Wendellee/Renesas_Cup_Final_Game"
about_qrcode_1.update(about_qrcode_1_data, len(about_qrcode_1_data))
about_qrcode_1.set_pos(781, 376)

# Create about_permit
about_permit = lv.label(about)
about_permit.set_text("Scan the QR code to get code on Github")
about_permit.set_long_mode(lv.label.LONG.WRAP)
about_permit.set_width(lv.pct(100))
about_permit.set_pos(156, 460)
about_permit.set_size(519, 25)
# Set style for about_permit, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
about_permit.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_text_font(test_font("montserratMedium", 20), lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_text_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_text_letter_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_permit.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create about_webaddress
about_webaddress = lv.label(about)
about_webaddress.set_text("https://github.com/Wendellee/Renesas_Cup_Final_Game")
about_webaddress.set_long_mode(lv.label.LONG.WRAP)
about_webaddress.set_width(lv.pct(100))
about_webaddress.set_pos(112, 505)
about_webaddress.set_size(646, 26)
# Set style for about_webaddress, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
about_webaddress.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_text_font(test_font("montserratMedium", 20), lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_text_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_text_letter_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_webaddress.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

# Create about_title
about_title = lv.label(about)
about_title.set_text("About us")
about_title.set_long_mode(lv.label.LONG.WRAP)
about_title.set_width(lv.pct(100))
about_title.set_pos(432, 46)
about_title.set_size(173, 32)
# Set style for about_title, Part: lv.PART.MAIN, State: lv.STATE.DEFAULT.
about_title.set_style_border_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_radius(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_text_font(test_font("montserratMedium", 27), lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_text_opa(255, lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_text_letter_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_text_line_space(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_text_align(lv.TEXT_ALIGN.CENTER, lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_bg_opa(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_pad_top(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_pad_right(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_pad_bottom(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_pad_left(0, lv.PART.MAIN|lv.STATE.DEFAULT)
about_title.set_style_shadow_width(0, lv.PART.MAIN|lv.STATE.DEFAULT)

about.update_layout()

def main_event_handler(e):
    code = e.get_code()
    indev = lv.indev_active()
    gestureDir = lv.DIR.NONE
    if indev is not None: gestureDir = indev.get_gesture_dir()
    if (code == lv.EVENT.GESTURE and lv.DIR.BOTTOM == gestureDir):
        if indev is not None: indev.wait_release()
        pass
        

main.add_event_cb(lambda e: main_event_handler(e), lv.EVENT.ALL, None)

def about_event_handler(e):
    code = e.get_code()
    indev = lv.indev_active()
    gestureDir = lv.DIR.NONE
    if indev is not None: gestureDir = indev.get_gesture_dir()
    if (code == lv.EVENT.GESTURE and lv.DIR.TOP == gestureDir):
        if indev is not None: indev.wait_release()
        pass
        

about.add_event_cb(lambda e: about_event_handler(e), lv.EVENT.ALL, None)

# content from custom.py

# Load the default screen
lv.screen_load(about)

if __name__ == '__main__':
    while True:
        lv.task_handler()
        time.sleep_ms(5)
