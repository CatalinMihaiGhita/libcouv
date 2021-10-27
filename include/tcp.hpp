// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <coroutine>
#include <iostream>
#include <memory>

#include <uv.h> // libuv
#include <reader.hpp>
#include <writer.hpp>
#include <listner.hpp>
#include <connector.hpp>
#include <expect.hpp>

namespace couv
{
    class tcp
    {
        std::shared_ptr<uv_tcp_t> socket;
        friend class connector;
        friend class listner;
        friend class reader;
        friend class writer;

        struct tcp_deleter
        {
            void operator()(void* p) const {
                uv_close(static_cast<uv_handle_t*>(p), [](uv_handle_t* req) {
                    delete reinterpret_cast<uv_tcp_t*>(req);
                });
            }
        };

    public:
        tcp() : socket{new uv_tcp_t, tcp_deleter{}}
        {
            uv_tcp_init(uv_default_loop(), socket.get());
        }

        tcp(const tcp&) = delete;
        tcp& operator=(const tcp&) = delete;
        tcp(tcp&&) = default;
        tcp& operator=(tcp&&) = default;
        
        error_code accept(tcp& client) noexcept
        {
            return uv_accept(reinterpret_cast<uv_stream_t*>(socket.get()), 
                reinterpret_cast<uv_stream_t*>(client.socket.get()));
        }

        error_code bind(std::string ip, int port) noexcept
        {
            struct sockaddr_in bind_addr;
            uv_ip4_addr(ip.c_str(), port, &bind_addr);
            
            return uv_tcp_bind(socket.get(), reinterpret_cast<sockaddr*>(&bind_addr), 0);
        }

        listner listen(int backlog)
        {
            return listner{*this, backlog};
        }

        connector connect(const char* ip, int port) {
            return connector{*this, ip, port};
        }

        connector connect(const getaddrinfo& info) {
            return connector{*this, info};
        }


        reader read() {
            reader r{*this};
            r.start();
            return r;
        }

        writer write(std::string data)
        {
            writer w{*this};
            w.write(std::move(data));
            return w;
        }
    };

    connector::connector(const tcp& tcp, const char* ip, int port) : 
        data{std::make_unique<connector_data>()}
    {
        data->req.data = data.get();
        struct sockaddr_in client_addr;
        uv_ip4_addr(ip, port, &client_addr);

        data->status = uv_tcp_connect(&data->req, tcp.socket.get(), reinterpret_cast<sockaddr*>(&client_addr),
            [](uv_connect_t* req, int status) {
                auto* data = static_cast<connector_data*>(req->data);
                data->status = status;
                [[likely]] if (data->co_handle) {
                    data->co_handle();
                }
            });

        if (data->status == 0) {
            data->status = 1;
        }
    }

    connector::connector(const tcp& tcp, const getaddrinfo& info) : 
        data{std::make_unique<connector_data>()}
    {
        data->req.data = data.get();
        data->status = uv_tcp_connect(&data->req, tcp.socket.get(), (const struct sockaddr*)info.data->getaddrinfo_handle.addrinfo->ai_addr,
            [](uv_connect_t* req, int status) {
                auto* data = static_cast<connector_data*>(req->data);
                data->status = status;
                [[likely]] if (data->co_handle) {
                    data->co_handle();
                }
            });

        if (data->status == 0) {
            data->status = 1;
        }
    }

    listner::listner(const tcp& tcp, int backlog) :
        stream{std::reinterpret_pointer_cast<uv_stream_t>(tcp.socket)},
        ready{false}
    {
        stream->data = this;
        uv_listen(stream.get(), backlog, [](uv_stream_t* req, int status) {
            auto self = static_cast<listner*>(req->data);
            self->status = status;
            self->ready = true;
            if (self->co_handle)
                self->co_handle();
        });
    }

    reader::reader(const tcp& tcp) : stream{std::reinterpret_pointer_cast<uv_stream_t>(tcp.socket)}
    {
        stream->data = this;
    }

    writer::writer(const tcp& tcp) : 
        data{new writer_data{std::reinterpret_pointer_cast<uv_stream_t>(tcp.socket)}} 
    {
        data->write_handle.data = data.get();
    }
}