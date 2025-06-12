#include <lvgl.h>
#include <lv_conf.h>

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <malloc.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <time.h>
#include "lvgl.h"

#include "lv_port_init.h"
#include "timestamp.h"

#include "main.h"
#include "view.h"
#include "sound.h"
#include "player.h"

#ifndef BUILD_ARM
#define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/
#include <SDL2/SDL.h>
#endif

static int quit = 0;

static void sigterm_handler(int sig)
{
    fprintf(stderr, "signal %d\n", sig);
    quit = 1;
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigterm_handler);
    lv_port_init();

    alsa_init();
    play_init();

    view_init();

#ifndef BUILD_ARM
    uint32_t lastTick = SDL_GetTicks();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint32_t lastTick = ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // 转为毫秒
#endif
    while (!quit)
    {
#ifdef BUILD_ARM
        usleep(500);
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint32_t current = ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // 转为毫秒
        lv_tick_inc(current - lastTick); // Update the tick timer. Tick is new for LVGL 9
        lastTick = current;
#else
        SDL_Delay(5);
        uint32_t current = SDL_GetTicks();
        lv_tick_inc(current - lastTick); // Update the tick timer. Tick is new for LVGL 9
        lastTick = current;
#endif
        lv_task_handler();
    }

    return 0;
}
