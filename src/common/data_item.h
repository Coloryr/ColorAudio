#ifndef _DATA_ITEM_H_
#define _DATA_ITEM_H_

#include <stdint.h>

/**
 * 数据存储
 */
class data_item
{
private:
    
public:
    /**
     * 数据
     */
    uint8_t *data;
    /**
     * 存储大小
     */
    uint32_t size;

    /**
     * 创建一个数据存储
     * @param size 存储大小
     */
    data_item(uint32_t size);
    /**
     * 销毁数据存储
     */
    ~data_item();
    /**
     * 创建一个副本
     */
    data_item* copy();
};

#endif