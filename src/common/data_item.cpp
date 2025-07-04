#include "data_item.h"

#include <malloc.h>
#include <string.h>

data_item::data_item(uint32_t size)
{
    this->size = size;
    if (size > 0)
    {
        data = static_cast<uint8_t *>(calloc(1, size));
    }
    else
    {
        data = NULL;
    }
}

data_item::~data_item()
{
    if (data)
    {
        free(data);
    }
}

data_item* data_item::copy()
{
    data_item* item = new data_item(size);
    memcpy(item->data, data, size);

    return item;
}