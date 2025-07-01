#include "lyric.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

lyric_node_t *lyric_head; // 链表头节点
long global_offset = 0;   // 时间偏移量（从元数据解析）

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
    char *line = strtok(data, "\n"); // 按换行符分割

    while (line)
    {
        // 跳过空行
        if (strlen(line) < 5)
        {
            line = strtok(NULL, "\n");
            continue;
        }

        // 解析时间戳
        if (line[0] == '[' && isdigit(line[1]))
        {
            int mm, ss, ms;
            if (sscanf(line, "[%d:%d.%d]", &mm, &ss, &ms) == 3)
            {
                long timestamp = mm * 60000 + ss * 1000 + ms;

                // 定位歌词起始位置
                char *lyric_start = strchr(line, ']') + 1;
                if (*lyric_start == ' ')
                    lyric_start++; // 跳过空格

                // 创建节点
                lyric_node_t *node = malloc(sizeof(lyric_node_t));
                node->timestamp = timestamp;
                node->lyric = strdup(lyric_start); // 深拷贝
                node->next = NULL;

                // 有序插入链表
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

    // 查找当前歌词
    while (cur && cur->next && cur->next->timestamp <= current_time)
    {
        cur = cur->next;
    }

    if(cur == NULL)
    {
        return NULL;
    }
    else if(current_time >= cur->timestamp)
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
        free(temp->lyric); // 释放深拷贝的字符串
        free(temp);
    }
}