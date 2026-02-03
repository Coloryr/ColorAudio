#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <queue>
#include <semaphore.h>

#include "lvgl.h"
#include "lv_conf.h"

#include "display/lv_port_init.h"
#include "main.h"
#include "ui/ui.h"
#include "ui/info_view.h"
#include "ui/setting_view.h"
#include "ui/lang.h"
#include "sound/sound.h"
#include "sound/sound_fft.h"
#include "input/rime_input.h"
#include "net/http_connect.h"
#include "music/music_player.h"
#include "music/net_music.h"
#include "music/local_music.h"
#include "config/config.h"
#include "wireless/wifi.h"
#include "wireless/bt.h"
#include "io/event.h"
#include "io/gpio.h"
#include "io/wireless.h"
#include "usb/usb_audio.h"

#ifndef BUILD_ARM
#define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/
#include <SDL2/SDL.h>
#endif

using namespace coloraudio::config;

static int quit = 0;
static pthread_t tid;
static pthread_t work_tid;
static sem_t work_sem;

static bool mode_change;
static main_mode_type now_mode = MAIN_MODE_NONE;
static std::queue<main_work *> work_queue;

static void sigterm_handler(int sig)
{
#ifdef BUILD_ARM
    set_amp_power(false);
#endif
    LV_LOG_USER("ColorAudio Exit %d\n", sig);

    exit(0);
}

static void *work_loop(void *arg)
{
    for (;;)
    {
        sem_wait(&work_sem);
        while (!work_queue.empty())
        {
#ifdef BUILD_ARM
            auto now_work = work_queue.front();
            if (now_work == NULL)
            {
                continue;
            }
            if (now_work->type == MAIN_WORK_USB)
            {
                usb_audio_exit();
            }
            else if (now_work->type == MAIN_WORK_WIFI_POWER)
            {
                if (Config::get_config_wifi_power())
                {
                    view_top_info_display(now_lang->setting_text7);
                    wifi_wait_ready();
                    view_top_info_close();
                }
                else
                {
                    view_top_info_display(now_lang->setting_text8);
                    wifi_wait_deactivate();
                    view_top_info_close();
                }
            }
            else if (now_work->type == MAIN_WORK_WIFI_ENABLE)
            {
                if (Config::get_config_wifi_enable())
                {
                    if (!wifi_have_device())
                    {
                        view_top_error_display(now_lang->setting_text11);
                    }
                    else
                    {
                        view_top_info_display(now_lang->setting_text9);
                        if (wifi_is_wpa_supplicant_running())
                        {
                            wifi_terminate_wpa_supplicant();
                        }
                        wifi_wpa_start();
                        view_top_info_close();
                    }
                }
                else
                {
                    view_top_info_display(now_lang->setting_text10);
                    wifi_terminate_wpa_supplicant();
                    view_top_info_close();
                }
            }
            else if (now_work->type == MAIN_WORK_WIFI_SCAN)
            {
                if (!wifi_have_device() || !wifi_is_wpa_supplicant_running())
                {
                    view_top_error_display(now_lang->setting_text12);
                }
                else
                {
                    std::vector<wifi_item_t> list;
                    if (wifi_scan(list))
                    {
                        view_setting_wifi_list(list);
                    }
                    else
                    {
                        view_top_error_display(now_lang->setting_text13);
                    }
                }
            }
            else if (now_work->type == MAIN_WORK_WIFI_CONNECT)
            {
                if (!wifi_have_device() || !wifi_is_wpa_supplicant_running())
                {
                    view_top_error_display(now_lang->setting_text12);
                }
                else
                {
                    wifi_connect_t *wifi = static_cast<wifi_connect_t *>(now_work->data);
                    std::string ssid = std::string(wifi->ssid);
                    std::string pwd = std::string(wifi->pwd);
                    delete wifi;
                    if (!wifi_connect(ssid, pwd))
                    {
                        view_top_error_display(now_lang->setting_text15);
                    }
                }
            }
            else if (now_work->type == MAIN_WORK_WIFI_DISCONNECT)
            {
                if (!wifi_have_device() || !wifi_is_wpa_supplicant_running())
                {
                    view_top_error_display(now_lang->setting_text12);
                }
                else
                {
                    if (!wifi_remove())
                    {
                        view_top_error_display(now_lang->setting_text19);
                    }
                }
            }
            work_queue.pop();
            delete now_work;
#endif
        }
    }
    return NULL;
}

static void *main_loop(void *arg)
{
    change_mode(Config::get_config_main_mode());

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
            wifi_wait_ready();
#endif
            view_top_info_close();
            bt_run_loop();
        }
        else if (now_mode == MAIN_MODE_USB)
        {
            usb_audio_tick();
        }
    }

    return NULL;
}

main_mode_type get_mode()
{
    return now_mode;
}

void add_work(main_work_type work, void *data)
{
    main_work *item = new main_work();
    item->type = work;
    item->data = data;
    work_queue.push(item);
    sem_post(&work_sem);
}

void change_mode(main_mode_type mode)
{
    if (mode == now_mode)
    {
        return;
    }

    mode_change = true;

    if (now_mode == MAIN_MODE_MUSIC)
    {
        music_close();
    }
    else if (now_mode == MAIN_MODE_BLE)
    {
        bt_stop();
    }
    else if (now_mode == MAIN_MODE_USB)
    {
        add_work(MAIN_WORK_USB, NULL);
        usb_audio_stop();
    }

    if (mode == MAIN_MODE_MUSIC)
    {
        music_go_local();
    }
    else if (mode == MAIN_MODE_BLE)
    {
        bt_init();
    }
    else if (mode == MAIN_MODE_USB)
    {
        usb_audio_start();
    }

    now_mode = mode;
    Config::set_config_main_mode(now_mode);
    Config::save_config();

    mode_change = false;
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigterm_handler);

    Config::load_config();

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

    sem_init(&work_sem, 0, 0);

    pthread_create(&tid, NULL, main_loop, NULL);
    pthread_setname_np(tid, "main_loop");

    pthread_create(&work_tid, NULL, work_loop, NULL);
    pthread_setname_np(work_tid, "work_loop");

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
