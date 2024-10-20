#include "errors.h"
#include "optional.h"
#include "unit_test.h"
#include <cstdio>

static auto test() -> void {
    using dark::optional;
    optional<int> a;
    optional<int> b(42);
    try {
        a.unwrap("expected error");
    } catch (const std::runtime_error &e) {
        dark::assertion(std::string(e.what()) == "assertion failed");
    }
    b.unwrap();
    struct Test {
        Test() {
            std::puts("Test::Test()");
        }
        ~Test() {
            std::puts("Test::~Test()");
        }
    };

    optional<Test> c;
    c.emplace();
}

static auto testcase = Testcase(test);
