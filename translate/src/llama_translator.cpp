#include <translate/llama_translator.h>

#include <translate/text_postprocess.h>

#include <ggml.h>
#include <llama.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mk::translate {
namespace fs = std::filesystem;

namespace {

std::string utf8_path(const fs::path& path) {
    const auto text = path.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

std::string build_prompt(std::string_view text, const LangPair& pair) {
    std::string prompt;
    prompt.reserve(text.size() + 160);

    prompt += "<|im_start|>user\n";
    prompt += "请将以下文本翻译为";
    prompt += language_name_zh(pair.tgt);
    prompt += "，直接输出翻译结果，不要进行任何解释。\n\n";
    prompt += text;
    prompt += "<|im_end|>\n";
    prompt += "<|im_start|>assistant\n<think>\n\n</think>\n\n";
    return prompt;
}

std::vector<llama_token> tokenize(const llama_vocab* vocab, std::string_view text) {
    std::vector<llama_token> tokens(text.size() + 16);
    auto count = llama_tokenize(vocab, text.data(), static_cast<int32_t>(text.size()),
                                tokens.data(), static_cast<int32_t>(tokens.size()),
                                false, true);
    if (count < 0) {
        tokens.resize(static_cast<std::size_t>(-count));
        count = llama_tokenize(vocab, text.data(), static_cast<int32_t>(text.size()),
                               tokens.data(), static_cast<int32_t>(tokens.size()),
                               false, true);
    }
    if (count < 0) throw std::runtime_error("tokenization failed");
    tokens.resize(static_cast<std::size_t>(count));
    return tokens;
}

} // namespace

struct LlamaTranslator::Impl {
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    const llama_vocab* vocab = nullptr;
    std::vector<llama_token> stop_tokens;
    TranslateConfig config;

    ~Impl() {
        if (ctx != nullptr) llama_free(ctx);
        if (model != nullptr) llama_model_free(model);
    }
};

LlamaTranslator::LlamaTranslator(const TranslateConfig& config)
    : impl_(std::make_unique<Impl>()) {
    static std::once_flag backend_once;
    std::call_once(backend_once, [] {
        llama_log_set([](ggml_log_level, const char*, void*) {}, nullptr);
        llama_backend_init();
    });

    if (!fs::exists(config.llama_model)) {
        throw std::runtime_error("llama model not found: " + utf8_path(config.llama_model));
    }

    impl_->config = config;
    const auto started = std::chrono::steady_clock::now();

    auto model_params = llama_model_default_params();
    model_params.n_gpu_layers = config.n_gpu_layers;
    model_params.load_mode = LLAMA_LOAD_MODE_MMAP;

    impl_->model = llama_model_load_from_file(utf8_path(config.llama_model).c_str(), model_params);
    if (impl_->model == nullptr) {
        throw std::runtime_error("failed to load llama model: " + utf8_path(config.llama_model));
    }
    impl_->vocab = llama_model_get_vocab(impl_->model);

    auto context_params = llama_context_default_params();
    context_params.n_ctx = static_cast<uint32_t>(std::max(config.n_ctx, 512));
    context_params.n_batch = static_cast<uint32_t>(std::max(config.n_batch, 64));
    context_params.n_ubatch = std::min<uint32_t>(context_params.n_batch, 512);
    context_params.n_threads = std::max(config.n_threads, 1);
    context_params.n_threads_batch = context_params.n_threads;
    context_params.flash_attn_type =
        config.flash_attn ? LLAMA_FLASH_ATTN_TYPE_AUTO : LLAMA_FLASH_ATTN_TYPE_DISABLED;
    context_params.type_k = GGML_TYPE_Q8_0;
    context_params.type_v = GGML_TYPE_Q8_0;
    context_params.offload_kqv = true;

    impl_->ctx = llama_init_from_model(impl_->model, context_params);
    if (impl_->ctx == nullptr) {
        context_params.type_k = GGML_TYPE_F16;
        context_params.type_v = GGML_TYPE_F16;
        impl_->ctx = llama_init_from_model(impl_->model, context_params);
    }
    if (impl_->ctx == nullptr) {
        throw std::runtime_error("failed to create llama context");
    }

    for (const std::string_view stop : {"<|im_end|>", "<|im_start|>"}) {
        const auto tokens = tokenize(impl_->vocab, stop);
        if (tokens.size() == 1) impl_->stop_tokens.push_back(tokens.front());
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started);
    std::cerr << "[translate] llama model loaded in " << elapsed.count() << " ms\n";
}

LlamaTranslator::~LlamaTranslator() = default;

std::string LlamaTranslator::translate(std::string_view text, const LangPair& pair,
                                       const TranslateOptions& options) {
    if (text.empty()) return {};

    auto& state = *impl_;
    const auto prompt = build_prompt(text, pair);
    auto tokens = tokenize(state.vocab, prompt);

    const auto max_new = static_cast<std::size_t>(std::max(options.max_new_tokens, 1));
    if (tokens.size() + max_new > static_cast<std::size_t>(std::max(state.config.n_ctx, 512))) {
        throw std::runtime_error("input exceeds the model context window");
    }

    llama_memory_clear(llama_get_memory(state.ctx), true);
    if (llama_decode(state.ctx, llama_batch_get_one(tokens.data(), static_cast<int32_t>(tokens.size()))) != 0) {
        throw std::runtime_error("llama prompt decode failed");
    }

    const std::unique_ptr<llama_sampler, decltype(&llama_sampler_free)> sampler(
        llama_sampler_chain_init(llama_sampler_chain_default_params()), &llama_sampler_free);
    if (!sampler) throw std::runtime_error("failed to create llama sampler");
    llama_sampler_chain_add(sampler.get(), llama_sampler_init_greedy());

    std::string output;
    output.reserve(static_cast<std::size_t>(options.max_new_tokens) * 2);

    for (std::size_t generated = 0; generated < max_new; ++generated) {
        llama_token token = llama_sampler_sample(sampler.get(), state.ctx, -1);
        if (llama_vocab_is_eog(state.vocab, token)) break;
        if (std::ranges::find(state.stop_tokens, token) != state.stop_tokens.end()) break;

        char buffer[256];
        const int32_t length = llama_token_to_piece(state.vocab, token, buffer,
                                                    static_cast<int32_t>(sizeof(buffer)), 0, false);
        if (length > 0) {
            output.append(buffer, static_cast<std::size_t>(length));
        } else if (length < 0) {
            std::string piece(static_cast<std::size_t>(-length), '\0');
            const int32_t written = llama_token_to_piece(state.vocab, token, piece.data(),
                                                         static_cast<int32_t>(piece.size()), 0, false);
            if (written > 0) output.append(piece.data(), static_cast<std::size_t>(written));
        }

        if (llama_decode(state.ctx, llama_batch_get_one(&token, 1)) != 0) {
            break;
        }
    }

    return clean_translation(std::move(output));
}

} // namespace mk::translate
