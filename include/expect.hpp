#pragma once

#include <variant>
#include <exception>

namespace couv {

template <typename T = void, typename E = std::exception_ptr>
class expect
{
    std::variant<T, E> var;

public:
    expect() = default;

    expect(const T& t) : var(t) {}
    expect(T&& t) : var(std::move(t)) {}

    expect(std::in_place_t, const E& e) : var(e) {}
    expect(std::in_place_t, E&& e) : var(std::move(e)) {}

    void emplace(const T& t) 
    {
        var.template emplace<0>(t);
    }

    void emplace(T&& t) 
    {
        var.template emplace<0>(std::move(t));
    }

    void emplace_error(const E& e) 
    {
        var.template emplace<1>(e);
    }

    void emplace_error(E&& e) 
    {
        var.template emplace<1>(std::move(e));
    }

    expect& operator=(const T& t) 
    {
        emplace(t);
        return *this;
    }

    expect& operator=(T&& t) 
    {
        emplace(std::move(t));
        return *this;
    }

    expect& operator=(const E& e) 
    {
        emplace_error(e);
        return *this;
    }

    expect& operator=(E&& e) 
    {
        emplace_error(std::move(e));
        return *this;
    }

    const E& error() const 
    {
        return get<1>(var);
    }

    T& value() & 
    { 
        if (var.index() == 0) 
            return get<0>(var); 
        else {
            if constexpr (std::is_same_v<std::exception_ptr, E>)
                std::rethrow_exception(get<1>(var)); 
            else
                throw get<1>(var);
        }
    }

    const T& value() const&  
    { 
        [[likely]] if (var.index() == 0) 
            return get<0>(var); 
        else {
            if constexpr (std::is_same_v<std::exception_ptr, E>)
                std::rethrow_exception(get<1>(var)); 
            else
                throw get<1>(var);
        }
    }
    
    T&& value() &&
    { 
        [[likely]] if (var.index() == 0) 
            return get<0>(std::move(var)); 
        else {
            if constexpr (std::is_same_v<std::exception_ptr, E>)
                std::rethrow_exception(get<1>(var)); 
            else
                throw get<1>(var);
        }
    }

    const T&& value() const&&  
    { 
        [[likely]] if (var.index() == 0) 
            return get<0>(std::move(var)); 
        else {
            if constexpr (std::is_same_v<std::exception_ptr, E>)
                std::rethrow_exception(get<1>(var)); 
            else
                throw get<1>(var);
        }
    }

    constexpr bool has_value() const noexcept { return var.index() == 0; }
    constexpr operator bool() const noexcept { return has_value(); }

    constexpr bool await_ready() const noexcept { return has_value(); }
    constexpr void await_suspend(std::coroutine_handle<> h) const noexcept { h.destroy(); }
    const T& await_resume() const& noexcept { return *get_if<0>(&var); }
    T&& await_resume() && noexcept { return std::move(*get_if<0>(&var)); }
};

template <typename E>
class expect<void, E>
{
    std::variant<std::monostate, E> var;
public:
    expect() = default;

    expect(const E& e) : var(e) {}
    expect(E&& e) : var(std::move(e)) {}

    void emplace() {
        var.template emplace<0>(std::monostate{});
    }

    void emplace_error(const E& e) 
    {
        var.template emplace<1>(e);
    }

    void emplace_error(E&& e) 
    {
        var.template emplace<1>(std::move(e));
    }

    expect& operator=(const E& e) 
    {
        emplace_error(e);
        return *this;
    }

    expect& operator=(E&& e) 
    {
        emplace_error(std::move(e));
        return *this;
    }

    void value() const 
    { 
        [[unlikely]] if (var.index() == 1) {
            if constexpr (std::is_same_v<std::exception_ptr, E>)
                std::rethrow_exception(get<1>(var)); 
            else
                throw get<1>(var);
        }
    }

    const E& error() const {
        return get<E>(var);
    }

    constexpr bool has_value() const noexcept { return var.index() == 0; }
    constexpr operator bool() const noexcept { return has_value(); }

    constexpr bool await_ready() const noexcept { return has_value(); }
    constexpr void await_suspend(std::coroutine_handle<> h) const noexcept { h.destroy(); }
    constexpr void await_resume() const noexcept {}
};


template <typename T>
struct expect_promise {
    expect<T>* value;
    expect_promise() = default;
    expect_promise(expect_promise const&) = delete;

    struct storage : public expect<T>{
            expect_promise* promise; 
            storage(expect_promise* promise) noexcept : expect<T>(std::in_place, std::exception_ptr{}), promise(promise) { promise->value = this;}
            storage(const storage& other) = delete;
            storage(storage&& other) = delete;
            storage& operator=(storage&& other) = delete; 
            storage& operator=(const storage& other) = delete; 
    };

    auto get_return_object() noexcept { 
        return storage{this};
    }

    std::suspend_never initial_suspend() const noexcept { return {}; }
    std::suspend_never final_suspend() const noexcept { return {}; }

    template <typename U = T>
    void return_value(U&& u) { *value = std::forward<U>(u); }
    void unhandled_exception() noexcept { *value = std::current_exception(); }
};

template <>
struct expect_promise<void> {
    expect<>* value;
    expect_promise() = default;
    expect_promise(expect_promise const&) = delete;

     struct storage : public expect<> {
            expect_promise* promise; 
            storage(expect_promise* promise) noexcept : promise(promise) { promise->value = this;}
            storage(const storage& other) = delete;
            storage(storage&& other) = delete;
            storage& operator=(storage&& other) = delete; 
            storage& operator=(const storage& other) = delete; 
    };

    auto get_return_object() noexcept { 
        return storage{this};
    }

    auto initial_suspend() const noexcept { return std::suspend_never{}; }
    auto final_suspend() const noexcept { return std::suspend_never{}; }

    void return_void() noexcept {} 
    void unhandled_exception() noexcept { *value = std::current_exception(); }
};

}

template <typename T, typename... Args>
struct std::coroutine_traits<couv::expect<T>, Args...> {
    using promise_type = couv::expect_promise<T>;
};
