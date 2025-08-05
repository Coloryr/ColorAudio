#ifndef _UI_H_
#define _UI_H_

#include <stdint.h>

typedef enum
{
    VIEW_MAIN,
    VIEW_MUSIC,
    VIEW_BLE,
    VIEW_USB,
    VIEW_SETTING
} view_mode_type;

view_mode_type get_view_mode();
void view_jump(view_mode_type type);
void view_set_fft_data(uint16_t size, float* value);

void view_init();
void view_tick();

#endif