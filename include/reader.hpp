// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <coroutine>
#include <iostream>
#include <memory>
#include <error_code.hpp>

#include <uv.h> // libuv

namespace couv
{
    class tcp;
    class reader
    {
        std::shared_ptr<uv_stream_t> stream;
        std::string buf;
        ssize_t nread;
        std::coroutine_handle<> co_handle;
    public:
        reader(const tcp&);

        reader(const reader&) = delete;
        reader(reader&& r) : 
            stream{std::move(r.stream)}, 
            buf(std::move(r.buf)),
            nread(std::move(r.nread)),
            co_handle(std::move(r.co_handle))
        {
            stream->data = this;
        }

        void stop()
        {
            uv_read_stop(stream.get());
        }

        error_code start() noexcept
        {
            return uv_read_start(stream.get(),
                [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
                    auto self = static_cast<reader*>(handle->data);
                    self->buf.resize(suggested_size);
                    *buf = uv_buf_init(self->buf.data(), suggested_size);
                },
                [](uv_stream_t* req, ssize_t nread, const uv_buf_t* buf) {
                    auto self = static_cast<reader*>(req->data);
                    self->nread = nread;
                    if (self->co_handle)
                        self->co_handle();
                });
        }
        
        bool await_ready() const { return nread > 0; }
        void await_suspend(std::coroutine_handle<> h) { co_handle = h; } 
        const char* await_resume() 
        {
            const char* out = nullptr;
            if (nread > 0) {
                out = buf.c_str();
                nread = 0;
            } 
            co_handle = nullptr; 
            return out;
        }

        ~reader()
        {
            if (stream) {
                uv_read_stop(stream.get());
            }
        }
    };   
}