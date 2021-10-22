#pragma once

#include <variant>
#include <exception>

namespace couv {

template <typename T = void>
class expect
{
    std::variant<std::exception_ptr, T> var;

public:
    expect(const T& t) : var(t) {}
    expect(std::exception_ptr e) : var(std::move(e)) {}

    void operator=(const T& t) 
    {
        var.emplace<1>(t);
    }

    void operator=(T&& t) 
    {
        var.emplace<1>(std::move(t));
    }

    void operator=(std::exception_ptr e) 
    {
        var.emplace<0>(std::move(e));
    }

    const std::exception_ptr& error() const 
    {
        return get<std::exception_ptr>(var);
    }

    T& value() & 
    { 
        if (var.index() == 1) 
            return get<T>(var); 
        else
            std::rethrow_exception(get<std::exception_ptr>(var)); 
    }

    const T& value() const&  
    { 
        if (var.index() == 1) 
            return get<T>(var); 
        else
            std::rethrow_exception(get<std::exception_ptr>(var)); 
    }
    
    T&& value() &&
    { 
        if (var.index() == 1) 
            return get<T>(std::move(var)); 
        else
            std::rethrow_exception(get<std::exception_ptr>(var)); 
    }

    const T&& value() const&&  
    { 
        if (var.index() == 1) 
            return get<T>(std::move(var)); 
        else
            std::rethrow_exception(get<std::exception_ptr>(var)); 
    }
 
    operator bool() const noexcept { return var.index() == 1; }

    constexpr bool await_ready() const noexcept { return true; }
    constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
    decltype(auto) await_resume() const { return value(); }
};

template <>
class expect<void>
{
    std::variant<std::exception_ptr, std::monostate> var;
public:
    expect() : var(std::monostate{}) {}
    expect(std::exception_ptr e) : var(std::move(e)) {}

    void operator=(std::exception_ptr e) 
    {
        var.emplace<0>(std::move(e));
    }

    void value() const 
    { 
        if (var.index() == 0) 
            std::rethrow_exception(get<std::exception_ptr>(var)); 
    }

    const std::exception_ptr& error() const {
        return get<std::exception_ptr>(var);
    }

    operator bool() const noexcept { return var.index() == 1; }

    constexpr bool await_ready() const noexcept { return true; }
    constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
    void await_resume() const { value(); }
};


template <typename T>
struct expect_promise {
    expect<T>* value;
    expect_promise() = default;
    expect_promise(expect_promise const&) = delete;

    auto get_return_object() noexcept { 
        struct extended_expect : public expect<T>{
            expect_promise* promise; 
            extended_expect(expect_promise* promise) noexcept : expect<T>(nullptr), promise(promise) { promise->value = this;}
            extended_expect(extended_expect&& other) noexcept : expect<T>(nullptr), promise(other.promise) { promise->value = this; }
        };
        return extended_expect{this};
    }

    auto initial_suspend() const noexcept { return std::suspend_never{}; }
    auto final_suspend() const noexcept { return std::suspend_never{}; }

    template <typename U = T>
    void return_value(U&& u) { *value = std::forward<U>(u); }
    void unhandled_exception() { *value = std::current_exception(); }
};

template <>
struct expect_promise<void> {
    expect<>* value;
    expect_promise() = default;
    expect_promise(expect_promise const&) = delete;

    auto get_return_object() noexcept { 
        struct extended_expect : public expect<>{
            expect_promise* promise; 
            extended_expect(expect_promise* promise) noexcept : promise(promise) { promise->value = this;}
            extended_expect(extended_expect&& other) noexcept : promise(other.promise) { promise->value = this; }
        };
        return extended_expect{this};
    }

    auto initial_suspend() const noexcept { return std::suspend_never{}; }
    auto final_suspend() const noexcept { return std::suspend_never{}; }

    void return_void() {}
    void unhandled_exception() { *value = std::current_exception(); }
};

}

template <typename T, typename... Args>
struct std::coroutine_traits<couv::expect<T>, Args...> {
    using promise_type = couv::expect_promise<T>;
};
