#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include "ui/header_view.h"

#include "event.h"

#ifdef BUILD_ARM

bool headphone_1_in = false;
bool headphone_2_in = false;

static std::vector<InputDeviceListener *> instances;
static std::mutex instances_mutex;
static bool loop_running = false;
static std::thread event_thread;

InputDeviceListener::InputDeviceListener(const char *device_path)
    : device_path(device_path), running(false)
{
    fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0)
    {
        throw std::runtime_error("Failed to open input device");
    }

    std::lock_guard<std::mutex> lock(instances_mutex);
    instances.push_back(this);
    running = true;
}

InputDeviceListener::~InputDeviceListener()
{
    running = false;
    if (fd >= 0)
    {
        close(fd);
    }

    std::lock_guard<std::mutex> lock(instances_mutex);
    instances.erase(std::remove(instances.begin(), instances.end(), this));
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
        else
        {
            std::cout << "Event from " << device_path
                      << ": type=" << ev.type
                      << ", code=" << ev.code
                      << ", value=" << ev.value << std::endl;
        }
    }
    else if (bytes == -1 && errno != EAGAIN)
    {
        running = false;
        delete this;
    }
}

static void event_loop()
{
    while (loop_running)
    {
        std::vector<InputDeviceListener *> current_instances;
        {
            std::lock_guard<std::mutex> lock(instances_mutex);
            current_instances = instances;
        }

        for (auto listener : current_instances)
        {
            if (listener->running)
            {
                listener->read_event();
            }
        }
        usleep(10000); // 10ms
    }
}

void event_init()
{
    new InputDeviceListener("/dev/input/event0");
    new InputDeviceListener("/dev/input/event3");
    new InputDeviceListener("/dev/input/event4");
    new InputDeviceListener("/dev/input/event5");

    if (!loop_running)
    {
        loop_running = true;
        event_thread = std::thread(event_loop);
    }
}

void event_stop()
{
    if (loop_running)
    {
        loop_running = false;
        if (event_thread.joinable())
        {
            event_thread.join();
        }
    }
}

#endif