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
        
        int accept(tcp& client)
        {
            return uv_accept(reinterpret_cast<uv_stream_t*>(socket.get()), 
                reinterpret_cast<uv_stream_t*>(client.socket.get()));
        }

        expect<void> bind(std::string ip, int port) noexcept
        {
            struct sockaddr_in bind_addr;
            uv_ip4_addr(ip.c_str(), port, &bind_addr);
            
            if (auto err = uv_tcp_bind(socket.get(), reinterpret_cast<sockaddr*>(&bind_addr), 0))
                return std::make_exception_ptr(err);
            return {};
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
        req{std::make_unique<uv_connect_t>()}
    {
        req->data = this;
        struct sockaddr_in client_addr;
        uv_ip4_addr(ip, port, &client_addr);

        status = uv_tcp_connect(req.get(), tcp.socket.get(), reinterpret_cast<sockaddr*>(&client_addr),
            [](uv_connect_t* req, int status) {
                auto* self = static_cast<connector*>(req->data);
                if (!self) {
                    delete req;
                    return;
                }
                self->status = status;
                if (self->co_handle) {
                    self->co_handle();
                }
            });

        if (!status) {
            status = 1;
        }
    }

    connector::connector(const tcp& tcp, const getaddrinfo& info) : 
        req{std::make_unique<uv_connect_t>()}
    {
        req->data = this;
        status = uv_tcp_connect(req.get(), tcp.socket.get(), (const struct sockaddr*)info.getaddrinfo_handle->addrinfo->ai_addr,
            [](uv_connect_t* req, int status) {
                auto* self = static_cast<connector*>(req->data);
                if (!self) {
                    delete req;
                    return;
                }
                self->status = status;
                if (self->co_handle) {
                    self->co_handle();
                }
            });

        if (!status) {
            status = 1;
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

    reader::reader(const tcp& tcp) :
        stream{std::reinterpret_pointer_cast<uv_stream_t>(tcp.socket)},
        nread{0}
    {
        stream->data = this;
    }

    writer::writer(const tcp& tcp) : 
        write_handle{std::make_unique<uv_write_t>()}, 
        stream{std::reinterpret_pointer_cast<uv_stream_t>(tcp.socket)},
        status{1}
    {
            write_handle->data = this;
    }
}