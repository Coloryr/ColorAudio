#ifndef __VIEW_SETTING_H__
#define __VIEW_SETTING_H__

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void lv_setting_wifi_clear_list();
void lv_setting_wifi_add_list(const char *item);
void lv_setting_init(const char *info, const char *version);
void lv_setting_update_power(bool charging, uint32_t level);
void lv_setting_update_tf(const char *info);
void lv_setting_update_wifi(const char *info);
void lv_setting_set_wifi(bool wifipower, bool wifiopen);
void lv_setting_set_disconnect(bool enable);
void lv_setting_set_codec(bool enable);

lv_obj_t *lv_setting_create(lv_obj_t *parent, lv_event_cb_t power, lv_event_cb_t wifi,
                            lv_event_cb_t scan, lv_event_cb_t codec,
                            lv_event_cb_t list, lv_event_cb_t disconnect);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // __VIEW_SETTING_H__