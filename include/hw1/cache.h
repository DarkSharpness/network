#pragma once
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

inline std::shared_mutex cache_mutex;
inline std::unordered_map<std::string, std::string> cache;

inline auto look_up_cache(const std::string &str) -> std::optional<std::string> {
    std::shared_lock lock{cache_mutex};
    if (auto iter = cache.find(str); iter != cache.end()) {
        return iter->second;
    } else {
        return {};
    }
}

inline auto push_to_cache(std::string host, std::string response) -> void {
    std::unique_lock lock{cache_mutex};
    cache.try_emplace(std::move(host), std::move(response));
}

inline auto save_cache_to_file() -> void {
    auto tmp_path = std::filesystem::temp_directory_path() / "proxy_cache";
    auto error    = std::error_code{};
    std::filesystem::create_directories(tmp_path, error);
    auto file = std::ofstream{tmp_path / "index.txt"};
    std::unique_lock lock{cache_mutex};

    for (auto &[host, data] : cache) {
        file << host << '\n';
        const auto hash = std::hash<std::string>{}(host);
        std::ofstream{tmp_path / std::to_string(hash)} << data;
    }
}

inline auto load_cache_from_file() -> void {
    auto tmp_path = std::filesystem::temp_directory_path() / "proxy_cache";
    auto error    = std::error_code{};
    if (!std::filesystem::exists(tmp_path, error))
        return;

    std::cout << "Recovering cache...\n";

    auto file = std::ifstream{tmp_path / "index.txt"};
    for (std::string host; std::getline(file, host);) {
        auto hash = std::hash<std::string>{}(host);
        auto data = std::ifstream{tmp_path / std::to_string(hash)};
        std::cout << std::format("- cached: {}\n", host);
        push_to_cache(
            host,
            std::string{std::istreambuf_iterator<char>{data}, std::istreambuf_iterator<char>{}}
        );
    }
}
