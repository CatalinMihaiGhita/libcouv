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
    class timer
    {
        std::unique_ptr<uv_timer_t> timer_handle;
        std::coroutine_handle<> co_handle;
        int ready;

    public:
        timer() : 
            timer_handle{std::make_unique<uv_timer_t>()}, 
            ready{0} 
        {
            uv_timer_init(uv_default_loop(), timer_handle.get());
            timer_handle->data = this;
        }

        timer(uint64_t  timeout, uint64_t repeat = 0) : 
            timer_handle{std::make_unique<uv_timer_t>()}, 
            ready{0} 
        {
            uv_timer_init(uv_default_loop(), timer_handle.get());
            timer_handle->data = this;
            start(timeout, repeat);
        }


        timer(const timer&) = delete;
        timer(timer&& t) : 
            timer_handle{std::move(t.timer_handle)},
            co_handle{std::move(t.co_handle)},
            ready{t.ready}
        {
            timer_handle->data = this;
        }

        int start(uint64_t  timeout, uint64_t repeat = 0) {
            return uv_timer_start(timer_handle.get(), [](uv_timer_t* timer_handle) {
                auto self = static_cast<timer*>(timer_handle->data);
                ++self->ready;
                if (self->co_handle) {
                    self->co_handle();
                }
            }, timeout, repeat);
        }

        int again() {
            return uv_timer_again(timer_handle.get());
        }

        void set_repeat(uint64_t repeat) {
            return uv_timer_set_repeat(timer_handle.get(), repeat);
        }

        int stop() {
            return uv_timer_stop(timer_handle.get());
        }

        bool await_ready() const { 
            return ready; 
        }

        void await_suspend(std::coroutine_handle<> h) {
            co_handle = h;
        }
        
        void await_resume() { 
            co_handle = nullptr;
            --ready; 
        };

        ~timer()
        {
            if (timer_handle) {
                uv_close(reinterpret_cast<uv_handle_t*>(timer_handle.release()), 
                [](uv_handle_t* req) {
                    delete reinterpret_cast<uv_timer_t*>(req);
                });
            }
        }
    };

};