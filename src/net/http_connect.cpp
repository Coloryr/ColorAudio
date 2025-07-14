#include "http_connect.h"

#include "../stream/stream.h"
#include "../common/data_item.h"

#include "../lvgl/src/misc/lv_log.h"

#include <string>
#include <regex>
#include <iomanip>
#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <boost/url.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/system/error_code.hpp>
#include <boost/algorithm/string.hpp>

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace asio = boost::asio;
namespace ip = boost::asio::ip;
namespace urls = boost::urls;
namespace ssl = boost::asio::ssl;

parsed_url_t parse_url(const std::string &url_str)
{
    parsed_url_t parsed;

    auto path = url_str;
    std::string params;
    auto paramsPos = url_str.find("?");
    if (paramsPos != std::string::npos)
    {
        path = url_str.substr(0, paramsPos);
        params = url_str.substr(paramsPos + 1);
    }

    boost::system::result<urls::url_view, boost::system::error_code> uv = urls::parse_uri(path);

    if (!uv.has_error())
    {
        auto uv1 = uv.value();
        parsed.protocol = uv1.scheme();
        parsed.host = uv1.host();
        parsed.port = uv1.port().empty() ? (parsed.protocol == "https" ? "443" : "80") : uv1.port();

        std::string path = uv1.path();
        if (path.empty())
            path = "/";

        parsed.target = path;
    }

    if (!params.empty())
    {
        std::vector<std::string> substr;
        boost::split(substr, params, boost::is_any_of("&"));

        params.resize(params.size() * 3);
        params.clear();
        for (const auto &s : substr)
        {
            if (!s.empty())
            {
                auto eqpos = s.find('=');
                if (eqpos != std::string::npos)
                {
                    auto key = s.substr(0, eqpos);
                    auto value = s.substr(eqpos + 1);
                    params += urls::encode(key, urls::unreserved_chars) + "=" + urls::encode(value, urls::unreserved_chars) + "&";
                }
            }
        }

        if (!params.empty())
        {
            params.resize(params.size() - 1);
            parsed.target += "?" + params;
        }
    }

    return parsed;
}

void http_get_impl(const parsed_url_t &parsed, http::response<http::dynamic_body> &res)
{
    asio::io_context ioc;
    beast::error_code ec;

    ip::tcp::resolver resolver(ioc);

    auto read_response = [&](auto &stream)
    {
        http::request<http::empty_body> req{http::verb::get, parsed.target, 11};
        req.set(http::field::host, parsed.host);
        req.set(http::field::user_agent, HTTP_USER_AGENT);
        http::write(stream, req);

        beast::flat_buffer buffer;

        http::read(stream, buffer, res);
    };

    if (parsed.protocol == "https")
    {
        ssl::context ctx(ssl::context::tlsv13_client);
        ctx.set_default_verify_paths();
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        SSL_set_tlsext_host_name(stream.native_handle(), parsed.host.c_str());

        auto const results = resolver.resolve(parsed.host, parsed.port);
        beast::get_lowest_layer(stream).connect(results);
        stream.handshake(ssl::stream_base::client);

        read_response(stream);
        stream.shutdown(ec);
    }
    else
    {
        beast::tcp_stream stream(ioc);
        auto const results = resolver.resolve(parsed.host, parsed.port);
        stream.connect(results);

        read_response(stream);
        stream.socket().shutdown(ip::tcp::socket::shutdown_both, ec);
    }
}

std::string http_get_string(const std::string &url)
{
    http::response<http::dynamic_body> res;
    try
    {
        auto parsed = parse_url(url);
        http_get_impl(parsed, res);
        auto data = res.body().data();
        return beast::buffers_to_string(data);
    }
    catch (const std::exception &e)
    {
        LV_LOG_ERROR("%s", e.what());
        return "";
    }
}

data_item *http_get_data(const std::string &url)
{
    http::response<http::dynamic_body> res;
    try
    {
        auto parsed = parse_url(url);
        http_get_impl(parsed, res);
        auto data = res.body().data();
        data_item *item = new data_item(data.buffer_bytes());
        asio::buffer_copy(asio::buffer(item->data, item->size), data);
        return item;
    }
    catch (const std::exception &e)
    {
    }

    return NULL;
}

using namespace ColorAudio;

HttpStream::HttpStream(std::string &url)
{
    parsed = parse_url(url);

    resolver = new ip::tcp::resolver(ioc);
    req = {http::verb::get, parsed.target, 11};
    req.set(http::field::host, parsed.host);
    // req.set(http::field::keep_alive, "true");
    req.set(http::field::user_agent, HTTP_USER_AGENT);
    req.set(http::field::accept, HTTP_USER_AGENT);
    req.set(http::field::cache_control, HTTP_CACHE_CONTROL);

    http_buffer = new beast::flat_buffer();
    parser = new http::response_parser<http::buffer_body>();
    parser->body_limit(boost::none);

    if (parsed.protocol == "https")
    {
        ctx = new ssl::context(ssl::context::tlsv12_client);
        ctx->set_default_verify_paths();
        sslstream = new beast::ssl_stream<beast::tcp_stream>(ioc, *ctx);

        SSL_set_tlsext_host_name(sslstream->native_handle(), parsed.host.c_str());
        is_https = true;
    }
    else
    {
       stream = new beast::tcp_stream(ioc);
    }
}

HttpStream::~HttpStream()
{
    close();
}

bool HttpStream::connect()
{
    auto const results = resolver->resolve(parsed.host, parsed.port);

    if (is_https)
    {
        beast::get_lowest_layer(*sslstream).connect(results);
        sslstream->handshake(ssl::stream_base::client);
        http::write(*sslstream, req);
        http::read_header(*sslstream, *http_buffer, *parser, ec);
    }
    else
    {
        stream->connect(results);
        http::write(*stream, req);
        http::read_header(*stream, *http_buffer, *parser, ec);
    }

    if (ec)
    {
        LV_LOG_ERROR("%s", ec.message().c_str());
        return false;
    }

    http::response<http::buffer_body> &res = (*parser).get();

    auto item = res[http::field::content_length];
    if (!item.empty())
    {
        size = atoi(item.data());
    }

    return true;
}

void HttpStream::disconnect()
{
    if (sslstream)
    {
        sslstream->shutdown(ec);
    }
    if (stream)
    {
        stream->socket().shutdown(ip::tcp::socket::shutdown_both, ec);
    }
}

void HttpStream::close()
{
    if (sslstream)
    {
        sslstream->shutdown(ec);
        delete sslstream;
        sslstream = NULL;
    }
    if (ctx)
    {
        delete ctx;
        ctx = NULL;
    }

    if (stream)
    {
        stream->socket().shutdown(ip::tcp::socket::shutdown_both, ec);
        delete stream;
        stream = NULL;
    }

    if (http_buffer)
    {
        delete http_buffer;
        http_buffer = NULL;
    }

    if (parser)
    {
        delete parser;
        parser = NULL;
    }

    if (resolver)
    {
        delete resolver;
        resolver = NULL;
    }
}

uint32_t HttpStream::get_size()
{
    return size;
}

uint32_t HttpStream::read(uint8_t *buffer, uint32_t len)
{
    if (!parser->is_done())
    {
        parser->get().body().data = buffer;
        parser->get().body().size = len;

        if (is_https)
        {
            http::read(*sslstream, *http_buffer, *parser, ec);
        }
        else
        {
            http::read(*stream, *http_buffer, *parser, ec);
        }
        if (ec == http::error::need_buffer)
        {
            ec = {};
            return len - parser->get().body().size;
        }

        if (ec)
            throw beast::system_error{ec};

        return len - parser->get().body().size;
    }

    return 0;
}

uint32_t HttpStream::re_connect(uint32_t pos)
{
    return 0;
}