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
    class listner
    {
        std::shared_ptr<uv_stream_t> stream;
        std::coroutine_handle<> co_handle;
        int status;
        bool ready;

    public:
        listner(const tcp& tcp, int backlog);

        listner(listner&& l) :
            stream{std::move(l.stream)},
            co_handle{std::move(l.co_handle)},
            status{l.status},
            ready{l.ready}
        {
            stream->data = this;
        }

        bool await_ready() const { 
            return ready; 
        }

        void await_suspend(std::coroutine_handle<> h) {
            co_handle = h;
        }
        
        int await_resume() { 
            ready = false;
            co_handle = nullptr;
            return status;
        }
    };
}