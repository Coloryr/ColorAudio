#include "view_dialog.h"

#include "../font.h"
#include "../anim.h"

#include "lvgl.h"

static uint8_t arg[] = {0, 1};

static lv_obj_t *dialog_obj;
static lv_obj_t *label_obj;

void lv_create_dialog(lv_obj_t *parent, lv_event_cb_t cb)
{
    // 创建背景遮罩（全屏半透明背景）
    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_remove_style_all(root);
    lv_obj_set_size(root, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(root, LV_OPA_50, 0);
    lv_obj_set_style_bg_color(root, lv_color_black(), 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    // 创建对话框容器
    lv_obj_t *container = lv_obj_create(root);
    lv_obj_remove_style_all(container);
    lv_obj_set_size(container, 300, 200);
    lv_obj_center(container);
    lv_obj_set_style_radius(container, 10, 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(container, lv_color_hex(0x2c2c2c), 0);
    lv_obj_set_style_pad_all(container, 20, 0);

    // 创建消息文本
    label_obj = lv_label_create(container);
    lv_obj_set_width(label_obj, lv_pct(100));
    lv_obj_set_style_text_font(label_obj, font_22, 0);
    lv_obj_set_style_text_color(label_obj, lv_color_white(), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_obj, LV_ALIGN_TOP_MID, 0, 20);

    // 创建按钮容器
    lv_obj_t *btn_container = lv_obj_create(container);
    lv_obj_remove_style_all(btn_container);
    lv_obj_set_size(btn_container, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -20);

    // 创建取消按钮
    lv_obj_t *btn_cancel = lv_btn_create(btn_container);
    lv_obj_set_size(btn_cancel, 100, 40);
    lv_obj_t *btn_cancel_label = lv_label_create(btn_cancel);
    lv_label_set_text(btn_cancel_label, "取消");
    lv_obj_center(btn_cancel_label);

    // 创建确认按钮
    lv_obj_t *btn_confirm = lv_btn_create(btn_container);
    lv_obj_set_size(btn_confirm, 100, 40);
    lv_obj_t *btn_confirm_label = lv_label_create(btn_confirm);
    lv_label_set_text(btn_confirm_label, "确认");
    lv_obj_center(btn_confirm_label);

    // 设置按钮样式
    lv_obj_set_style_bg_color(btn_confirm, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0xF44336), 0);
    lv_obj_set_style_text_color(btn_confirm, lv_color_white(), 0);
    lv_obj_set_style_text_color(btn_cancel, lv_color_white(), 0);

    // 绑定按钮事件
    lv_obj_add_event_cb(btn_confirm, cb, LV_EVENT_CLICKED, &arg[1]);
    lv_obj_add_event_cb(btn_cancel, cb, LV_EVENT_CLICKED, &arg[0]);

    dialog_obj = root;

    lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
}

void lv_dialog_set_text(const char *message)
{
    lv_label_set_text(label_obj, message);
}

void lv_dialog_display(bool dispaly)
{
    anim_set_display(dialog_obj, dispaly);
}