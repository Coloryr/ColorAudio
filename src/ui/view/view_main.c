#include "view_main.h"

#include "../view_setting.h"
#include "../font.h"
#include "../main.h"

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

static void create_wave_images(lv_obj_t *parent)
{
    LV_IMAGE_DECLARE(img_lv_bg1);
    LV_IMAGE_DECLARE(img_lv_bg2);
    lv_obj_t *wave_top = lv_image_create(parent);
    lv_image_set_src(wave_top, &img_lv_bg2);
    lv_image_set_inner_align(wave_top, LV_IMAGE_ALIGN_TILE);
    lv_obj_set_width(wave_top, LV_HOR_RES);
    lv_obj_align(wave_top, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_flag(wave_top, LV_OBJ_FLAG_IGNORE_LAYOUT);

    lv_obj_t *wave_bottom = lv_image_create(parent);
    lv_image_set_src(wave_bottom, &img_lv_bg1);
    lv_image_set_inner_align(wave_bottom, LV_IMAGE_ALIGN_TILE);
    lv_obj_set_width(wave_bottom, LV_HOR_RES);
    lv_obj_align(wave_bottom, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(wave_bottom, LV_OBJ_FLAG_IGNORE_LAYOUT);
}

lv_obj_t *lv_main_create(lv_obj_t *parent, lv_event_cb_t cb)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(obj, 255, 0);
    lv_obj_set_size(obj, LV_HOR_RES, LV_VER_RES);

    create_wave_images(obj);

    uint32_t wid = lv_obj_get_width(parent);

    now_mode_obj = lv_label_create(obj);
    lv_obj_set_size(now_mode_obj, wid - LV_MUSIC_HANDLE_SIZE, LV_SIZE_CONTENT);
    lv_obj_align(now_mode_obj, LV_ALIGN_TOP_MID, 0, 40);
    lv_style_set_text_align(now_mode_obj, LV_TEXT_ALIGN_CENTER);
    lv_label_set_long_mode(now_mode_obj, LV_LABEL_LONG_MODE_SCROLL_CIRCULAR);
    lv_label_set_text(now_mode_obj, "欢迎使用ColorAudio");

    // 创建Flex容器实现垂直居中布局
    lv_obj_t *cont = lv_obj_create(obj);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(cont);

    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_opa(&style, 255);
    lv_style_set_radius(&style, 10);                           // 增加圆角
    lv_style_set_shadow_width(&style, 20);                     // 增加阴影宽度
    lv_style_set_shadow_spread(&style, 10);                    // 增加阴影扩散
    lv_style_set_shadow_color(&style, lv_color_hex(0xd9c5ff)); // 调整阴影颜色

    // 创建按钮1 - 带图标
    lv_obj_t *button = lv_obj_create(cont);
    lv_obj_remove_style_all(button);
    lv_obj_add_style(button, &style, 0);
    lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, &args[0]);
    lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(button, 150, 150);
    lv_obj_set_style_margin_bottom(button, 30, 0);

    // 创建水平布局容器
    lv_obj_t *btn_cont = lv_obj_create(button);
    lv_obj_remove_style_all(btn_cont);
    lv_obj_remove_flag(btn_cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(btn_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(btn_cont);

    // 添加图标
    lv_obj_t *icon = lv_image_create(btn_cont);
    lv_image_set_src(icon, &img_lv_music);
    lv_obj_set_style_margin_right(icon, 10, 0);

    // 添加文本标签
    lv_obj_t *label = lv_label_create(btn_cont);
    lv_label_set_text(label, "本地音乐");
    lv_obj_set_style_text_font(label, font_22, 0);

    // 创建按钮2 - 带图标
    lv_obj_t *button1 = lv_obj_create(cont);
    lv_obj_remove_style_all(button1);
    lv_obj_add_style(button1, &style, 0);
    lv_obj_add_flag(button1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(button1, cb, LV_EVENT_CLICKED, &args[1]);
    lv_obj_set_size(button1, 150, 150);
    lv_obj_set_style_margin_bottom(button1, 30, 0);

    // 创建水平布局容器
    lv_obj_t *btn_cont1 = lv_obj_create(button1);
    lv_obj_remove_style_all(btn_cont1);
    lv_obj_remove_flag(btn_cont1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(btn_cont1, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_cont1, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_cont1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(btn_cont1);

    // 添加图标
    lv_obj_t *icon1 = lv_image_create(btn_cont1);
    lv_image_set_src(icon1, &img_lv_ble);
    lv_obj_set_style_margin_right(icon1, 10, 0);

    // 添加文本标签
    label = lv_label_create(btn_cont1);
    lv_label_set_text(label, "蓝牙音频");
    lv_obj_set_style_text_font(label, font_22, 0);

    // 创建按钮3 - 带图标
    lv_obj_t *button2 = lv_obj_create(cont);
    lv_obj_remove_style_all(button2);
    lv_obj_add_style(button2, &style, 0);
    lv_obj_add_flag(button2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(button2, cb, LV_EVENT_CLICKED, &args[2]);
    lv_obj_set_size(button2, 150, 150);
    lv_obj_set_style_margin_bottom(button2, 30, 0);

    // 创建水平布局容器
    lv_obj_t *btn_cont2 = lv_obj_create(button2);
    lv_obj_remove_style_all(btn_cont2);
    lv_obj_remove_flag(btn_cont2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(btn_cont2, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_cont2, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_cont2, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(btn_cont2);

    // 添加图标
    lv_obj_t *icon2 = lv_image_create(btn_cont2);
    lv_image_set_src(icon2, &img_lv_usb);
    lv_obj_set_style_margin_right(icon2, 10, 0);

    // 添加文本标签
    label = lv_label_create(btn_cont2);
    lv_label_set_text(label, "USB音频");
    lv_obj_set_style_text_font(label, font_22, 0);

    // 创建按钮3 - 带图标
    lv_obj_t *button3 = lv_obj_create(cont);
    lv_obj_remove_style_all(button3);
    lv_obj_add_style(button3, &style, 0);
    lv_obj_add_flag(button3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(button3, cb, LV_EVENT_CLICKED, &args[3]);
    lv_obj_set_size(button3, 150, 40);

    // 创建水平布局容器
    lv_obj_t *btn_cont3 = lv_obj_create(button3);
    lv_obj_remove_style_all(btn_cont3);
    lv_obj_remove_flag(btn_cont3, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(btn_cont3, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_cont3, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont3, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(btn_cont3);

    // 添加图标
    lv_obj_t *icon3 = lv_image_create(btn_cont3);
    lv_image_set_src(icon3, &img_lv_setting);
    lv_obj_set_style_margin_right(icon3, 10, 0);

    // 添加文本标签
    label = lv_label_create(btn_cont3);
    lv_label_set_text(label, "设置");
    lv_obj_set_style_text_font(label, font_22, 0);

    return obj;
}

void lv_main_set_now(const char *text)
{
    lv_label_set_text(now_mode_obj, text);
}
