#include "view_main.h"
#include "view_wave.h"
#include "view_header.h"

#include "ui/lang.h"
#include "ui/view_setting.h"
#include "ui/font.h"
#include "main.h"

#include "lvgl.h"

LV_IMAGE_DECLARE(img_lv_music);
LV_IMAGE_DECLARE(img_lv_ble);
LV_IMAGE_DECLARE(img_lv_usb);
LV_IMAGE_DECLARE(img_lv_setting);

static main_button_type args[] = {MAIN_BUTTON_MUSIC,
                                  MAIN_BUTTON_BLE,
                                  MAIN_BUTTON_USB,
                                  MAIN_BUTTON_SETTING};
static lv_style_t style;

static lv_obj_t *now_mode_obj;

lv_obj_t *lv_main_create(lv_obj_t *parent, lv_event_cb_t cb)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(obj, 255, 0);
    lv_obj_set_size(obj, LV_HOR_RES, LV_VER_RES);

    lv_wave_images_create(obj, 2);

    uint32_t wid = lv_obj_get_width(parent);

    now_mode_obj = lv_label_create(obj);
    lv_obj_set_size(now_mode_obj, wid - LV_MUSIC_HANDLE_SIZE, LV_SIZE_CONTENT);
    lv_obj_align(now_mode_obj, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_text_align(now_mode_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(now_mode_obj, font_22, 0);
    lv_label_set_long_mode(now_mode_obj, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_label_set_text(now_mode_obj, now_lang->title);

    lv_obj_t *cont = lv_obj_create(obj);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(cont);

    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_opa(&style, 255);
    lv_style_set_radius(&style, 10);
    lv_style_set_shadow_width(&style, 20);
    lv_style_set_shadow_spread(&style, 10);
    lv_style_set_shadow_color(&style, lv_color_hex(0xd9c5ff));

    lv_obj_t *button = lv_obj_create(cont);
    lv_obj_remove_style_all(button);
    lv_obj_add_style(button, &style, 0);
    lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, &args[0]);
    lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(button, 150, 150);
    lv_obj_set_style_margin_bottom(button, 30, 0);

    lv_obj_t *btn_cont = lv_obj_create(button);
    lv_obj_remove_style_all(btn_cont);
    lv_obj_remove_flag(btn_cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(btn_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(btn_cont);

    lv_obj_t *icon = lv_image_create(btn_cont);
    lv_image_set_src(icon, &img_lv_music);
    lv_obj_set_style_margin_right(icon, 10, 0);

    lv_obj_t *label = lv_label_create(btn_cont);
    lv_label_set_text(label, now_lang->main_text1);
    lv_obj_set_style_text_font(label, font_22, 0);

    lv_obj_t *button1 = lv_obj_create(cont);
    lv_obj_remove_style_all(button1);
    lv_obj_add_style(button1, &style, 0);
    lv_obj_add_flag(button1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(button1, cb, LV_EVENT_CLICKED, &args[1]);
    lv_obj_set_size(button1, 150, 150);
    lv_obj_set_style_margin_bottom(button1, 30, 0);

    lv_obj_t *btn_cont1 = lv_obj_create(button1);
    lv_obj_remove_style_all(btn_cont1);
    lv_obj_remove_flag(btn_cont1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(btn_cont1, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_cont1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_cont1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(btn_cont1);

    lv_obj_t *icon1 = lv_image_create(btn_cont1);
    lv_image_set_src(icon1, &img_lv_ble);
    lv_obj_set_style_margin_right(icon1, 10, 0);

    label = lv_label_create(btn_cont1);
    lv_label_set_text(label, now_lang->main_text2);
    lv_obj_set_style_text_font(label, font_22, 0);

    lv_obj_t *button2 = lv_obj_create(cont);
    lv_obj_remove_style_all(button2);
    lv_obj_add_style(button2, &style, 0);
    lv_obj_add_flag(button2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(button2, cb, LV_EVENT_CLICKED, &args[2]);
    lv_obj_set_size(button2, 150, 150);
    lv_obj_set_style_margin_bottom(button2, 30, 0);

    lv_obj_t *btn_cont2 = lv_obj_create(button2);
    lv_obj_remove_style_all(btn_cont2);
    lv_obj_remove_flag(btn_cont2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(btn_cont2, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_cont2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_cont2, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(btn_cont2);

    lv_obj_t *icon2 = lv_image_create(btn_cont2);
    lv_image_set_src(icon2, &img_lv_usb);
    lv_obj_set_style_margin_right(icon2, 10, 0);

    label = lv_label_create(btn_cont2);
    lv_label_set_text(label, now_lang->main_text3);
    lv_obj_set_style_text_font(label, font_22, 0);

    lv_obj_t *button3 = lv_obj_create(cont);
    lv_obj_remove_style_all(button3);
    lv_obj_add_style(button3, &style, 0);
    lv_obj_add_flag(button3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(button3, cb, LV_EVENT_CLICKED, &args[3]);
    lv_obj_set_size(button3, 150, 40);

    lv_obj_t *btn_cont3 = lv_obj_create(button3);
    lv_obj_remove_style_all(btn_cont3);
    lv_obj_remove_flag(btn_cont3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(btn_cont3, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_cont3, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont3, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(btn_cont3);

    lv_obj_t *icon3 = lv_image_create(btn_cont3);
    lv_image_set_src(icon3, &img_lv_setting);
    lv_obj_set_style_margin_right(icon3, 10, 0);

    label = lv_label_create(btn_cont3);
    lv_label_set_text(label, now_lang->main_text4);
    lv_obj_set_style_text_font(label, font_22, 0);

    return obj;
}

void lv_main_set_now(const char *text)
{
    lv_label_set_text(now_mode_obj, text);
}
