#include "error.h"

auto main() -> int {
    using dark::assertion;
    assertion(std::cin, "std::cin is not open {}", 1);
}
