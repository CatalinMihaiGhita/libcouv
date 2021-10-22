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
    template <class T = void>
    class task
    {
    public:
        struct promise_type
        {
            expect<T> _value;
            std::coroutine_handle<> _continuation;

            promise_type() : _continuation{std::noop_coroutine()}
            {
            }

            promise_type(const promise_type&) = delete;
            promise_type(promise_type&&) = default;


            task<T> get_return_object() noexcept
            {
                return std::coroutine_handle<promise_type>::from_promise(*this);
            }

            auto initial_suspend() { 
                return std::suspend_never{}; 
            }
            auto final_suspend() noexcept { 
                struct final_awaiter
                {
                    bool await_ready() const noexcept { return false; }
                    auto await_suspend(std::coroutine_handle<promise_type> ch) const noexcept { return ch.promise()._continuation; }
                    void await_resume() const noexcept {}
                };
                return final_awaiter{}; 
            }

            template <typename U>
            void return_value(U&& value) { 
                _value = std::forward<U>(value);
            }
            void unhandled_exception() { 
                _value = std::current_exception();
            }
        };

        task(std::coroutine_handle<promise_type> ch)
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
        void await_suspend(std::coroutine_handle<> ch) const noexcept { 
            _handle.promise()._continuation = ch; 
        }

        const expect<T>& await_resume() const noexcept { 
            return _handle.promise()._value;
        }

    private:
        std::coroutine_handle<promise_type> _handle;
    };

    template<>
    class task<void>
    {
    public:
        struct promise_type
        {
            expect<> _value;
            std::coroutine_handle<> _continuation;

            promise_type() : _continuation{std::noop_coroutine()} {}

            promise_type(const promise_type&) = delete;
            promise_type(promise_type&&) = default;

            task<void> get_return_object() noexcept
            {
                return std::coroutine_handle<promise_type>::from_promise(*this);
            }

            auto initial_suspend() { 
                return std::suspend_never{}; 
            }
            auto final_suspend() noexcept { 
                struct final_awaiter
                {
                    bool await_ready() const noexcept { return false; }
                    auto await_suspend(std::coroutine_handle<promise_type> ch) const noexcept { return ch.promise()._continuation; }
                    void await_resume() const noexcept {}
                };
                return final_awaiter{}; 
            }

            void return_void() const noexcept {}

            void unhandled_exception() noexcept {  
                _value = std::current_exception();
            }
        };

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

        void await_suspend(std::coroutine_handle<> ch) const noexcept {  _handle.promise()._continuation = ch; }

        const expect<>& await_resume() const noexcept { return _handle.promise()._value; }
    
    private:
        std::coroutine_handle<promise_type> _handle;
    };  
}