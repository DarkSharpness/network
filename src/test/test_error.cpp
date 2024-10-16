#include "error.h"
#include "unit_test.h"

static auto test() -> void {
    using dark::assertion;
    assertion(std::cin, "std::cin is not open {}", 1);
}

static auto testcase = Testcase(test);
