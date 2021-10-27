// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <coroutine>
#include <iostream>
#include <memory>
#include <mutex>

#include <uv.h> // libuv

namespace couv 
{
    template <typename T>
    class async;

    template <typename T = void>
    class async_sender
    {
        struct async_data
        {
            uv_async_t async_handle;
            std::coroutine_handle<> co_handle;
            std::mutex mutex;
            T value;
        };

        std::shared_ptr<async_data> data;
        friend class async<T>;

        async_sender(const std::shared_ptr<async_data>& data) : data{data}
        {}

    public:
        template <typename U>
        void send(U&& u) const
        {
            {
                std::lock_guard<decltype(data->mutex)> lock(data->mutex);
                data->value = std::forward<U>(u);
            }
            uv_async_send(&data->async_handle);
        }

        template <typename U>
        void operator()(U&& u) const 
        {
            send(std::forward<U>(u));
        }
    };

    template <typename T = void>
    class async
    {
        using async_data = typename async_sender<T>::async_data;
        std::shared_ptr<async_data> data;

        struct async_deleter
        {
            void operator()(async_data* p) const {
                uv_close(reinterpret_cast<uv_handle_t*>(&p->async_handle), [](uv_handle_t* req) {
                    delete reinterpret_cast<async_data*>(req->data);
                });
            }
        };

    public:
        async() : 
            data{new async_data, async_deleter{}}
        {
            data->async_handle.data = data.get();
            uv_async_init(uv_default_loop(), &data->async_handle, [] (uv_async_t *async_handle) {
                auto self = static_cast<async_data*>(async_handle->data);
                if (self->co_handle) {
                    self->co_handle();
                }
            });
        }

        async(const async&) = delete;
        async(async&& t) = default;

        bool await_ready() { 
            return false;
        }

        void await_suspend(std::coroutine_handle<> h) {
            data->co_handle = h;
        }
        
        T await_resume() const 
        {  
            std::lock_guard<decltype(data->mutex)> lock(data->mutex);
            return std::move(data->value);
        };

        async_sender<T> sender() const
        {
            return async_sender<T>{data};
        }
    };    
    
    template <>
    class async_sender<void>
    {
        std::shared_ptr<uv_async_t> async_handle;
        friend class async<void>;

        async_sender(const std::shared_ptr<uv_async_t>& async_handle) : async_handle{async_handle}
        {}

    public:
        void send() const
        {
            uv_async_send(async_handle.get());
        }

        void operator()() const 
        {
            uv_async_send(async_handle.get());
        }
    };

    template <>
    class async<void>
    {
        std::shared_ptr<uv_async_t> async_handle;

        struct async_deleter
        {
            void operator()(void* p) const {
                uv_close(static_cast<uv_handle_t*>(p), [](uv_handle_t* req) {
                    delete reinterpret_cast<uv_async_t*>(req);
                });
            }
        };

    public:
        async() : 
            async_handle{new uv_async_t, async_deleter{}}
        {
            uv_async_init(uv_default_loop(), async_handle.get(), [] (uv_async_t *async_handle) {
                if (async_handle->data) {
                    std::coroutine_handle<>::from_address(async_handle->data).resume();
                }
            });
        }

        async(const async&) = delete;
        async(async&& t) : 
            async_handle{std::move(t.async_handle)}
        {
        }

        bool await_ready() { 
            return false;
        }

        void await_suspend(std::coroutine_handle<> h) {
            async_handle->data = h.address();
        }
        
        void await_resume() const { };

        async_sender<> sender() const
        {
            return {async_handle};
        }

        ~async()
        {
            if (async_handle) {
                async_handle->data = nullptr;
            }
        }
    };    
}

