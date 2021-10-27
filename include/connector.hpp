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
#include <error_code.hpp>

namespace couv
{
    class tcp;
    class connector
    {
        struct connector_data {
            uv_connect_t req;
            std::coroutine_handle<> co_handle;
            int status;
        };

        std::unique_ptr<connector_data> data;

    public:

        connector(const tcp& tcp, const char* ip, int port);
        connector(const tcp& tcp, const getaddrinfo&);

        connector(connector&&) = default;
        connector& operator=(connector&&) = default;

        ~connector()
        {
            [[unlikely]] if (data && data->status == 1) {
                data->req.cb = [](uv_connect_t* req, int) {
                    delete static_cast<connector_data*>(req->data);
                };
                data.release();
            }
        }

        bool await_ready() { 
            return data->status <= 0; 
        }

        void await_suspend(std::coroutine_handle<> h) {
            data->co_handle = h;
        }
        
        error_code await_resume() { 
            return data->status;
        }
    };
}