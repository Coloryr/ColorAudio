#include "data_item.h"

#include <malloc.h>

data_item::data_item(uint32_t size)
{
    this->size = size;
    if (this->size > 0)
    {
        this->data = static_cast<uint8_t *>(calloc(1, size));
    }
    else
    {
        this->data = NULL;
    }
}

data_item::~data_item()
{
    if (this->data)
    {
        free(this->data);
    }
}
