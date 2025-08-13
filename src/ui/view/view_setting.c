#include "view_setting.h"
#include "view_wave.h"

#include "../font.h"

#include "lvgl.h"

#include <stdio.h>

// // 假设有以下外部函数可用（实际项目中需要实现）
// extern const char *get_hardware_info();
// extern const char *get_firmware_version();
// extern int get_battery_level();
// extern bool is_charging();
// extern const char *get_tf_card_info();
// extern void wifi_power_set(bool enable);
// extern void scan_wifi_networks();
// extern const char **get_wifi_list(int *count);
// extern const char *get_connected_wifi();
// extern void codec_set_enabled(bool enable);

// 界面组件指针
static lv_obj_t *root_container;
static lv_obj_t *lbl_hardware;
static lv_obj_t *lbl_version;
static lv_obj_t *lbl_battery;
static lv_obj_t *lbl_charging;
static lv_obj_t *lbl_tfcard;
static lv_obj_t *sw_wifi_power;
static lv_obj_t *sw_wifi_open;
static lv_obj_t *btn_scan_wifi;
static lv_obj_t *wifi_list;
static lv_obj_t *lbl_connected_wifi;
static lv_obj_t *sw_codec;

static void wifi_state_change(bool enable)
{
    if (enable)
    {
        lv_obj_remove_state(btn_scan_wifi, LV_STATE_DISABLED);
        lv_obj_remove_state(wifi_list, LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(btn_scan_wifi, LV_STATE_DISABLED);
        lv_obj_add_state(wifi_list, LV_STATE_DISABLED);
    }
}

static void wifi_power_change(bool enable)
{
    if (enable)
    {
        lv_obj_remove_state(sw_wifi_open, LV_STATE_DISABLED);
        if (lv_obj_has_state(sw_wifi_open, LV_STATE_CHECKED))
        {
            wifi_state_change(true);
        }
    }
    else
    {
        lv_obj_add_state(sw_wifi_open, LV_STATE_DISABLED);
        wifi_state_change(false);
    }
}

static void power_event(lv_event_t *event)
{
    lv_obj_t *sw = lv_event_get_target_obj(event);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    wifi_power_change(enabled);
}

static void wifi_event(lv_event_t *event)
{
    lv_obj_t *sw = lv_event_get_target_obj(event);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    wifi_power_change(enabled);
}

// 创建分区标题
static lv_obj_t *create_section_title(lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font_22, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(label, 5, 0);
    lv_obj_set_style_pad_bottom(label, 5, 0);
    lv_obj_set_style_pad_left(label, 20, 0);
    lv_obj_set_style_pad_right(label, 20, 0);
    // lv_obj_set_style_bg_opa(label, LV_OPA_20, 0);
    // lv_obj_set_style_radius(label, 10, 0);
    // lv_obj_set_style_bg_color(label, lv_palette_main(LV_PALETTE_GREY), 0);
    return label;
}

// 创建带标签的设置项
static lv_obj_t *create_setting_item(lv_obj_t *parent, const char *label_text, lv_obj_t **control)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), 60);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, label_text);
    lv_obj_set_style_text_font(label, font_16, 0);
    lv_obj_set_align(label, LV_ALIGN_LEFT_MID);

    if (control)
    {
        *control = lv_switch_create(cont);
        lv_obj_set_align(*control, LV_ALIGN_RIGHT_MID);
    }

    return cont;
}

// 更新数据
void lv_setting_init(const char *info, const char *version)
{
    // 更新系统信息
    lv_label_set_text(lbl_hardware, info);
    lv_label_set_text(lbl_version, version);
}

void lv_setting_update_power(bool charging, uint32_t level)
{
    // 更新电源信息
    static char bat_text[32];
    snprintf(bat_text, sizeof(bat_text), "电量: %d%%", level);
    lv_label_set_text(lbl_battery, bat_text);
    lv_label_set_text(lbl_charging, charging ? "充电中" : "未充电");
}

void lv_setting_update_tf(const char *info)
{
    // 更新存储信息
    lv_label_set_text(lbl_tfcard, info);
}

void lv_setting_update_wifi(const char *info)
{
    // 更新WiFi信息
    lv_label_set_text(lbl_connected_wifi, info);
}

void lv_setting_wifi_clear_list()
{
    lv_dropdown_clear_options(wifi_list);
}

void lv_setting_wifi_add_list(const char *item)
{
    lv_dropdown_add_option(wifi_list, item, 0);
}

void lv_setting_set_wifi(bool wireless, bool wifiopen)
{
}

void lv_setting_set_codec(bool enable)
{
    if (enable)
    {
        lv_obj_add_state(sw_codec, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_remove_state(sw_codec, LV_STATE_CHECKED);
    }
}

lv_obj_t *lv_setting_create(lv_obj_t *parent, lv_event_cb_t power, lv_event_cb_t wifi,
                            lv_event_cb_t scan, lv_event_cb_t codec)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(obj, 255, 0);
    lv_obj_set_size(obj, LV_HOR_RES, LV_VER_RES);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_wave_images_create(obj, 2);

    // 创建滚动容器
    root_container = lv_obj_create(obj);
    lv_obj_remove_style_all(root_container);
    lv_obj_set_size(root_container, LV_HOR_RES, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(root_container, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_flex_align(root_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(root_container, LV_DIR_VER);
    lv_obj_set_style_pad_all(root_container, 20, 0);
    lv_obj_set_style_pad_top(root_container, 60, 0);

    // ===== 无线控制区 =====
    create_section_title(root_container, "无线网络");

    // WiFi电源开关
    lv_obj_t *obj1 = create_setting_item(root_container, "无线模块电源", &sw_wifi_power);
    lv_obj_add_event_cb(sw_wifi_power, power, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(sw_wifi_power, power_event, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_style_margin_top(obj1, 10, 0);

    lv_obj_t *obj4 = create_setting_item(root_container, "WIFI", &sw_wifi_open);
    lv_obj_add_event_cb(sw_wifi_open, wifi, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(sw_wifi_open, wifi_event, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_style_margin_top(obj4, 10, 0);

    // 已连接WiFi
    lbl_connected_wifi = lv_label_create(root_container);
    lv_label_set_text(lbl_connected_wifi, "未连接网络");
    lv_obj_set_style_margin_top(obj1, 10, 0);

    lv_obj_t *obj2 = lv_obj_create(root_container);
    lv_obj_remove_style_all(obj2);
    lv_obj_set_size(obj2, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_margin_top(obj2, 10, 0);

    lv_obj_t *label = lv_label_create(obj2);
    lv_obj_set_align(label, LV_ALIGN_LEFT_MID);
    lv_label_set_text(label, "可用网络列表");

    // 扫描按钮
    btn_scan_wifi = lv_button_create(obj2);
    lv_obj_set_align(btn_scan_wifi, LV_ALIGN_RIGHT_MID);
    lv_obj_t *btn_label = lv_label_create(btn_scan_wifi);
    lv_label_set_text(btn_label, "扫描WiFi");
    lv_obj_add_event_cb(btn_scan_wifi, scan, LV_EVENT_CLICKED, NULL);

    // WiFi列表
    wifi_list = lv_dropdown_create(root_container);
    lv_dropdown_set_options(wifi_list, "");
    lv_obj_set_width(wifi_list, LV_PCT(100));
    lv_obj_set_style_margin_top(wifi_list, 10, 0);

    // ===== 音频设置区 =====
    lv_obj_t *obj5 = create_section_title(root_container, "音频设置");
    lv_obj_set_style_margin_top(obj5, 40, 0);

    // CODEC开关
    lv_obj_t *obj6 = create_setting_item(root_container, "双CS43198", &sw_codec);
    lv_obj_add_event_cb(sw_codec, codec, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_set_style_margin_top(obj6, 10, 0);

    // ===== 系统信息区 =====
    create_section_title(root_container, "系统信息");

    lbl_hardware = lv_label_create(root_container);
    lv_label_set_text(lbl_hardware, "硬件: ");

    lbl_version = lv_label_create(root_container);
    lv_label_set_text(lbl_version, "版本: ");

    // ===== 电源管理区 =====
    create_section_title(root_container, "电源管理");

    lbl_battery = lv_label_create(root_container);
    lv_label_set_text(lbl_battery, "电量: ");

    lbl_charging = lv_label_create(root_container);
    lv_label_set_text(lbl_charging, "充电状态: ");

    // ===== 存储信息区 =====
    create_section_title(root_container, "存储信息");

    lbl_tfcard = lv_label_create(root_container);
    lv_label_set_text(lbl_tfcard, "TF卡: ");

    wifi_power_change(false);

    return obj;
}
