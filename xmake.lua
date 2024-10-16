add_rules("mode.debug", "mode.release")

-- A list of warnings:
local warnings = {
    "all",      -- turn on all warnings
    "extra",    -- turn on extra warnings
    "error",    -- treat warnings as errors
    "pedantic", -- be pedantic
}

local other_cxflags = {
    "-Wswitch-default",     -- warn if no default case in a switch statement
}

add_includedirs("include/")
set_warnings(warnings)
add_cxflags(other_cxflags)
set_toolchains("gcc")
set_languages("c++23")
set_kind("binary")

target("hw1")
    add_files("src/hw1/*.cpp")

target("unit_test")
    add_includedirs("src/test/")
    add_files("src/test/*.cpp")
