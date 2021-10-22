// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <optional>

namespace couv {
    
    template <typename T>
    struct optional_promise {
        std::optional<T>* value;
        optional_promise() = default;
        optional_promise(optional_promise const&) = delete;

        auto get_return_object() noexcept { 
            struct extended_optional : public std::optional<T>{
                optional_promise* promise; 
                extended_optional(optional_promise* promise) noexcept : promise(promise) { promise->value = this;}
                extended_optional(extended_optional&& other) noexcept : promise(other.promise) { promise->value = this; }
            };
            return extended_optional{this};
        }

        auto initial_suspend() const noexcept { return std::suspend_never{}; }
        auto final_suspend() const noexcept { return std::suspend_never{}; }

        template <typename U = T>
        void return_value(U&& u) { *value = std::forward<U>(u); }
        void unhandled_exception() {}
    };

    template <typename T>
    auto operator co_await(std::optional<T>& opt) {
        struct optional_awaiter {
            std::optional<T>& opt;
            constexpr bool await_ready() const noexcept { return true; }
            constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
            constexpr T& await_resume() const& noexcept { return opt.value(); }
        };
        return optional_awaiter{opt};
    }

    template <typename T>
    auto operator co_await(const std::optional<T>& opt) {
        struct optional_awaiter {
            const std::optional<T>& opt;
            constexpr bool await_ready() const noexcept { return true; }
            constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
            constexpr const T& await_resume() const& noexcept { return opt.value(); }
        };
        return optional_awaiter{opt};
    }

    template <typename T>
    auto operator co_await(std::optional<T>&& opt) {
        struct optional_awaiter {
            std::optional<T>&& opt;
            constexpr bool await_ready() const noexcept { return true; }
            constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
            constexpr T&& await_resume() noexcept { return std::move(opt).value(); }
        };
        return optional_awaiter{opt};
    }

    template <typename T>
    auto operator co_await(const std::optional<T>&& opt) {
        struct optional_awaiter {
            const std::optional<T>&& opt;
            constexpr bool await_ready() const noexcept { return true; }
            constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
            constexpr const T&& await_resume() noexcept { return std::move(opt).value(); }
        };
        return optional_awaiter{opt};
    }
}

template <typename T, typename... Args>
struct std::coroutine_traits<std::optional<T>, Args...> {
    using promise_type = couv::optional_promise<T>;
};
