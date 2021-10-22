// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <task.hpp>
#include <signal.hpp>
#include <timer.hpp>
#include <tcp.hpp>
#include <work.hpp>
#include <async.hpp>
#include <optional.hpp>
#include <expect.hpp>

namespace couv
{    
    int loop()
    {
        int ret = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
        uv_loop_close(uv_default_loop());
        return ret;
    }

}