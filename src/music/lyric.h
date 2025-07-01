#ifndef _LYRIC_H_
#define _LYRIC_H_

#include <stdint.h>

typedef struct lyric_node_t
{
    uint32_t timestamp;
    char *lyric;
    struct lyric_node_t *next;
} lyric_node_t;

#ifdef __cplusplus
extern "C" {
#endif

lyric_node_t *parse_memory_lrc(char *data);
char *lyric_find(lyric_node_t *head, uint32_t current_time);
void lyric_close(lyric_node_t *head);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif