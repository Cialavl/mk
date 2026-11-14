#include <assist/ocr_engine.h>

#include <assist/utf8_path.h>

#include "src/api/pipelines/ocr.h"
#include "src/base/base_cv_result.h"

#include <boost/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace mk::assist {
namespace {

namespace fs = std::filesystem;

std::filesystem::path default_paddle_root() {
    if (const auto env = env_utf8("MK_PADDLEOCR_ROOT"); env && !env->empty()) {
        return path_from_utf8(*env);
    }
    return path_from_utf8("C:/Cpp_Libs/mt/Release/PaddleOCR");
}

void add_dll_dir(const fs::path& dir) {
#if defined(_WIN32)
    if (!dir.empty() && fs::exists(dir)) {
        AddDllDirectory(dir.wstring().c_str());
    }
#else
    (void)dir;
#endif
}

void prepare_dll_dirs(const fs::path& paddle_root) {
#if defined(_WIN32)
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_USER_DIRS);
    add_dll_dir(paddle_root / "bin");
    add_dll_dir(paddle_root.parent_path() / "paddle_inference/paddle/lib");
    add_dll_dir(paddle_root.parent_path() / "paddle_inference/third_party/install/mklml/lib");
#endif
}

std::filesystem::path executable_dir() {
#if defined(_WIN32)
    std::wstring buffer(MAX_PATH, L' ');
    const DWORD written = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (written == 0) return {};
    buffer.resize(written);
    return fs::path(buffer).parent_path();
#else
    return fs::current_path();
#endif
}

// PaddleOCR resolves its pipeline yaml from a build-time source path, so the
// config has to be supplied explicitly.
std::filesystem::path config_path() {
    if (const auto env = env_utf8("MK_PADDLEOCR_CONFIG"); env && !env->empty()) {
        return path_from_utf8(*env);
    }

    const fs::path exe_dir = executable_dir();
    const fs::path candidates[] = {
        exe_dir / "configs" / "OCR.yaml",
        exe_dir / ".." / ".." / ".." / "assist" / "configs" / "OCR.yaml",
    };
    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) return fs::weakly_canonical(candidate);
    }
    return {};
}

std::string read_text_file(const fs::path& file) {
    std::ifstream input(file, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot read OCR result: " + utf8_from_path(file));
    }
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

const fs::path* find_json_result(const fs::path& dir) {
    static fs::path found;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            found = entry.path();
            return &found;
        }
    }
    return nullptr;
}

std::string extract_text(const boost::json::value& root) {
    const auto* object = root.if_object();
    if (object == nullptr) return {};

    const auto* texts_value = object->if_contains("rec_texts");
    if (texts_value == nullptr || !texts_value->is_array()) return {};

    const auto* boxes_value = object->if_contains("rec_boxes");
    const auto* scores_value = object->if_contains("rec_scores");

    struct Line {
        double y = 0.0;
        double x = 0.0;
        std::string text;
    };
    std::vector<Line> lines;
    lines.reserve(texts_value->as_array().size());

    for (std::size_t i = 0; i < texts_value->as_array().size(); ++i) {
        const auto& item = texts_value->as_array()[i];
        if (!item.is_string()) continue;
        std::string text(item.as_string().c_str());
        if (text.empty()) continue;

        // Drop low-confidence fragments; OCR noise is useless for translation.
        if (scores_value != nullptr && scores_value->is_array() &&
            i < scores_value->as_array().size() && scores_value->as_array()[i].is_number()) {
            if (scores_value->as_array()[i].to_number<double>() < 0.6) continue;
        }

        Line line;
        line.text = std::move(text);
        if (boxes_value != nullptr && boxes_value->is_array() && i < boxes_value->as_array().size()) {
            const auto& box = boxes_value->as_array()[i];
            if (box.is_array() && box.as_array().size() >= 2) {
                if (box.as_array()[0].is_number()) line.x = box.as_array()[0].to_number<double>();
                if (box.as_array()[1].is_number()) line.y = box.as_array()[1].to_number<double>();
            }
        }
        lines.push_back(std::move(line));
    }

    std::stable_sort(lines.begin(), lines.end(), [](const Line& a, const Line& b) {
        if (a.y != b.y) return a.y < b.y;
        return a.x < b.x;
    });

    std::string result;
    for (const auto& line : lines) {
        if (!result.empty()) result.push_back('\n');
        result += line.text;
    }
    return result;
}

} // namespace

struct OcrEngine::Impl {
    explicit Impl(const fs::path& root) : paddle_root(root) {
        prepare_dll_dirs(paddle_root);

        PaddleOCRParams params;
        params.text_detection_model_dir = (paddle_root / "models/PP-OCRv5_mobile_det_infer").string();
        params.text_detection_model_name = "PP-OCRv5_mobile_det";
        params.text_recognition_model_dir = (paddle_root / "models/PP-OCRv6_medium_rec_infer").string();
        params.text_recognition_model_name = "PP-OCRv6_medium_rec";
        params.use_doc_orientation_classify = false;
        params.use_doc_unwarping = false;
        params.use_textline_orientation = false;
        params.device = "cpu";

        if (const auto config = config_path(); !config.empty()) {
            params.paddlex_config = Utility::PaddleXConfigVariant(utf8_from_path(config));
        }

        ocr = std::make_unique<PaddleOCR>(params);
    }

    fs::path paddle_root;
    std::unique_ptr<PaddleOCR> ocr;
};

OcrEngine::OcrEngine(std::filesystem::path paddle_root)
    : impl_(std::make_unique<Impl>(paddle_root.empty() ? default_paddle_root() : std::move(paddle_root))) {
}

OcrEngine::~OcrEngine() = default;

std::string OcrEngine::recognize(const std::filesystem::path& image) {
    if (!std::filesystem::exists(image)) {
        throw std::runtime_error("image not found: " + utf8_from_path(image));
    }

    const auto temp_dir = working_temp_dir() /
                          ("mk-ocr-" + std::to_string(GetCurrentProcessId()) + "-" +
                           std::to_string(GetTickCount64()));
    std::filesystem::create_directories(temp_dir);

    std::string text;
    try {
        // Paddle's loader expects ANSI paths; stage the image under an ASCII name.
        const fs::path staged = temp_dir / ("input" + image.extension().string());
        fs::copy_file(image, staged, fs::copy_options::overwrite_existing);

        const auto results = impl_->ocr->Predict(staged.string());
        for (const auto& result : results) {
            result->SaveToJson(temp_dir.string());
        }

        const fs::path* json = find_json_result(temp_dir);
        if (json == nullptr) {
            throw std::runtime_error("OCR produced no result");
        }
        text = extract_text(boost::json::parse(read_text_file(*json)));
    } catch (...) {
        std::error_code ec;
        std::filesystem::remove_all(temp_dir, ec);
        throw;
    }

    std::error_code ec;
    std::filesystem::remove_all(temp_dir, ec);
    return text;
}

} // namespace mk::assist
