#ifndef _HTTP_CONNECT_H_
#define _HTTP_CONNECT_H_

#include "../stream/stream_http.h"
#include "../common/data_item.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/system/error_code.hpp>
#include <string>

#define HTTP_HEADER_CONTENT_JSON "Content-Type: application/json"
#define HTTP_USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36 Edg/138.0.0.0"
#define HTTP_ACCEPT "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7"
#define HTTP_CACHE_CONTROL "max-age=0"

typedef struct
{
    std::string protocol;
    std::string host;
    std::string port;
    std::string target;
} parsed_url_t;

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace asio = boost::asio;
namespace ip = boost::asio::ip;

namespace ColorAudio
{
    class HttpStream : public IStreamHttp
    {
    private:
        parsed_url_t parsed;
        asio::io_context ioc;
        beast::error_code ec;
        http::request<http::empty_body> req;
        bool is_https = false;
        uint32_t size;

        ip::tcp::resolver *resolver;
        asio::ssl::context *ctx;
        beast::ssl_stream<beast::tcp_stream> *sslstream;
        beast::tcp_stream *stream;
        beast::flat_buffer *http_buffer;
        beast::http::response_parser<http::buffer_body> *parser;

    public:
        HttpStream(std::string &url);
        ~HttpStream();

        bool connect();
        void disconnect();
        void close();
        uint32_t get_size();
        uint32_t read(uint8_t *buffer, uint32_t len);
        uint32_t re_connect(uint32_t pos);
    };
}

std::string http_get_string(const std::string &url);
data_item *http_get_data(const std::string &url);

#endif