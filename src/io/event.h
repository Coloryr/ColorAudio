#ifndef __EVENT_H__
#define __EVENT_H__

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <mutex>

#ifdef BUILD_ARM

class InputDeviceListener
{
public:
    InputDeviceListener(const char *device_path);
    ~InputDeviceListener();

    int fd;
    std::string device_path;
    bool running;

    void read_event();
};

extern bool headphone_1_in;
extern bool headphone_2_in;

void event_init();
void event_stop();

#endif

#endif // __EVENT_H__
