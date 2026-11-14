#include <assist/utf8_path.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace mk::assist {

std::filesystem::path path_from_utf8(std::string_view utf8) {
    if (utf8.empty()) return {};
#if defined(_WIN32)
    const int size = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (size <= 0) return std::filesystem::path(std::string(utf8));
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), size);
    return std::filesystem::path(wide);
#else
    return std::filesystem::path(std::string(utf8));
#endif
}

std::string utf8_from_path(const std::filesystem::path& path) {
#if defined(_WIN32)
    const std::wstring wide = path.wstring();
    if (wide.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                                         nullptr, 0, nullptr, nullptr);
    if (size <= 0) return path.string();
    std::string utf8(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                        utf8.data(), size, nullptr, nullptr);
    return utf8;
#else
    return path.string();
#endif
}

std::optional<std::string> env_utf8(const char* name) {
#if defined(_WIN32)
    const int name_size = MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
    if (name_size <= 0) return std::nullopt;
    std::wstring wide_name(static_cast<std::size_t>(name_size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wide_name.data(), name_size);

    const DWORD size = GetEnvironmentVariableW(wide_name.c_str(), nullptr, 0);
    if (size == 0) return std::nullopt;
    std::wstring value(static_cast<std::size_t>(size), L'\0');
    const DWORD written = GetEnvironmentVariableW(wide_name.c_str(), value.data(), size);
    if (written == 0) return std::nullopt;
    value.resize(written);
    return utf8_from_path(std::filesystem::path(value));
#else
    if (const char* value = std::getenv(name); value != nullptr && *value != '\0') {
        return std::string(value);
    }
    return std::nullopt;
#endif
}

namespace {

bool usable(const std::filesystem::path& dir) {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec && !std::filesystem::is_directory(dir)) return false;

    const auto probe = dir / "mk-write-probe.tmp";
    {
        std::ofstream out(probe, std::ios::binary);
        if (!out) return false;
        out << 'x';
    }
    std::filesystem::remove(probe, ec);
    return true;
}

} // namespace

std::filesystem::path working_temp_dir() {
    std::vector<std::filesystem::path> candidates;

    if (const auto env = env_utf8("MK_TMP"); env && !env->empty()) {
        candidates.push_back(path_from_utf8(*env));
    }
    if (const auto local = env_utf8("LOCALAPPDATA"); local && !local->empty()) {
        candidates.push_back(path_from_utf8(*local) / "mk" / "tmp");
    }
    std::error_code ec;
    candidates.push_back(std::filesystem::temp_directory_path(ec));
    candidates.push_back(std::filesystem::current_path(ec) / ".mk-tmp");

    for (const auto& candidate : candidates) {
        if (candidate.empty()) continue;
        if (usable(candidate)) return candidate;
    }
    throw std::runtime_error("no writable temporary directory for mk");
}

} // namespace mk::assist
