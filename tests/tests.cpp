// Copyright (C) 2021 Ghita Catalin Mihai
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <couv.hpp>
#include <thread>
#include <chrono>

couv::task<> tcp_test()
{
    couv::getaddrinfo info("www.google.com", "80");
    std::cout << (int)co_await info << std::endl;

    couv::tcp tcp;
    co_await tcp.connect(info);
    const char* httpget =
        "GET / HTTP/1.0\r\n"
        "Host: www.goole.com\r\n"
        "Cache-Control: max-age=0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
        "\r\n";
    tcp.write(httpget);
    auto reader = tcp.read();
    while (auto data = co_await reader) {
        std::cout << data << std::endl;
    }
}

couv::task<> tcp_server_test()
{
    couv::tcp tcp;
    std::vector<couv::tcp> clients;
    co_await tcp.bind("0.0.0.0", 8080);

    auto listner = tcp.listen(128);
    while (true) {
        std::cout << "tcp listen" << std::endl;
        if (co_await listner) {
            couv::tcp client;
            if (tcp.accept(client)) {
                clients.push_back(std::move(client));
            }
        }
    }
}

couv::task<> timer_test()
{
    std::cout << "timer start" << std::endl;
    co_await couv::timer(1000);
    std::cout << "timer test" << std::endl;
}

couv::work<double> worker(couv::async_sender<int> sender) // thread pool work
{
    std::cout << "doing work on " << std::this_thread::get_id() << std::endl;
    using namespace std::chrono_literals;
    int a = 10;
    while (a) {
        std::this_thread::sleep_for(2000ms);
        sender(a--);
    }
    co_return 100;
}

couv::task<> print_progress(couv::async<int> async)
{
    while (true) {
        auto a = co_await async;
        std::cout << "printing progress " << a << std::endl;
    }
}

couv::task<> work_test()
{
    couv::async<int> async;
    auto sender = async.sender();
    std::cout << "start work from " << std::this_thread::get_id() << std::endl;
    auto print_task = print_progress(std::move(async));
    auto i = co_await worker(std::move(sender));
    std::cout << "work finished " << i.value() << std::endl;
}

couv::task<> signal_test()
{
    auto server_task = tcp_server_test();
    auto work_task = work_test();
    co_await couv::signal{2};
    std::cout << "got sigint cancel server and work" << std::endl;
}

int main()
{
    auto tcp_task = tcp_test();
    auto signal_task = signal_test();
    auto timer_task = timer_test();
    
    std::cout << "loop run" << std::endl;
    return couv::loop();
}