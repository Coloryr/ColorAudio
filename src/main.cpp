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
#include "lv_conf.h"

#include "display/lv_port_init.h"
#include "sys/timestamp.h"

#include "main.h"
#include "ui/view.h"
#include "ui/ui.h"
#include "player/sound.h"
#include "player/player.h"
#include "input/rime_input.h"
#include "net/http_connect.h"
#include "music/net_music.h"
#include "music/local_music.h"

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

    rime_init();

    view_init();

    // local_music_init();
    net_music_init();

#ifdef BUILD_ARM
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint32_t lastTick = ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // 转为毫秒
#else
    uint32_t lastTick = SDL_GetTicks();
#endif
    while (!quit)
    {
        uint32_t current;
#ifdef BUILD_ARM
        clock_gettime(CLOCK_MONOTONIC, &ts);
        current = ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // 转为毫秒
#else
        SDL_Delay(1);
        current = SDL_GetTicks();
#endif
        view_tick();
        lv_tick_inc(current - lastTick); // Update the tick timer. Tick is new for LVGL 9
        lastTick = current;
        lv_task_handler();
    }

    return 0;
}
