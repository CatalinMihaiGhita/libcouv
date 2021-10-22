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
        std::unique_ptr<uv_idle_t> idle_handle;
        std::coroutine_handle<> co_handle;
        int ready;

    public:
        idle(bool autostart = false) : 
            idle_handle{std::make_unique<uv_idle_t>()}, 
            ready{0} 
        {
            uv_idle_init(uv_default_loop(), idle_handle.get());
            idle_handle->data = this;
            if (autostart) {
                start();
            }
        }

        idle(const idle&) = delete;
        idle(idle&& t) : 
            idle_handle{std::move(t.idle_handle)},
            co_handle{std::move(t.co_handle)},
            ready{t.ready}
        {
            idle_handle->data = this;
        }

        error_code start() {
            return uv_idle_start(idle_handle.get(), [](uv_idle_t* idle_handle) {
                auto self = static_cast<idle*>(idle_handle->data);
                ++self->ready;
                if (self->co_handle) {
                    self->co_handle();
                }
            });
        }

        error_code stop() {
            return uv_idle_stop(idle_handle.get());
        }

        bool await_ready() { 
            return ready; 
        }

        void await_suspend(std::coroutine_handle<> h) {
            co_handle = h;
        }
        
        void await_resume() { 
            co_handle = nullptr;
            --ready; 
        };

        ~idle()
        {
            if (idle_handle) {
                uv_close(reinterpret_cast<uv_handle_t*>(idle_handle.release()), 
                [](uv_handle_t* req) {
                    delete reinterpret_cast<uv_idle_t*>(req);
                });
            }
        }
    };    
}