#ifndef __MAIN_H__
#define __MAIN_H__

typedef enum
{
    MAIN_MODE_NONE,
    MAIN_MODE_MUSIC,
    MAIN_MODE_BLE,
    MAIN_MODE_USB
} main_mode_type;

void change_mode(main_mode_type mode);

#endif

