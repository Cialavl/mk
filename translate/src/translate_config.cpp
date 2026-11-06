#include <translate/translate_config.h>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/json.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <thread>
#include <utility>

namespace mk::translate {
namespace fs = std::filesystem;
namespace json = boost::json;

namespace {

std::string read_file(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

fs::path resolve(const fs::path& root, std::string_view relative) {
    if (relative.empty()) return {};
    fs::path path{std::string(relative)};
    return path.is_absolute() ? path : root / path;
}

std::string get_string(const json::object& object, std::string_view key, std::string fallback) {
    if (const auto* value = object.if_contains(key); value && value->is_string()) {
        return std::string(value->get_string());
    }
    return fallback;
}

int get_int(const json::object& object, std::string_view key, int fallback) {
    if (const auto* value = object.if_contains(key)) {
        if (value->is_int64()) return static_cast<int>(value->get_int64());
        if (value->is_uint64()) return static_cast<int>(value->get_uint64());
    }
    return fallback;
}

bool get_bool(const json::object& object, std::string_view key, bool fallback) {
    if (const auto* value = object.if_contains(key); value && value->is_bool()) {
        return value->get_bool();
    }
    return fallback;
}

} // namespace

TranslateConfig TranslateConfig::defaults(fs::path root) {
    TranslateConfig config;
    config.model_root = std::move(root);
    config.llama_model = config.model_root / "index-9b" / "Index-Translate-9B.Q4_K_M.gguf";
    config.n_threads = std::clamp(static_cast<int>(std::thread::hardware_concurrency()), 1, 16);
    return config;
}

std::expected<TranslateConfig, std::string> TranslateConfig::load(const fs::path& file) {
    std::error_code ec;
    if (!fs::exists(file, ec)) {
        return std::unexpected("config not found: " + file.string());
    }

    const auto text = read_file(file);
    if (text.empty()) {
        return std::unexpected("empty config: " + file.string());
    }

    boost::system::error_code parse_error;
    const auto value = json::parse(text, parse_error);
    if (parse_error) {
        return std::unexpected("json parse error: " + parse_error.message());
    }
    if (!value.is_object()) {
        return std::unexpected("config root must be an object");
    }

    auto config = defaults(file.parent_path());
    const auto& root = value.as_object();
    config.engine = get_string(root, "engine", config.engine);

    if (const auto* llama = root.if_contains("llama"); llama && llama->is_object()) {
        const auto& object = llama->as_object();
        if (auto model = get_string(object, "model", ""); !model.empty()) {
            config.llama_model = resolve(config.model_root, model);
        }
        if (const auto* size = object.if_contains("expected_bytes")) {
            if (size->is_uint64()) {
                config.expected_bytes = static_cast<std::uintmax_t>(size->get_uint64());
            } else if (size->is_int64() && size->get_int64() >= 0) {
                config.expected_bytes = static_cast<std::uintmax_t>(size->get_int64());
            }
        }
        config.n_ctx = get_int(object, "n_ctx", config.n_ctx);
        config.n_batch = get_int(object, "n_batch", config.n_batch);
        config.n_gpu_layers = get_int(object, "n_gpu_layers", config.n_gpu_layers);
        config.n_threads = get_int(object, "n_threads", config.n_threads);
        config.flash_attn = get_bool(object, "flash_attn", config.flash_attn);
        config.max_new_tokens = get_int(object, "max_new_tokens", config.max_new_tokens);
    }

    return config;
}

fs::path executable_dir() {
    std::error_code ec;
    const auto path = boost::dll::program_location(ec);
    if (ec || path.empty()) return fs::current_path();
    return fs::path(path.parent_path().string());
}

fs::path find_model_root() {
    const auto exe = executable_dir();
    const std::array<fs::path, 4> candidates{
        exe / "model",
        exe / ".." / ".." / ".." / ".." / "resources" / "model",
        fs::current_path() / "resources" / "model",
        fs::current_path() / "model",
    };

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (fs::exists(candidate / "config.json", ec)) {
            auto canonical = fs::weakly_canonical(candidate, ec);
            return ec ? candidate : canonical;
        }
        if (fs::exists(candidate / "index-9b", ec) || fs::exists(candidate / "index-2b", ec)) {
            auto canonical = fs::weakly_canonical(candidate, ec);
            return ec ? candidate : canonical;
        }
    }
    return exe / "model";
}

} // namespace mk::translate
