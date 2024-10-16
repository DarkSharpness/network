#include "unit_test.h"
#include <format>
#include <iostream>
#include <string_view>
#include <syncstream>
#include <thread>

auto tester_instance() -> Tester & {
    static auto instance = Tester{};
    return instance;
}

static auto thread_test(std::function<void()> function, std::string name) -> void {
    auto thread_id = std::this_thread::get_id();
    std::osyncstream(std::cout) << std::format("{:=^80}\nThread ID: ", "") << thread_id
                                << std::format("\n - Running test: {}\n{:=^80}\n", name, "");
    auto msg = std::string_view{"\033[1;32mpassed\033[0m"};
    auto tic = std::chrono::high_resolution_clock::now();
    try {
        function();
    } catch (...) { msg = "\033[1;31mfailed\033[0m"; }
    auto toc = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic).count();

    std::cout << std::format("{:=^80}\nTest {} - {} after {}ms\n{:=^80}\n", "", name, msg, dur, "");
}

auto Tester::run() -> void {
    auto threads = std::vector<std::thread>{};
    for (auto &testcase : testcases) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(0.1s);
        threads.emplace_back(thread_test, std::move(testcase.function), std::move(testcase.name));
    }
    testcases.clear();
    for (auto &thread : threads)
        thread.join();
}

auto main() -> int {
    tester_instance().run();
}
