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
    class signal
    {
        struct signal_data {
            uv_signal_t signal_handle;
            std::coroutine_handle<> co_handle;
            int ready{0};
            int signum;
        };

        struct signal_deleter
        {
            void operator()(signal_data* data) const noexcept {
                uv_close(reinterpret_cast<uv_handle_t*>(&data->signal_handle), [](uv_handle_t* signal_handle) {
                    delete static_cast<signal_data*>(signal_handle->data);
                });
            }
        };
        
        std::unique_ptr<signal_data, signal_deleter> data;

    public:
        signal() : 
            data{new signal_data{}, signal_deleter{}}
        {
            uv_signal_init(uv_default_loop(), &data->signal_handle);
            data->signal_handle.data = data.get();
        }

        signal(int signum) : signal()
        {
            start(signum);
        }

        error_code start(int signum) {
            return uv_signal_start(&data->signal_handle, [](uv_signal_t* signal_handle, int signum) {
                auto data = static_cast<signal_data*>(signal_handle->data);
                ++data->ready;
                data->signum = signum;
                [[likely]] if (data->co_handle) {
                    data->co_handle();
                }
            }, signum);
        }

        error_code stop() {
            return uv_signal_stop(&data->signal_handle);
        }

        bool await_ready() { 
            return data->ready; 
        }

        void await_suspend(std::coroutine_handle<> h) {
            data->co_handle = h;
        }
        
        int await_resume() { 
            data->co_handle = nullptr;
            --data->ready; 
            return data->signum;
        };
    };
}