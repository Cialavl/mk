#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

namespace mk::translate {

struct TranslateConfig {
    std::string engine{"llama"};
    std::filesystem::path model_root;
    std::filesystem::path llama_model;
    std::uintmax_t expected_bytes{0};

    int n_ctx{4096};
    int n_batch{2048};
    int n_gpu_layers{99};
    int n_threads{8};
    bool flash_attn{true};
    int max_new_tokens{2048};

    [[nodiscard]] static TranslateConfig defaults(std::filesystem::path model_root);
    [[nodiscard]] static std::expected<TranslateConfig, std::string> load(const std::filesystem::path& file);
};

[[nodiscard]] std::filesystem::path executable_dir();
[[nodiscard]] std::filesystem::path find_model_root();

} // namespace mk::translate
