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
        std::unique_ptr<uv_getaddrinfo_t> getaddrinfo_handle;
        std::coroutine_handle<> co_handle;
        bool ready;
        int status;

    public:
        getaddrinfo(const char *node, const char *service, struct addrinfo *hints = nullptr) : 
            getaddrinfo_handle{std::make_unique<uv_getaddrinfo_t>()}, 
            ready{0} 
        {
            getaddrinfo_handle->data = this;
            uv_getaddrinfo(uv_default_loop(), getaddrinfo_handle.get(), [](uv_getaddrinfo_t *getaddrinfo_handle, int status, struct addrinfo *res) {
                auto self = static_cast<getaddrinfo*>(getaddrinfo_handle->data);
                if (status == UV_ECANCELED) {
                    delete getaddrinfo_handle;
                    return;
                }
                self->ready = true;
                self->status = status;
                if (self->co_handle) {
                    self->co_handle();
                }
            }, node, service, hints);
        }

        getaddrinfo(const getaddrinfo&) = delete;
        getaddrinfo(getaddrinfo&& t) : 
            getaddrinfo_handle{std::move(t.getaddrinfo_handle)},
            co_handle{std::move(t.co_handle)},
            ready{t.ready},
            status{t.status}
        {
            getaddrinfo_handle->data = this;
        }

        bool await_ready() { 
            return ready; 
        }

        void await_suspend(std::coroutine_handle<> h) {
            co_handle = h;
        }
        
        error_code await_resume() { 
            return status;
        };

        ~getaddrinfo()
        {
            if (getaddrinfo_handle) {
                uv_freeaddrinfo(getaddrinfo_handle->addrinfo);
                if (uv_cancel(reinterpret_cast<uv_req_t*>(getaddrinfo_handle.get())) == 0) {
                    getaddrinfo_handle.release();
                }
            }
        }
    };    
}