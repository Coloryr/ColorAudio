#include "utils.h"
#include "stream.h"

#include <malloc.h>
#include <string.h>
#include <turbojpeg.h>
#include <png.h>

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

typedef enum
{
    UTF16_BE,
    UTF16_LE,
    UTF16_UNKNOWN
} UTF_ORDER;

static UTF_ORDER detect_byte_order(uint16_t input)
{
    if (input == 0xFFFE)
        return UTF16_BE;
    if (input == 0xFEFF)
        return UTF16_LE;
    return UTF16_UNKNOWN;
}

static uint16_t read_utf16_code_unit(uint16_t *ptr, UTF_ORDER order)
{
    uint8_t *ptr1 = (uint8_t *)ptr;
    return (order == UTF16_BE) ? (ptr1[0] << 8) | ptr1[1] : ptr1[0] | (ptr1[1] << 8);
}

uint32_t utf16_to_utf8(uint16_t *input, uint8_t **output, uint32_t size)
{
    uint32_t utf8_index = 0;
    uint32_t utf8_size = 1;
    uint32_t utf16_index = 0;

    UTF_ORDER byte_order = detect_byte_order(input[0]);
    uint32_t i = (byte_order != UTF16_UNKNOWN) ? 1 : 0; // 跳过BOM

    if (byte_order == UTF16_UNKNOWN)
    {
        size_t zero_count = 0;
        for (int j = 0; j < 100 && input[j] != '\0'; j += 2)
        {
            if (input[j] == 0x00)
                zero_count++;
        }
        byte_order = (zero_count > 20) ? UTF16_BE : UTF16_LE;
    }
    else
    {
        utf16_index += 2;
    }

    while (input[i] != '\0' && utf16_index < size)
    {
        uint16_t unicode_code = read_utf16_code_unit(&input[i], byte_order);
        utf16_index += 2;
        if (unicode_code >= 0xD800 && unicode_code <= 0xDBFF && input[i + 1] != '\0')
        {
            uint16_t surrogate_pair_code = read_utf16_code_unit(&input[i + 1], byte_order);
            utf16_index += 2;
            if (surrogate_pair_code >= 0xDC00 && surrogate_pair_code <= 0xDFFF)
            {
                unicode_code = ((unicode_code - 0xD800) << 10) + (surrogate_pair_code - 0xDC00) + 0x10000;
                i += 1;
            }
        }

        if (unicode_code < 0x80)
        {
            utf8_size += 1;
        }
        else if (unicode_code < 0x800)
        {
            utf8_size += 2;
        }
        else if (unicode_code < 0x10000)
        {
            utf8_size += 3;
        }
        else
        {
            utf8_size += 4;
        }

        i += 1;
    }

    *output = (char *)malloc(utf8_size * sizeof(char));

    if (*output == NULL)
    {
        return 0;
    }

    i = (byte_order != UTF16_UNKNOWN) ? 1 : 0; // 跳过BOM
    utf16_index = (byte_order != UTF16_UNKNOWN) ? 2 : 0;

    utf8_index = 0;
    while (input[i] != '\0' && utf16_index < size)
    {
        uint16_t unicode_code = read_utf16_code_unit(&input[i], byte_order);
        utf16_index += 2;
        if (unicode_code >= 0xD800 && unicode_code <= 0xDBFF && input[i + 1] != '\0')
        {
            uint16_t surrogate_pair_code = read_utf16_code_unit(&input[i + 1], byte_order);
            utf16_index += 2;
            if (surrogate_pair_code >= 0xDC00 && surrogate_pair_code <= 0xDFFF)
            {
                unicode_code = ((unicode_code - 0xD800) << 10) + (surrogate_pair_code - 0xDC00) + 0x10000;
                i += 1;
            }
        }
        if (unicode_code < 0x80)
        {
            (*output)[utf8_index++] = unicode_code;
        }
        else if (unicode_code < 0x800)
        {
            (*output)[utf8_index++] = ((unicode_code >> 6) & 0x1F) | 0xC0;
            (*output)[utf8_index++] = (unicode_code & 0x3F) | 0x80;
        }
        else if (unicode_code < 0x10000)
        {
            (*output)[utf8_index++] = ((unicode_code >> 12) & 0x0F) | 0xE0;
            (*output)[utf8_index++] = ((unicode_code >> 6) & 0x3F) | 0x80;
            (*output)[utf8_index++] = (unicode_code & 0x3F) | 0x80;
        }
        else
        {
            (*output)[utf8_index++] = ((unicode_code >> 18) & 0x07) | 0xF0;
            (*output)[utf8_index++] = ((unicode_code >> 12) & 0x3F) | 0x80;
            (*output)[utf8_index++] = ((unicode_code >> 6) & 0x3F) | 0x80;
            (*output)[utf8_index++] = (unicode_code & 0x3F) | 0x80;
        }
        i += 1;
    }
    (*output)[utf8_index++] = '\0';
    return utf8_index;
}

const uint8_t jpg_signature[] = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46};
const uint8_t png_signature[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

static int is_jpg(uint8_t *raw_data, size_t len)
{
    if (len < sizeof(jpg_signature))
        return false;
    return memcmp(jpg_signature, raw_data, sizeof(jpg_signature)) == 0;
}

static int is_png(uint8_t *raw_data, size_t len)
{
    if (len < sizeof(png_signature))
        return false;
    return memcmp(png_signature, raw_data, sizeof(png_signature)) == 0;
}

static void istream_png_reader(png_structp png_ptr, png_bytep png_data, png_size_t data_size)
{
    stream_t *st = (stream_t *)png_get_io_ptr(png_ptr);

    if (stream_test_read_size(st, data_size) == false)
    {
        return;
    }

    stream_read(st, png_data, data_size);
};

bool load_image(uint8_t *data, uint32_t size, lv_image_dsc_t *img_dsc)
{
    if (is_jpg(data, size))
    {
        tjhandle handle = tjInitDecompress();
        if (!handle)
        {
            return false;
        }

        int width = 0, height = 0;
        int res = tjDecompressHeader(handle, data, size, &width, &height);
        if (res != 0)
        {
            tjDestroy(handle);
            return false;
        }

        img_dsc->header.w = width;
        img_dsc->header.h = height;
        img_dsc->data_size = width * height * 3;
        img_dsc->header.stride = width * 3;
        img_dsc->header.cf = LV_COLOR_FORMAT_RGB888;
        img_dsc->data = (uint8_t *)malloc(img_dsc->data_size); // RGB 24bpp
        if (!img_dsc->data)
        {
            tjDestroy(handle);
            return false;
        }

        if (tjDecompress2(handle, data, size, (uint8_t *)img_dsc->data, width, 0, height, TJPF_BGR, 0) != 0)
        {
            tjDestroy(handle);
            return false;
        }

        tjDestroy(handle);

        return true;
    }
    else if (png_sig_cmp(data, 0, 8) == 0)
    {
        png_structp png_ptr = png_create_read_struct(
            PNG_LIBPNG_VER_STRING,
            NULL,
            NULL,
            NULL);
        if (!png_ptr)
        {
            return false;
        }
        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
        {
            png_destroy_read_struct(&png_ptr, NULL, NULL); // 释放已经分配的资源
            return false;
        }

        if (setjmp(png_jmpbuf(png_ptr)))
        {
            LV_LOG_ERROR("Png decode error");
        }
        stream_t *st = stream_create_mem(data, size);
        png_set_read_fn(png_ptr, st, istream_png_reader);

        png_read_info(png_ptr, info_ptr);
        png_uint_32 width = png_get_image_width(png_ptr, info_ptr);
        png_uint_32 height = png_get_image_height(png_ptr, info_ptr);
        png_byte color_type = png_get_color_type(png_ptr, info_ptr);
        png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        if (color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png_ptr); // 调色板转RGB
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
            png_set_expand_gray_1_2_4_to_8(png_ptr); // 灰度位扩展
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
            png_set_tRNS_to_alpha(png_ptr); // 透明度通道支持
        if (bit_depth == 16)
            png_set_strip_16(png_ptr); // 16位->8位
        if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png_ptr); // 灰度转RGB
        if (!(color_type & PNG_COLOR_MASK_ALPHA))
            png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER); // 添加不透明Alpha通道
        // png_set_swap_alpha(png_ptr);                            // RGBA -> ARGB
        png_set_bgr(png_ptr);

        png_read_update_info(png_ptr, info_ptr); // 更新格式信息

        img_dsc->header.w = width;
        img_dsc->header.h = height;
        img_dsc->data_size = width * height * 4;
        img_dsc->header.stride = width * 4;
        img_dsc->header.cf = LV_COLOR_FORMAT_ARGB8888;
        img_dsc->data = (png_bytep)malloc(img_dsc->data_size);
        if (!img_dsc->data)
        {
            LV_LOG_ERROR("Png decode error");
            stream_close(st);
            png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            return false;
        }

        png_bytepp row_pointers = malloc(height * sizeof(png_bytep));
        for (png_uint_32 y = 0; y < height; y++)
        {
            row_pointers[y] = (uint8_t *)img_dsc->data + y * width * 4;
        }

        png_read_image(png_ptr, row_pointers);
        png_read_end(png_ptr, NULL);

        free(row_pointers);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        stream_close(st);

        return true;
    }

    return false;
}