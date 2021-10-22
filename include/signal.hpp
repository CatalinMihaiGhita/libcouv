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
    class signal
    {
        std::unique_ptr<uv_signal_t> signal_handle;
        std::coroutine_handle<> co_handle;
        int ready;
        int signum;

    public:
        signal() : 
            signal_handle{std::make_unique<uv_signal_t>()}, 
            ready{0} 
        {
            uv_signal_init(uv_default_loop(), signal_handle.get());
            signal_handle->data = this;
        }

        signal(int signum) : 
            signal_handle{std::make_unique<uv_signal_t>()}, 
            ready{0} 
        {
            uv_signal_init(uv_default_loop(), signal_handle.get());
            signal_handle->data = this;
            start(signum);
        }

        signal(const signal&) = delete;
        signal(signal&& t) : 
            signal_handle{std::move(t.signal_handle)},
            co_handle{std::move(t.co_handle)},
            ready{t.ready}
        {
            signal_handle->data = this;
        }

        int start(int signum) {
            return uv_signal_start(signal_handle.get(), [](uv_signal_t* signal_handle, int signum) {
                auto self = static_cast<signal*>(signal_handle->data);
                ++self->ready;
                self->signum = signum;
                if (self->co_handle) {
                    self->co_handle();
                }
            }, signum);
        }

        int stop() {
            return uv_signal_stop(signal_handle.get());
        }

        bool await_ready() { 
            return ready; 
        }

        void await_suspend(std::coroutine_handle<> h) {
            co_handle = h;
        }
        
        int await_resume() { 
            co_handle = nullptr;
            --ready; 
            return signum;
        };

        ~signal()
        {
            if (signal_handle) {
                uv_close(reinterpret_cast<uv_handle_t*>(signal_handle.release()), 
                    [](uv_handle_t* req) {
                        delete reinterpret_cast<uv_signal_t*>(req);
                    });
            }
        }
    };
}