#include "http_connect.h"

#include "stream.h"
#include "data_item.h"

#include "lvgl/src/misc/lv_log.h"

#include <curl/curl.h>
#include <stdint.h>
#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

static void close(void *data)
{
    curl_steam_t *curl = (curl_steam_t *)data;
    free(curl->curl_cb);

    http_free(curl->connect);
}

static uint32_t get_size(void *data)
{
    curl_steam_t *curl = (curl_steam_t *)data;
}
static uint32_t read(void *data, uint8_t *buffer, uint32_t len)
{
    curl_steam_t *curl = (curl_steam_t *)data;

    uint64_t nread;
    CURLcode res = curl_easy_recv(curl, buffer, len, &nread);

    return nread;
}
static uint32_t re_connect(void *data, uint32_t pos)
{
    curl_steam_t *curl = (curl_steam_t *)data;
}

static size_t write_callback_stream(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t total_size = size * nmemb;
    stream_t *stream = (stream_t *)userdata;

    stream_write(stream, ptr, total_size);
    return total_size;
}

static size_t write_callback_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t total_size = size * nmemb;
    data_item_t *data = (data_item_t *)userdata;
    if (data->data == NULL)
    {
        data->data = malloc(total_size + 1);
    }
    else
    {
        data->data = realloc(data->data, data->size + total_size + 1);
    }
    memcpy(data->data + data->size, ptr, total_size);
    data->size += total_size;
    data->data[data->size] = '\0';

    return total_size;
}

static url_components_t parse_url(uint8_t *url)
{
    url_components_t comp = {NULL, NULL, 0, NULL, NULL};
    char *temp = strdup(url); // 复制字符串避免修改原数据

    // 提取协议（如"http"）
    char *proto_end = strstr(temp, "://");
    if (proto_end)
    {
        *proto_end = '\0';
        comp.protocol = strdup(temp);
        temp = proto_end + 3; // 跳过"://"
    }
    else
    {
        comp.protocol = strdup("http"); // 默认协议
    }

    // 提取主机和端口
    char *host_end = strpbrk(temp, ":/?#"); // 查找分隔符
    if (host_end)
    {
        *host_end = '\0';
        comp.host = strdup(temp);
        temp = host_end + 1;
    }
    else
    {
        comp.host = strdup(temp);
        temp = NULL;
    }

    // 解析端口（如":8080"）
    char *port_ptr = strchr(comp.host, ':');
    if (port_ptr)
    {
        *port_ptr = '\0';
        comp.port = atoi(port_ptr + 1);
    }
    else
    {
        comp.port = (strcmp(comp.protocol, "https") == 0) ? 443 : 80; // 默认端口
    }

    // 提取路径和查询参数
    if (temp)
    {
        char *query_ptr = strchr(temp, '?');
        if (query_ptr)
        {
            *query_ptr = '\0';
            comp.path = strdup(temp);
            comp.query = strdup(query_ptr + 1);
        }
        else
        {
            comp.path = strdup(temp);
        }
    }
    else
    {
        comp.path = strdup("/"); // 默认路径
    }

    free(temp);
    return comp;
}

// 释放内存
static void free_url_components(url_components_t comp)
{
    free(comp.protocol);
    free(comp.host);
    free(comp.path);
    free(comp.query);
}

static void generate_http_headers(url_components_t comp, uint8_t *data)
{
    int size = sprintf(data, "GET %s%s%s HTTP/1.1\r\n",
                       comp.path,
                       comp.query ? "?" : "",
                       comp.query ? comp.query : "");

    size = sprintf(data + size, "Host: %s", comp.host);
    if ((comp.port != 80) && (comp.port != 443))
    {
        size = sprintf(data + size, ":%d", comp.port);
    }
    size = sprintf(data + size, "\r\n");

    size = sprintf(data + size, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36 Edg/138.0.0.0\r\n");
    size = sprintf(data + size, "Accept: */*\r\n");

    // size = sprintf(data + size, "Connection: close\r\n");

    // size = sprintf(data + size, "Content-Type: application/json\r\n");
    // size = sprintf(data + size, "Content-Length: %d\r\n", body_len);

    size = sprintf(data + size, "\r\n");
}

void http_init()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

http_connect_item_t *http_create_connect()
{
    http_connect_item_t *connect = calloc(1, sizeof(http_connect_item_t));
    connect->curl = curl_easy_init();

    curl_easy_setopt(connect->curl, CURLOPT_SSL_VERIFYPEER, 1L); // 验证证书
    curl_easy_setopt(connect->curl, CURLOPT_FOLLOWLOCATION, 1L); // 自动跟随重定向
    curl_easy_setopt(connect->curl, CURLOPT_VERBOSE, 1L);        // 打印详细通信日志

    return connect;
}

void http_url(http_connect_item_t *connect, uint8_t *url)
{
    curl_easy_setopt(connect->curl, CURLOPT_URL, url);
}

void http_add_header(http_connect_item_t *connect, uint8_t *data)
{
    connect->header_list = curl_slist_append(connect->header_list, data);
    curl_easy_setopt(connect->curl, CURLOPT_HTTPHEADER, connect->header_list);
}

void http_post(http_connect_item_t *connect, uint8_t *data, uint32_t size)
{
    curl_easy_setopt(connect->curl, CURLOPT_POST, 1);
    curl_easy_setopt(connect->curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(connect->curl, CURLOPT_POSTFIELDSIZE, size);
}

void http_get(http_connect_item_t *connect)
{
    curl_easy_setopt(connect->curl, CURLOPT_POST, 0);
}

void http_start_stream(http_connect_item_t *connect, uint8_t *url, stream_t **stream)
{
    curl_steam_t *curl_st = malloc(sizeof(curl_steam_t));
    curl_st->connect = connect;
    curl_st->curl_cb = malloc(sizeof(stream_http_i));
    curl_st->curl_cb->data = curl_st;
    curl_st->curl_cb->close = close;
    curl_st->curl_cb->get_size = get_size;
    curl_st->curl_cb->read = read;
    curl_st->curl_cb->re_connect = re_connect;

    curl_easy_setopt(connect->curl, CURLOPT_TIMEOUT, 0);
    curl_easy_setopt(connect->curl, CURLOPT_CONNECT_ONLY, 1L);

    url_components_t com = parse_url(url);

    uint8_t req[1024];

    generate_http_headers(com, req);

    size_t request_len = strlen(req);

    CURLcode res = curl_easy_perform(connect->curl);

    if (res != CURLE_OK)
    {
        LV_LOG_ERROR("Error: %s\n", curl_easy_strerror(res));
        return;
    }

    if (res == CURLE_OK)
    {
        curl_socket_t sockfd;

        /* Extract the socket from the curl handle - we need it for waiting. */
        res = curl_easy_getinfo(connect->curl, CURLINFO_ACTIVESOCKET, &sockfd);

        curl_st->sockfd = sockfd;
    }

    stream_t *st = stream_create_http(curl_st->curl_cb);
    *stream = st;
}

void http_start_data(http_connect_item_t *connect, data_item_t **data)
{
    data_item_t *DataItem = data_item_create(0);
    *data = DataItem;
    curl_easy_setopt(connect->curl, CURLOPT_TIMEOUT, 20);
    curl_easy_setopt(connect->curl, CURLOPT_WRITEFUNCTION, write_callback_data);
    curl_easy_setopt(connect->curl, CURLOPT_WRITEDATA, DataItem);
}

bool http_run(http_connect_item_t *connect)
{
    return curl_easy_perform(connect->curl) == CURLE_OK;
}

uint8_t *http_encode_param(http_connect_item_t *connect, uint8_t *data)
{
    return curl_easy_escape(connect->curl, data, 0);
}

void http_free_data(uint8_t *data)
{
    curl_free(data);
}

void http_free(http_connect_item_t *connect)
{
    if (connect->curl)
    {
        curl_easy_cleanup(connect->curl);
    }
    if (connect->header_list)
    {
        curl_slist_free_all(connect->header_list);
    }

    free(connect);
}