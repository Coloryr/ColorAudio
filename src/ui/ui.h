#ifndef _UI_H_
#define _UI_H_

typedef enum
{
    VIEW_MAIN,
    VIEW_MUSIC,
    VIEW_BLE,
    VIEW_USB,
    VIEW_SETTING
} view_type;

void view_jump(view_type type);

void view_init();
void view_tick();

#endif