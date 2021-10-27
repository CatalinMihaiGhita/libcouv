#pragma once

#include <coroutine>
#include <memory>

#include <uv.h> // libuv

namespace couv
{
    class connector;
    class getaddrinfo
    {
        friend class connector;
        struct getaddrinfo_data {
            uv_getaddrinfo_t getaddrinfo_handle;
            std::coroutine_handle<> co_handle;
            bool ready{false};
            int status;
        };

        std::unique_ptr<getaddrinfo_data> data;

    public:
        getaddrinfo(const char *node, const char *service, struct addrinfo *hints = nullptr) : 
            data{std::make_unique<getaddrinfo_data>()}
        {
            data->getaddrinfo_handle.data = data.get();
            uv_getaddrinfo(uv_default_loop(), &data->getaddrinfo_handle, [](uv_getaddrinfo_t *getaddrinfo_handle, int status, struct addrinfo *res) {
                auto data = static_cast<getaddrinfo_data*>(getaddrinfo_handle->data);
                if (status == UV_ECANCELED) {
                    delete data;
                    return;
                }
                data->ready = true;
                data->status = status;
                [[likely]] if (data->co_handle) {
                    data->co_handle();
                }
            }, node, service, hints);
        }

        bool await_ready() const { 
            return data->ready; 
        }

        void await_suspend(std::coroutine_handle<> h) {
            data->co_handle = h;
        }
        
        error_code await_resume() const { 
            return data->status;
        };

        ~getaddrinfo()
        {
            if (data) {
                uv_freeaddrinfo(data->getaddrinfo_handle.addrinfo);
                if (uv_cancel(reinterpret_cast<uv_req_t*>(&data->getaddrinfo_handle)) == 0) {
                    data.release();
                }
            }
        }
    };    
}