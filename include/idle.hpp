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
    class idle
    {
        struct idle_data {
            uv_idle_t idle_handle;
            std::coroutine_handle<> co_handle;
            int ready{0};
        };

        struct idle_deleter
        {
            void operator()(idle_data* data) const noexcept {
                uv_close(reinterpret_cast<uv_handle_t*>(&data->idle_handle), [](uv_handle_t* idle_handle) {
                    delete static_cast<idle_data*>(idle_handle->data);
                });
            }
        };

        std::unique_ptr<idle_data, idle_deleter> data;

    public:
        idle(bool autostart = false) : 
            data{new idle_data{}, idle_deleter{}}
        {
            uv_idle_init(uv_default_loop(), &data->idle_handle);
            data->idle_handle.data = data.get();
            if (autostart) {
                start();
            }
        }

        idle(idle&&) = default;
        idle& operator=(idle&&) = default;

        error_code start() {
            return uv_idle_start(&data->idle_handle, [](uv_idle_t* idle_handle) {
                auto data = static_cast<idle_data*>(idle_handle->data);
                ++data->ready;
                [[likely]] if (data->co_handle) {
                    data->co_handle();
                }
            });
        }

        error_code stop() {
            return uv_idle_stop(&data->idle_handle);
        }

        bool await_ready() { 
            return data->ready; 
        }

        void await_suspend(std::coroutine_handle<> h) {
            data->co_handle = h;
        }
        
        void await_resume() { 
            data->co_handle = nullptr;
            --data->ready; 
        };
    };    
}