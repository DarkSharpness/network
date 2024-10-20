#include "unit_test.h"
#include <concepts>
#include <iostream>
#include <string>
#include <string_view>
#include <strings.h>

static auto test() -> void {
    constexpr dark::consteval_string str = "Hello, World!";
    static_assert(std::constructible_from<dark::consteval_string, const char *>);
    static_assert(!std::constructible_from<dark::consteval_string, std::string_view>);

    // cstring_view is a lightweight wrapper around const char* with size
    // with assumption that the string is null terminated
    constexpr dark::cstring_view view = str;
    static_assert(view.size() == 13, "'Hello, World!' has 13 characters");

    // for user given length, it just serves as a hint
    // if the str[length] is not '\0', it will be ignored
    static constexpr const char c[] = "test string";
    constexpr dark::cstring_view view1{c, 1};
    static_assert(view1.size() == 11, "view1 has 11 character");

    // A on-stack string_view
    dark::stack_string stack_str = "stack string";
    std::cout << std::string_view{stack_str.view()} << std::endl;

    // We can construct cstring_view from std::string
    // since std::string guarantees null terminated string
    dark::stack_string<20> str1 = "null terminated ?";
    str1.remove(2);
    str1.append(' ');
    std::string str2         = str1 + stack_str.view().substr(6);
    dark::cstring_view view2 = str2;

    // We can construct std::string_view from cstring_view
    std::cout << std::string_view{view2} << std::endl;
}

static auto testcase = Testcase(test);
