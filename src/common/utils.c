#include "utils.h"

uint32_t get_length(uint8_t *buffer)
{
    char *p = buffer;
    uint32_t count = 0;
    while (*p++ != 0)
    {
        count++;
    }
    return count;
}