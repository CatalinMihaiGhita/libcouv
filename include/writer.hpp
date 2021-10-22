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
    class writer
    {
        struct writer_data {
            std::shared_ptr<uv_stream_t> stream;
            uv_write_t write_handle;
            std::string to_write;
            std::coroutine_handle<> co_handle;
            int status;
        };

        std::unique_ptr<writer_data> data;

    public:
        writer(const tcp&);
        writer(writer&&) = default;
        writer& operator=(writer&&) = default;

        writer& write(std::string some_data)
        {
            data->to_write = std::move(some_data);
            return *this;
        }
        
        bool await_ready() const noexcept { return false; }

        void await_suspend(std::coroutine_handle<> h) noexcept
        { 
            data->co_handle = h; 
            uv_buf_t buf = uv_buf_init(data->to_write.data(), data->to_write.size());
            uv_write(&data->write_handle, data->stream.get(), &buf, 1,
            [](uv_write_t* write_handle, int status) {
                auto data = static_cast<writer_data*>(write_handle->data);
                [[likely]] if (data->co_handle) {
                    data->status = status;
                    data->co_handle();  
                } else delete data;
            });
        } 

        error_code await_resume() noexcept
        { 
            data->co_handle = nullptr;
            return data->status;
        }

        ~writer() 
        {
            [[unlikely]] if (data && data->co_handle) 
            {
                data->co_handle = nullptr;
                data.release();
            }
        }
    };   
}