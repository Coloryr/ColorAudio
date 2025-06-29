#include "stream.h"
#include "utils.h"

#include "lvgl/src/misc/lv_log.h"

#include <malloc.h>
#include <string.h>
#include <pthread.h>

Stream::Stream(steam_type type)
{
    this->type = type;
}

Stream::~Stream()
{
}

uint8_t Stream::read_byte()
{
    uint8_t byte;
    if (this->read(&byte, 1) == 1)
    {
        return byte;
    }

    return 0;
}
