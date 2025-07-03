#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "lvgl.h"

#include "../lv_conf.h"
#include "../ui/font.h"

#include "lv_port_disp.h"
#include "lv_port_indev.h"

/* 0, 90, 180, 270 */
// static int g_indev_rotation = 0;
/* 0, 90, 180, 270 */
static int g_disp_rotation = 0;

void lv_port_init(void)
{
    const char *buf;

    lv_init();
    load_font();

    lv_port_disp_init(480, 800, g_disp_rotation);
    lv_port_indev_init();
}

