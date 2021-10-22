// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <coroutine>
#include <iostream>
#include <memory>
#include <thread>
#include <optional>

#include <uv.h> // libuv

namespace couv
{
    template <class T = void>
    struct work
    {
        struct promise_type
        {
            uv_work_t work_handle;
            std::coroutine_handle<> continuation;
            std::coroutine_handle<> thread_pool_continuation;
            std::optional<T> value;
            bool ready{false};

            promise_type() = default;

            ~promise_type() {
                uv_cancel(reinterpret_cast<uv_req_t*>(&work_handle));
            }

            promise_type(const promise_type&) = delete;
            promise_type(promise_type&&) = default;

            work<T> get_return_object() noexcept
            {
                return std::coroutine_handle<promise_type>::from_promise(*this);
            }

            auto initial_suspend() noexcept { 
                struct awaitable
                {
                    promise_type* promise;
                    bool await_ready() const noexcept { return false; }
                    void await_suspend(std::coroutine_handle<> h) noexcept {
                        promise->work_handle.data = promise;
                        promise->thread_pool_continuation = h;
                        uv_queue_work(uv_default_loop(), &promise->work_handle, 
                        [](uv_work_t *work_handle){
                            auto self = static_cast<promise_type*>(work_handle->data);
                            self->thread_pool_continuation();
                        },
                        [](uv_work_t *work_handle, int status){
                            if (status != UV_ECANCELED) {
                                auto self = static_cast<promise_type*>(work_handle->data);
                                self->ready = true;
                                if (self->continuation)
                                    self->continuation();
                            }
                        });
                    }
                    void await_resume() const noexcept {};
                };
                return awaitable{this};
            }

            auto final_suspend() noexcept { 
                return std::suspend_always{};
            }

            template <typename U>
            void return_value(U&& v) { 
                value.emplace(std::forward<U>(v)); 
            }
            void unhandled_exception() { }
        };

        work(std::coroutine_handle<promise_type> ch)
            : _handle(ch)
        {}

        work(const work&) = delete;

        work(work&& f) : _handle(std::move(f._handle)) { 
            f._handle = nullptr; 
        } 

        ~work()
        {
            if (_handle) {
                _handle.destroy();
            }
        }

        bool await_ready() const {
             return  _handle.promise().ready;
        }
        void await_suspend(std::coroutine_handle<> ch) const { 
            _handle.promise().continuation = ch; 
        }
        const T& await_resume() const { return *_handle.promise().value; }

        std::coroutine_handle<promise_type> _handle;
    };

    template<>
    struct work<void>
    {
        struct promise_type
        {
            uv_work_t work_handle;
            std::coroutine_handle<> continuation;
            std::coroutine_handle<> thread_pool_continuation;
            bool ready{false};
            
            promise_type() = default;

            ~promise_type() {
                uv_cancel(reinterpret_cast<uv_req_t*>(&work_handle));
            }

            promise_type(const promise_type&) = delete;
            promise_type(promise_type&&) = default;

            work<void> get_return_object() noexcept
            {
                return std::coroutine_handle<promise_type>::from_promise(*this);
            }

            auto initial_suspend() noexcept { 
                struct awaitable
                {
                    promise_type* promise;
                    bool await_ready() const noexcept { return false; }
                    void await_suspend(std::coroutine_handle<> h) const noexcept {
                        promise->work_handle.data = promise;
                        promise->thread_pool_continuation = h;
                        uv_queue_work(uv_default_loop(), &promise->work_handle, 
                        [](uv_work_t *work_handle){
                            auto self = static_cast<promise_type*>(work_handle->data);
                            self->thread_pool_continuation();
                        },
                        [](uv_work_t *work_handle, int status){
                            if (status != UV_ECANCELED) {
                                auto self = static_cast<promise_type*>(work_handle->data);
                                self->ready = true;
                                if (self->continuation)
                                    self->continuation();
                            }
                        });
                    }
                    void await_resume() const noexcept {};
                };
                return awaitable{this};
            }
            
            auto final_suspend() noexcept { 
               return std::suspend_always{};
            }

            void return_void() noexcept { }         
            void unhandled_exception() { }
        };

        work(std::coroutine_handle<promise_type> ch)
            : _handle(ch)
        {}

        work(const work&) = delete;

        work(work&& f) : _handle(std::move(f._handle)) { 
            f._handle = nullptr; 
        } 

        ~work() {
            if (_handle) {
                _handle.destroy();
            }
        }

        bool await_ready() const noexcept {
             return _handle.promise().ready; 
        }
        void await_suspend(std::coroutine_handle<> ch) const noexcept { 
            _handle.promise().continuation = ch; 
        }
        void await_resume() const noexcept {}

        std::coroutine_handle<promise_type> _handle;
    };  

    

};