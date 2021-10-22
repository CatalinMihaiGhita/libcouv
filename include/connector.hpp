// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <coroutine>
#include <iostream>
#include <memory>

#include <uv.h> // libuv
#include <getaddrinfo.hpp>

namespace couv
{
    class tcp;
    class connector
    {
        std::unique_ptr<uv_connect_t> req;
        std::coroutine_handle<> co_handle;
        int status;

    public:

        connector(const tcp& tcp, const char* ip, int port);
        connector(const tcp& tcp, const getaddrinfo&);

        connector(connector&& c) :
            req(std::move(c.req)),
            co_handle(std::move(c.co_handle)),
            status(std::move(c.status))
        {
            req->data = this;
        }

        ~connector()
        {
            if (req && status == 1) {
                // pending callback
                req->data = nullptr;
                req.release();
            }
        }

        bool await_ready() { 
            return status <= 0; 
        }

        void await_suspend(std::coroutine_handle<> h) {
            co_handle = h;
        }
        
        int await_resume() { 
            return status;
        }
    };
}