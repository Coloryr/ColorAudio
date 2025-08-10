#include "lvgl.h"
#include "lv_conf.h"

#include "display/lv_port_init.h"
#include "main.h"
#include "ui/ui.h"
#include "ui/info_view.h"
#include "sound/sound.h"
#include "input/rime_input.h"
#include "net/http_connect.h"
#include "music/music_player.h"
#include "music/net_music.h"
#include "music/local_music.h"
#include "config/config.h"
#include "wireless/wifi.h"
#include "wireless/ble.h"
#include "wireless/le_audio.h"
#include "io/event.h"
#include "io/gpio.h"
#include "io/wireless.h"
#include "usb/usb_audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef BUILD_ARM
#define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/
#include <SDL2/SDL.h>
#endif

using namespace ColorAudio;

static int quit = 0;
static pthread_t tid;

static bool mode_change;

static main_mode_type now_mode = MAIN_MODE_NONE;

static void sigterm_handler(int sig)
{
#ifdef BUILD_ARM
    set_amp_power(false);
#endif
    LV_LOG_USER("ColorAudio Exit %d\n", sig);

    exit(0);
}

static void *main_loop(void *arg)
{
    change_mode(config::get_config_main_mode());

    for (;;)
    {
        usleep(100);
        if (mode_change)
        {
            continue;
        }
        if (now_mode == MAIN_MODE_MUSIC)
        {
            music_run_loop();
        }
        else if (now_mode == MAIN_MODE_BLE)
        {
            view_top_info_display("正在启用蓝牙");
#ifdef BUILD_ARM
            set_wireless_power_on();
#endif
            view_top_info_close();
            ble_run_loop();
        }
        else if (now_mode == MAIN_MODE_USB)
        {
            usb_audio_tick();
        }
    }
}

main_mode_type get_mode()
{
    return now_mode;
}

void change_mode(main_mode_type mode)
{
    if (mode == now_mode)
    {
        return;
    }

    if (now_mode == MAIN_MODE_MUSIC)
    {
        music_close();
    }
    else if (now_mode == MAIN_MODE_BLE)
    {
        ble_stop();
    }
    else if (now_mode == MAIN_MODE_USB)
    {
        usb_audio_stop();
    }

    if (mode == MAIN_MODE_MUSIC)
    {
        music_go_local();
    }
    else if (mode == MAIN_MODE_BLE)
    {
        ble_init();
    }
    else if (mode == MAIN_MODE_USB)
    {
        usb_audio_start();
    }

    now_mode = mode;
    config::set_config_main_mode(now_mode);
    config::save_config();
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigterm_handler);

    config::load_config();

    alsa_init();
#ifdef BUILD_ARM
    set_amp_power(true);
#endif

    play_init();
    rime_init();
    lv_port_init();

    view_init();
    music_init();
    usb_audio_init();
#ifdef BUILD_ARM
    event_init();
#endif

    pthread_create(&tid, NULL, main_loop, NULL);
    pthread_setname_np(tid, "main loop");

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
        SDL_Delay(5);
        current = SDL_GetTicks();
#endif
        view_tick();
        lv_tick_inc(current - lastTick); // Update the tick timer. Tick is new for LVGL 9
        lastTick = current;
        lv_task_handler();
    }

    return 0;
}
