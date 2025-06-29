#ifndef _LYRIC_H_
#define _LYRIC_H_

#include <stdint.h>

typedef struct lyric_node_t
{
    uint32_t timestamp;
    uint8_t *lyric;
    struct lyric_node_t *next;
} lyric_node_t;

lyric_node_t *parse_memory_lrc(uint8_t *data);
uint8_t *lyric_find(lyric_node_t *head, uint32_t current_time);
void lyric_close(lyric_node_t *head);

#endif