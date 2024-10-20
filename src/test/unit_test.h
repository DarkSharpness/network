#pragma once
#include <functional>
#include <source_location>
#include <string>
#include <vector>

struct Tester {
public:
    using _Function_t = std::function<void(void)>;
    auto &add(std::string name, _Function_t function) {
        testcases.push_back({name, function});
        return *this;
    }

    auto run() -> void;
    friend auto tester_instance() -> Tester &;

private:
    explicit Tester() = default;
    struct TestCase {
        std::string name;
        _Function_t function;
    };
    std::vector<TestCase> testcases;
};

auto tester_instance() -> Tester &;

struct Testcase {
    using _Src_t = std::source_location;
    template <typename _F>
    Testcase(_F &&function, _Src_t loc = _Src_t::current()) {
        tester_instance().add(loc.file_name(), std::forward<_F>(function));
    }
};
