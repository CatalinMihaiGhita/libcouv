// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <coroutine>
#include <memory>
#include <error_code.hpp>

#include <uv.h> // libuv

namespace couv
{
    class timer
    {
        struct timer_data {
            uv_timer_t timer_handle;
            std::coroutine_handle<> co_handle;
            int ready{0};
        };

        struct timer_deleter
        {
            void operator()(timer_data* data) const noexcept {
                uv_close(reinterpret_cast<uv_handle_t*>(&data->timer_handle), [](uv_handle_t* timer_handle) {
                    delete static_cast<timer_data*>(timer_handle->data);
                });
            }
        };
        
        std::unique_ptr<timer_data, timer_deleter> data;

    public:
        timer() : data{new timer_data{}, timer_deleter{}}
        {
            uv_timer_init(uv_default_loop(), &data->timer_handle);
            data->timer_handle.data = data.get();
        }

        timer(uint64_t  timeout, uint64_t repeat = 0) 
            : data{new timer_data{}, timer_deleter{}}
        {
            uv_timer_init(uv_default_loop(), &data->timer_handle);
            data->timer_handle.data = data.get();
            start(timeout, repeat);
        }

        error_code start(uint64_t  timeout, uint64_t repeat = 0) noexcept {
            return uv_timer_start(&data->timer_handle, [](uv_timer_t* timer_handle) {
                auto data = static_cast<timer_data*>(timer_handle->data);
                ++data->ready;
                if (data->co_handle) {
                    data->co_handle();
                }
            }, timeout, repeat);
        }

        error_code again() noexcept {
            return uv_timer_again(&data->timer_handle);
        }

        void set_repeat(uint64_t repeat) noexcept {
            return uv_timer_set_repeat(&data->timer_handle, repeat);
        }

        error_code stop() noexcept {
            return uv_timer_stop(&data->timer_handle);
        }

        bool await_ready() const noexcept { 
            return data->ready; 
        }

        void await_suspend(std::coroutine_handle<> h) noexcept {
            data->co_handle = h;
        }
        
        void await_resume() noexcept { 
            data->co_handle = nullptr;
            --data->ready; 
        };
    };

};