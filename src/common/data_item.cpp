#include "data_item.h"

#include <malloc.h>

DataItem::DataItem(uint32_t size)
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

DataItem::~DataItem()
{
    if (this->data)
    {
        free(this->data);
    }
}
