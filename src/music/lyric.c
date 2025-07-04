#include "lyric.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

static void insert_sorted(lyric_node_t **head, lyric_node_t *node)
{
    // 头节点为空或新节点时间最小
    if (*head == NULL || node->timestamp < (*head)->timestamp)
    {
        node->next = *head;
        *head = node;
        return;
    }

    // 遍历找到插入位置
    lyric_node_t *cur = *head;
    while (cur->next && cur->next->timestamp <= node->timestamp)
    {
        cur = cur->next;
    }
    node->next = cur->next;
    cur->next = node;
}

lyric_node_t *parse_memory_lrc(char *data)
{
    lyric_node_t *head = NULL;
    char *line = strtok(data, "\n");

    while (line)
    {
        if (strlen(line) < 5)
        {
            line = strtok(NULL, "\n");
            continue;
        }

        if (line[0] == '[' && isdigit(line[1]))
        {
            int mm, ss, ms;
            if (sscanf(line, "[%d:%d.%d]", &mm, &ss, &ms) == 3)
            {
                long timestamp = mm * 60000 + ss * 1000 + ms;

                char *lyric_start = strchr(line, ']') + 1;
                if (*lyric_start == ' ')
                    lyric_start++;

                lyric_node_t *node = malloc(sizeof(lyric_node_t));
                node->timestamp = timestamp;
                node->lyric = strdup(lyric_start);
                node->next = NULL;

                insert_sorted(&head, node);
            }
        }
        line = strtok(NULL, "\n");
    }
    return head;
}

char *lyric_find(lyric_node_t *head, uint32_t current_time)
{
    lyric_node_t *cur = head;

    while (cur && cur->next && cur->next->timestamp <= current_time)
    {
        cur = cur->next;
    }

    if (cur == NULL)
    {
        return NULL;
    }
    else if (current_time >= cur->timestamp)
    {
        return cur->lyric;
    }
    else
    {
        return NULL;
    }
}

void lyric_close(lyric_node_t *head)
{
    while (head)
    {
        lyric_node_t *temp = head;
        head = head->next;
        free(temp->lyric);
        free(temp);
    }
}