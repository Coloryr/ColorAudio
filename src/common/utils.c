#include "utils.h"
#include "stream.h"

#include <malloc.h>
#include <string.h>

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