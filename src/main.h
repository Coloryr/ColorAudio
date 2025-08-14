#ifndef __MAIN_H__
#define __MAIN_H__

typedef enum
{
    MAIN_MODE_NONE = 0,
    MAIN_MODE_MUSIC,
    MAIN_MODE_BLE,
    MAIN_MODE_USB
} main_mode_type;

typedef enum
{
    MAIN_WORK_NONE = 0,
    MAIN_WORK_USB,
    MAIN_WORK_WIFI_POWER,
    MAIN_WORK_WIFI_ENABLE,
    MAIN_WORK_WIFI_SCAN,
    MAIN_WORK_WIFI_CONNECT,
    MAIN_WORK_WIFI_DISCONNECT,
} main_work_type;

typedef struct
{
    main_work_type type;
    void *data;
} main_work;

void change_mode(main_mode_type mode);
void add_work(main_work_type work, void *data);
main_mode_type get_mode();

#endif
