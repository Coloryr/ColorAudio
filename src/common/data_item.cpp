#include <malloc.h>
#include <string.h>

#include "data_item.h"

using namespace coloraudio::common;

DataItem::DataItem(uint32_t size)
{
    if (size == 0)
    {
        throw "size is zero";
    }
    this->size = size;
    data = static_cast<uint8_t *>(calloc(1, size));
}

DataItem::DataItem(uint8_t *buffer, uint32_t size)
{
    if (size == 0)
    {
        throw "size is zero";
    }
    this->size = size;
    data = static_cast<uint8_t *>(malloc(size));
    memcpy(data, buffer, size);
}

DataItem::~DataItem()
{
    if (data)
    {
        free(data);
    }
}

DataItem *DataItem::copy()
{
    return new DataItem(data, size);
}

void DataItem::resize(uint32_t size)
{
    void *temp = realloc(data, size);
    if (temp == NULL)
    {
        throw "resize is zero";
    }
    this->size = size;
    data = static_cast<uint8_t *>(temp);
}