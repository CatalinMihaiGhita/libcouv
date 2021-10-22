// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <coroutine>
#include <iostream>
#include <memory>

#include <uv.h> // libuv

namespace couv
{
    class tcp;
    class writer
    {
        std::shared_ptr<uv_stream_t> stream;
        std::unique_ptr<uv_write_t> write_handle;
        std::string to_write;
        std::coroutine_handle<> co_handle;
        int status;

    public:
        writer(const tcp&);

        writer(const writer&) = delete;
        writer& operator=(const writer&) = delete;

        writer(writer&& r) : 
            stream{std::move(r.stream)}, 
            write_handle{std::move(r.write_handle)},
            to_write{std::move(r.to_write)},
            co_handle{std::move(r.co_handle)},
            status{r.status}
        {
            write_handle->data = this;
        }

        writer& operator=(writer&& r) 
        {
            stream = std::move(r.stream); 
            write_handle = std::move(r.write_handle);
            to_write = std::move(r.to_write);
            co_handle = std::move(r.co_handle);
            status = r.status;
            write_handle->data = this;
            return *this;
        }

        writer& write(std::string data)
        {
            to_write = std::move(data);
            uv_buf_t buf = uv_buf_init(to_write.data(), to_write.size());
            status = 2;
            uv_write(write_handle.get(), stream.get(), &buf, 1,
            [](uv_write_t* write_handle, int status) {
                auto self = static_cast<writer*>(write_handle->data);
                if (!self) {
                    delete write_handle;
                    return;
                }
                self->status = status;
                if (self->co_handle) 
                    self->co_handle();  
            });
            return *this;
        }
        
        bool await_ready() const { return status <= 0; }

        void await_suspend(std::coroutine_handle<> h) { co_handle = h; } 

        int await_resume() 
        { 
            status = 1;
            co_handle = nullptr; 
            return status;
        }

        ~writer()
        {
            if (write_handle && status == 2) {
                // write already requested, release to delete in callback
                write_handle->data = nullptr;
                write_handle.release();
            }
        }
    };   
}