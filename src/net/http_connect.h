#ifndef _HTTP_CONNECT_H_
#define _HTTP_CONNECT_H_

#include "data_item.h"
#include "stream.h"
#include "stream_buffer.h"

#include <curl/curl.h>

#define HTTP_HEADER_CONTENT_JSON "Content-Type: application/json"

typedef size_t (*http_write_callback)(void *ptr, size_t size, size_t nmemb, void *userdata);

typedef struct
{
    CURL *curl;
    struct curl_slist *header_list;
} http_connect_item_t;

typedef struct
{
    http_connect_item_t *connect;
    curl_socket_t sockfd;
    stream_http_i* curl_cb;
} curl_steam_t;

typedef struct {
    char *protocol; // 协议（http/https）
    char *host;     // 主机名
    int port;       // 端口
    char *path;     // 路径
    char *query;    // 查询参数
} url_components_t;

void http_init();

http_connect_item_t *http_create_connect();

uint8_t *http_encode_param(http_connect_item_t *connect, uint8_t *data);

void http_url(http_connect_item_t *connect, uint8_t *url);
void http_add_header(http_connect_item_t *connect, uint8_t *data);
void http_post(http_connect_item_t *connect, uint8_t *data, uint32_t size);
void http_get(http_connect_item_t *connect);
void http_free_data(uint8_t *data);
void http_free(http_connect_item_t *connect);

void http_start_stream(http_connect_item_t *connect, uint8_t *url, stream_t **stream);

void http_start_data(http_connect_item_t *connect, data_item_t **data);

bool http_run(http_connect_item_t *connect);

#endif