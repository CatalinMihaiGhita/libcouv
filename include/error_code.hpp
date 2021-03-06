
// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <expect.hpp>

namespace couv {

    class error_code
    {
        int code;
    public:
        constexpr error_code(int code) noexcept : code(code)  {}
        constexpr operator int() const noexcept { return code; }
        constexpr operator bool() const noexcept { return code; }
        operator expect<void, error_code>() const noexcept 
        { 
            expect<void, error_code> e; 
            if (code) e = code; 
            return e;
        }

        void throw_if() const 
        {
            if (code) throw code;
        }

        constexpr bool await_ready() const noexcept { return true; }
        constexpr void await_suspend(std::coroutine_handle<>) const noexcept {}
        void await_resume() const { throw_if(); }
    };

}