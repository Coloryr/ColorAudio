#include "event.h"

#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <pthread.h>
#include <linux/input.h>
#include <thread>
#include <vector>
#include <mutex>
#include <string>

#include "ui/header_view.h"

#ifdef BUILD_ARM

using namespace coloraudio::io::event;

bool headphone_1_in = false;
bool headphone_2_in = false;
KEY_EVENT key_data = KEY_UNKNOW;

static std::vector<InputDeviceListener *> instances;
static pthread_t event_thread;
static bool loop_running = false;

InputDeviceListener::InputDeviceListener(const char *device)
    : device_path(device)
{
    fd = open(device, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        throw "Failed to open input device";
    }
    is_run = true;
}

InputDeviceListener::~InputDeviceListener()
{
    if (fd >= 0)
    {
        close(fd);
    }
}

void InputDeviceListener::read_event()
{
    struct input_event ev;
    int bytes = read(fd, &ev, sizeof(ev));
    if (bytes == sizeof(ev))
    {
        if (device_path.ends_with("event4"))
        {
            if (ev.type == 5 && ev.code == 7)
            {
                headphone_2_in = ev.value == 1;
            }
            view_header_update();
        }
        else if (device_path.ends_with("event3"))
        {
            if (ev.type == 5 && ev.code == 7)
            {
                headphone_1_in = ev.value == 1;
            }
            view_header_update();
        }

        LV_LOG_USER("Event from %s: type=%d, code=%d, value=%d", device_path, ev.type, ev.code, ev.value);
    }
    else if (bytes == -1 && errno != EAGAIN)
    {
        LV_LOG_ERROR("Eevnt read: %s, with error: %d", device_path, bytes);
        is_run = false;
    }
}

static void *event_loop(void *arg)
{
    while (loop_running)
    {
        for (auto listener : instances)
        {
            if (listener->can_run())
            {
                listener->read_event();
            }
        }
        usleep(10000); // 10ms
    }

    return nullptr;
}

void event_init()
{
    instances.push_back(new InputDeviceListener("/dev/input/event0"));
    instances.push_back(new InputDeviceListener("/dev/input/event3"));
    instances.push_back(new InputDeviceListener("/dev/input/event4"));
    instances.push_back(new InputDeviceListener("/dev/input/event5"));

    if (!loop_running)
    {
        loop_running = true;
        int res = pthread_create(&event_thread, NULL, event_loop, NULL);
        if (!res)
        {
            LV_LOG_ERROR("Event read thread run fail: %d", res);
        }
    }
}

void event_stop()
{
    loop_running = false;
    if (event_thread)
    {
        pthread_detach(event_thread);
        event_thread = NULL;
    }
}

#endif