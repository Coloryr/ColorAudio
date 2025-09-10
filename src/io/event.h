#ifndef __EVENT_H__
#define __EVENT_H__

#ifdef BUILD_ARM

#include <string>

enum KEY_EVENT
{
    KEY_0 = 0b000001,
    KEY_1 = 0b000010,
    KEY_2 = 0b000100,
    KEY_3 = 0b001000,
    KEY_POWER = 0b010000,
    KEY_DOWN  = 0b100000,
    KEY_UNKNOW = -1
};

enum ROTATE_EVENT
{
    ROTATE_LEFT,
    ROTATE_RIGHT,
    ROTATE_UNKNOW
};

namespace coloraudio::io::event
{
    class InputDeviceListener
    {
    private:
        int fd;
        std::string device_path;
        bool is_run;

    public:
        InputDeviceListener(const char *device);
        ~InputDeviceListener();

        void read_event();

        const bool can_run()
        {
            return is_run;
        }
    };
}

extern bool headphone_1_in;
extern bool headphone_2_in;
extern KEY_EVENT key_data;

void event_init();
void event_stop();

#endif

#endif // __EVENT_H__
