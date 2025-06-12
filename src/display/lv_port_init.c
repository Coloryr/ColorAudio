#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "lvgl.h"
#include "lv_conf.h"

#include "lv_port_disp.h"
#include "lv_port_indev.h"

lv_display_t * disp;

/* 0, 90, 180, 270 */
static int g_indev_rotation = 0;
/* 0, 90, 180, 270 */
static int g_disp_rotation = 0;

static uint32_t lv_systick(void)
{
    static uint64_t start_ms = 0;
    if (start_ms == 0)
    {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}

static void lv_sysdelay(uint32_t ms)
{
    usleep(ms * 1000);
}

void lv_port_init(void)
{
    const char *buf;

    lv_init();

    disp = lv_port_disp_init(800, 480, g_disp_rotation);
    lv_port_indev_init(g_indev_rotation);
}

