// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <coroutine>
#include <optional>
#include <task.hpp>

namespace couv {
    
    template <typename T>
    class optional_promise {
        std::optional<T>* opt;

        struct storage : public std::optional<T> {
            storage(optional_promise* promise) noexcept { promise->opt = this;}
            storage(const storage& other) = delete; 
            storage(storage&& other) = delete; 
            storage& operator=(storage&& other) = delete; 
            storage& operator=(const storage& other) = delete; 
        };

    public:
        optional_promise() = default;
        optional_promise(optional_promise const&) = delete;

        auto get_return_object() noexcept { 
            return storage{this};
        }

        std::suspend_never initial_suspend() const noexcept { return {}; }
        std::suspend_never final_suspend() const noexcept { return {}; }

        template <typename U = T>
        constexpr void return_value(U&& value) { *opt = std::forward<U>(value); }

        template <typename U = T>
        constexpr void return_value(const std::optional<U>& other) { *opt = other; }

        template <typename U = T>
        constexpr void return_value(std::optional<U>&& other) { *opt = std::move(other); }

        template <typename U>
        constexpr void return_value(std::nullopt_t) {}

        constexpr void unhandled_exception() const {}
    };

    template <typename T>
    struct optional_awaiter {
        const std::optional<T>& opt;
        constexpr bool await_ready() const noexcept { return opt.has_value(); }
        template <typename U>
        constexpr void await_suspend(std::coroutine_handle<optional_promise<U>> h) const noexcept { h.destroy(); }
        template <typename U>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<task_promise<U>> h) const noexcept {
            h.promise().return_value(std::make_exception_ptr(std::bad_optional_access{}));
            return h.promise().continuation();
        }
        constexpr const T& await_resume() const noexcept { return *opt; }
    };

    template <typename T>
    optional_awaiter<T> operator co_await(const std::optional<T>& opt) noexcept { return {opt};}

    template <typename T>
    struct move_optional_awaiter {
        std::optional<T>&& opt;
        constexpr bool await_ready() const noexcept { return opt.has_value(); }
        template <typename U>
        constexpr void await_suspend(std::coroutine_handle<optional_promise<U>> h) const noexcept { h.destroy(); }
        template <typename U>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<task_promise<U>> h) const noexcept 
        {
            h.promise().return_value(std::make_exception_ptr(std::bad_optional_access{}));
            return h.promise().continuation();
        }
        constexpr T&& await_resume() noexcept { return *std::move(opt); }
    };

    template <typename T>
    move_optional_awaiter<T> operator co_await(std::optional<T>&& opt) noexcept {return {std::move(opt)}; }
}

template <typename T, typename... Args>
struct std::coroutine_traits<std::optional<T>, Args...> {
    using promise_type = couv::optional_promise<T>;
};
