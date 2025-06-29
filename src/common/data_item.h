#ifndef _DATA_ITEM_H_
#define _DATA_ITEM_H_

#include <stdint.h>

class DataItem
{
private:
    
public:
    uint8_t *data;
    uint32_t size;

    DataItem(uint32_t size);
    ~DataItem();
};

#endif