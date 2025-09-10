#ifndef _DATA_ITEM_H_
#define _DATA_ITEM_H_

#include <stdint.h>

namespace coloraudio::common
{
    /**
     * 数据存储
     */
    class DataItem
    {
    private:
        /**
         * 数据
         */
        uint8_t *data;
        /**
         * 存储大小
         */
        uint32_t size;

    public:
        /**
         * 创建一个数据存储
         * @param size 存储大小
         */
        DataItem(uint32_t size);
        DataItem(uint8_t *buffer, uint32_t size);
        /**
         * 销毁数据存储
         */
        ~DataItem();
        /**
         * 创建一个副本
         */
        DataItem *copy();
        /**
         * 修改大小
         * @param size 新的大小
         */
        void resize(uint32_t size);

        uint8_t* get_data() const
        {
            return data;
        }

        uint32_t get_size() const
        {
            return size;
        }
    };
}

#endif