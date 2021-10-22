#pragma once

#include <variant>
#include <exception>

namespace couv {

template <typename T = void, typename E = std::exception_ptr>
class expect
{
    std::variant<E, T> var;

public:
    expect(const T& t) : var(t) {}
    expect(const E& e) : var(e) {}

    void operator=(const T& t) 
    {
        var.emplace<1>(t);
    }

    void operator=(T&& t) 
    {
        var.emplace<1>(std::move(t));
    }

    void operator=(const E& e) 
    {
        var.emplace<0>(e);
    }

    const E& error() const 
    {
        return get<0>(var);
    }

    T& value() & 
    { 
        if (var.index() == 1) 
            return get<T>(var); 
        else
            std::rethrow_exception(get<E>(var)); 
    }

    const T& value() const&  
    { 
        if (var.index() == 1) 
            return get<1>(var); 
        else {
            if constexpr (std::is_same_v<std::exception_ptr, E>)
                std::rethrow_exception(get<0>(var)); 
            else
                throw get<0>(var);
        }
    }
    
    T&& value() &&
    { 
        if (var.index() == 1) 
            return get<1>(std::move(var)); 
        else {
            if constexpr (std::is_same_v<std::exception_ptr, E>)
                std::rethrow_exception(get<0>(var)); 
            else
                throw get<0>(var);
        }
    }

    const T&& value() const&&  
    { 
        if (var.index() == 1) 
            return get<1>(std::move(var)); 
        else {
            if constexpr (std::is_same_v<std::exception_ptr, E>)
                std::rethrow_exception(get<0>(var)); 
            else
                throw get<0>(var);
        }
    }
 
    operator bool() const noexcept { return var.index() == 1; }

    constexpr bool await_ready() const noexcept { return true; }
    constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
    decltype(auto) await_resume() const { return value(); }
};

template <typename E>
class expect<void, E>
{
    std::variant<E, std::monostate> var;
public:
    expect() : var(std::monostate{}) {}
    expect(const E& e) : var(e) {}

    void operator=(const E& e) 
    {
        var = e;
    }

    void value() const 
    { 
        if (var.index() == 0) {
            if constexpr (std::is_same_v<std::exception_ptr, E>)
                std::rethrow_exception(get<E>(var)); 
            else
                throw get<E>(var);
        }
    }

    const E& error() const {
        return get<E>(var);
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
