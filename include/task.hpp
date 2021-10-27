// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <coroutine>
#include <iostream>
#include <expect.hpp>

namespace couv
{
    template <typename T>
    class task;

    template <typename T>
    class task_promise
    {
        expect<T> _value;
        std::coroutine_handle<> _continuation;
        friend class task<T>;

    public:
        task_promise() : _continuation{std::noop_coroutine()}
        {
        }

        task_promise(const task_promise&) = delete;
        task_promise(task_promise&&) = default;

        std::coroutine_handle<task_promise> get_return_object() noexcept
        {
            return std::coroutine_handle<task_promise>::from_promise(*this);
        }
        
        std::coroutine_handle<> continuation() const noexcept { return _continuation; }
        void set_continuation(std::coroutine_handle<> h) noexcept { _continuation = h; }

        std::suspend_never initial_suspend() { 
            return {}; 
        }
        auto final_suspend() noexcept { 
            struct final_awaiter
            {
                bool await_ready() const noexcept { return false; }
                auto await_suspend(std::coroutine_handle<task_promise> ch) const noexcept { return ch.promise()._continuation; }
                void await_resume() const noexcept {}
            };
            return final_awaiter{}; 
        }

        template <typename U>
        void return_value(U&& value) { 
            _value = std::forward<U>(value);
        }

        void return_value(const std::exception_ptr& value) { 
            _value = value;
        }

        void return_value(std::exception_ptr&& value) { 
            _value = std::move(value);
        }

        void unhandled_exception() { 
            _value = std::current_exception();
        }
    };

    template <class T = void>
    class task
    {
    public:
        using promise_type = task_promise<T>;

        task(std::coroutine_handle<task_promise<T>> ch)
            : _handle(ch)
        {
        }

        task(const task&) = delete;

        task(task&& f) : _handle(f._handle) { 
            f._handle = nullptr; 
        } 

        task& operator=(task&& f) 
        {
            destroy();
            std::swap(_handle, f._handle);
            return *this;
        } 

        void destroy()
        {
            if (_handle) {
                _handle.destroy();
                _handle = nullptr;
            }
        }

        ~task() {
            if (_handle) {
                _handle.destroy();
            }
        }

        bool valid() const { return static_cast<bool>(_handle); }

        const expect<T>& expected() const { return _handle.promise()._value; }

        bool await_ready() const noexcept {
            return _handle.done();
        }
        void await_suspend(std::coroutine_handle<> ch) noexcept { 
            _handle.promise()._continuation = ch; 
        }

        const expect<T>& await_resume() const& noexcept { 
            return _handle.promise()._value;
        }

        expect<T>&& await_resume() && noexcept { 
            return std::move(_handle.promise()._value);
        }

    private:
        std::coroutine_handle<task_promise<T>> _handle;
    };

    template <>
    class task_promise<void>
    {
        expect<void> _value;
        std::coroutine_handle<> _continuation;
        friend class task<void>;

    public:
        task_promise() : _continuation{std::noop_coroutine()} {}

        task_promise(const task_promise&) = delete;
        task_promise(task_promise&&) = default;

        std::coroutine_handle<task_promise> get_return_object() noexcept
        {
            return std::coroutine_handle<task_promise>::from_promise(*this);
        }

        std::coroutine_handle<> continuation() const noexcept { return _continuation; }
        void set_continuation(std::coroutine_handle<> h) noexcept { _continuation = h; }
        
        std::suspend_never initial_suspend() { 
            return {}; 
        }
        auto final_suspend() noexcept { 
            struct final_awaiter
            {
                bool await_ready() const noexcept { return false; }
                auto await_suspend(std::coroutine_handle<task_promise> ch) const noexcept { return ch.promise()._continuation; }
                void await_resume() const noexcept {}
            };
            return final_awaiter{}; 
        }

        void return_value(const std::exception_ptr& value) { 
            _value = value;
        }

        void return_value(std::exception_ptr&& value) { 
            _value = std::move(value);
        }

        void unhandled_exception() noexcept {  
            _value = std::current_exception();
        }
    };

    template<>
    class task<void>
    {
    public:
        using promise_type = task_promise<void>;

        task(std::coroutine_handle<promise_type> ch)
            : _handle(ch)
        {}

        task(const task&) = delete;

        task(task&& f) : _handle(f._handle) { 
            f._handle = nullptr; 
        } 

        task& operator=(task&& f) 
        {
            destroy();
            std::swap(_handle, f._handle);
            return *this;
        } 

        void destroy()
        {
            if (_handle) {
                _handle.destroy();
                _handle = nullptr;
            }
        }

        ~task() {
            if (_handle) {
                _handle.destroy();
            }
        }

        bool valid() const noexcept { return static_cast<bool>(_handle); }

        const expect<>& expected() const noexcept { return _handle.promise()._value; }

        bool await_ready() const noexcept { return _handle.done(); }

        void await_suspend(std::coroutine_handle<> ch) noexcept { _handle.promise()._continuation = ch; }

        const expect<>& await_resume() const & noexcept { return _handle.promise()._value; }
        expect<>&& await_resume() && noexcept { return std::move(_handle.promise()._value); }
    
    private:
        std::coroutine_handle<promise_type> _handle;
    };  
}