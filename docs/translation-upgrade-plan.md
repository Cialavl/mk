# 翻译引擎升级方案：Opus-MT → Index-Translate-9B

> 日期：2026-11-06
> 仓库：Cialavl/mk
> 环境：Windows 11 · MSVC · CMake/Ninja · CUDA 13.3 · RTX 5060 Laptop 8GB · 31GB RAM
> 结论：换模型 + 换引擎（llama.cpp），分 5 步落地，约 4.5 人日。

---

## 0. 结论

| # | 决策 | 内容 |
|---|---|---|
| 1 | 模型 | Index-Translate-9B（bilibili，2026-09，Apache-2.0，150 语言），Q4_K_M 5.78GB |
| 2 | 引擎 | llama.cpp（GGUF，CUDA），静态链接进 mk.exe；CTranslate2 不再作主引擎 |
| 3 | 兼容 | 保留 mk zhen / mk enzh 命令与 CT2 旧后端，配置切换，可随时回滚 |

**为什么必须换引擎**：CTranslate2 面向编码器-解码器 Transformer（NLLB / M2M100 / BART / T5 / Whisper），不支持 Qwen3.5、Hunyuan 这类 decoder-only LLM。2026 年最强的翻译模型全部是 LLM 架构，所以“换最强模型”必然等于“换推理引擎”。

**收益**

- 质量：FLORES COMET-22 从 Opus-MT（2020）提升到 0.8789（Index 官方报告，自报口径），WMT26 Judge 75.35。
- 语言对：2 对 → 150 语言任意互译。
- 能力：术语表、JSON/代码/占位符保留、长文、风格约束（instTrans 格式）。

**代价**

- 模型 5.78GB；首次编译 llama.cpp 约 15–30 分钟。
- 单次冷启动加载 2–4 秒（CLI 每次进程启动都付）；后续可加 serve 模式。
- 8GB 显存只剩约 1–1.5GB 给 KV/计算缓冲，参数要保守（见第 5 节）。

---

## 1. 代码扫描

### 1.1 调用链

~~~text
mk.exe (argc >= 2)
  └─ CMKCommandStrategy(cmdtype).initmap()          designmode/src/mk_command_strategy.cpp:37-58
       └─ new TranslateCommand(type)                mk_command/src/commands/translate_command.cpp:3-9
            └─ AtomCmdTranslate(type)               command_list/src/translate_command.cpp:55-89
                 ├─ get_model_root()                :149-154
                 │    └─ addlanguage(exe_dir)       :160-172   ← 写死 opus 路径
                 ├─ processor.Load / resprocessor.Load   :80-87
                 └─ set_ctranslate2(CUDA)           :93-96
  └─ execute(argv)
       └─ MkRun(text)                               :10-15
            └─ translation(text)                    :98-106
                 └─ StrPrse::strslice()             :179-191   ← 140/240 字符切分
                      └─ translationSentences()     :123-147
                           ├─ SentencePiece Encode
                           ├─ CT2 translate_batch(beam=6)
                           └─ SentencePiece Decode
~~~

### 1.2 现状

| 项 | 现状 |
|---|---|
| 模型 | Helsinki-NLP opus-mt-zh-en / en-zh（Marian，2020 年，各约 77M 参数） |
| 引擎 | CTranslate2 4.x（C:/Program Files/CTranslate2，CUDA 版，依赖 cublas64_13 + cudnn64_9） |
| 分词 | 每个方向两个 SentencePiece 模型，方向切换靠字符串 zhen / enzh |
| 模型目录 | resources/model 为空；构建时原样拷到 mk/model，因此当前 mk.exe 实际无法翻译 |
| 参数 | beam_size=6，length_penalty=1.0，max_decoding_length=500，repetition_penalty=1.1 |
| 输入 | 命令行参数拼接，非 UTF-8 时按 GBK 转码（mk_command_strategy.cpp:23-26） |

### 1.3 问题清单

| #   | 位置                                     | 问题                                | 影响                                 |
| --- | -------------------------------------- | --------------------------------- | ---------------------------------- |
| 1   | translate_command.cpp:160-172          | 模型路径硬编码为 opus-2020-07-17zhen/enzh | 换模型必须改代码                           |
| 2   | :55-89                                 | 构造时加载模型；加载失败只打印，不中断               | initmap 阶段就付加载成本；错误延后到 CT2 构造函数抛异常 |
| 3   | :93-96                                 | 设备写死 CUDA                         | 无 N 卡机器直接崩，无 CPU 回退                |
| 4   | mk_command/.../translate_command.cpp:7 | MkRun() 里又调一次 get_model_root()    | v_languages 重复 push，状态污染           |
| 5   | translate_command.cpp:179-191          | 按 UTF-16 码元 140/240 切分            | LLM 上下文被浪费；容易切断语义/术语               |
| 6   | :108-121                               | 输出只替换 ▁ 和 ⁇                       | 对 LLM 输出无意义，且可能误伤正常字符              |
| 7   | translate_command.h:52 + cpp:174-177   | 裸指针 new/delete                    | 异常路径泄漏或双删风险                        |
| 8   | command_list/CMakeLists.txt:39-40      | 强依赖 CTranslate2::ctranslate2      | 引擎不可替换                             |
| 9   | test/src/test.cpp:180-185, 226-231     | 集成测试只检查 model 目录存在                | 模型缺失时全部 skip，CI 覆盖为空               |
| 10  | mk/mk.cpp:8-10                         | 参数少于 2 个直接 error                  | 没有 --help / --config 入口            |

---

## 2. 模型选型

### 2.1 候选对比

分数来自 Index-Translate 技术报告（2026-09，arXiv:2609.40181）的同一套评测；COMET-22 为 0–1，WMT26 Judge 为 0–100。除 Opus 外均为他方/官方自报，需自测复核。

| 模型                         | 发布      | 规模/架构        | 许可         |  语言 | FLORES COMET-22 | WMT26 Judge | 8GB 显存          | 判定             |
| -------------------------- | ------- | ------------ | ---------- | --: | --------------: | ----------: | --------------- | -------------- |
| **Index-Translate-9B**     | 2026-09 | 9B Qwen3.5   | Apache-2.0 | 150 |      **0.8789** |   **75.35** | Q4_K_M 5.78GB ✅ | **主选**         |
| Index-Translate-2B         | 2026-09 | 2B Qwen3.5   | Apache-2.0 | 150 |          0.8655 |       60.26 | 1.31GB ✅        | 轻量档/兜底         |
| Hy-MT2-7B                  | 2026-05 | 7B Hunyuan   | Apache-2.0 |  33 |          0.8747 |       60.51 | 4.62GB ✅        | 备选（官方 GGUF 成熟） |
| Hy-MT2-30B-A3B             | 2026-05 | 30B MoE      | Apache-2.0 |  33 |          0.8787 |       66.81 | 约 18GB ❌        | 24GB 卡才考虑      |
| TranslateGemma-12B         | 2026-01 | 12B Gemma3   | Gemma      |  多语 |          0.8732 |       71.19 | 约 7.3GB ⚠️      | 许可/显存不占优       |
| NLLB-200-3.3B              | 2022    | 3.3B enc-dec | CC-BY-NC   | 200 |      未列入，明显低于上表 |           — | int8 3.5GB ✅    | 仅作 CT2 回退      |
| Opus-MT（现状）                | 2020    | 77M Marian   | CC-BY      | 1 对 |          基准，待实测 |           — | 0.3GB ✅         | 被替换            |
| GPT-5.6-Sol（API）           | 2026    | 闭源           | 商用         |  多语 |          0.8650 |   **89.10** | 0 ✅             | 云端最强           |
| DeepSeek-V4.1-Flash（API）   | 2026    | 闭源           | 商用         |  多语 |          0.8762 |       83.55 | 0 ✅             | 云端性价比          |
| Gemini 3.5 Flash Lite（API） | 2026    | 闭源           | 商用         |  多语 |          0.8750 |       79.52 | 0 ✅             | 云端低延迟          |

### 2.2 为什么是 Index-Translate-9B

1. **8GB 显存能全量上卡**：Q4_K_M 5.78GB，4k 上下文 + q8 KV 后约 6.5–7GB，留出余量。
2. **当前开源同规模最强**：FLORES COMET 0.8789，高于 Hy-MT2-7B、TranslateGemma-12B；Judge 75.35 大幅领先。
3. **150 语言**：一次接入，后续加语言只改映射表，不用再换模型。
4. **Apache-2.0**：商用/分发无额外约束；官方提供 llama.cpp GGUF（2026-10 master 转换）。
5. **指令翻译**：术语、格式、JSON/代码保留，正好补上 Opus 的短板。

### 2.3 排除项

- **Hy-MT2-30B-A3B**：质量接近，但 Q4 约 18GB，8GB 卡只能 CPU/MoE 卸载，交互体验差。
- **Qwen-MT-Lumen（WMT26）**：14.8B、30GB bf16，且 23 个方向里没有 zh→en（只有 en→zh、zh→ja）。
- **North-Small-Translate**：218B-A25B，Hugging Face 需申请权限，无法本地部署。
- **TranslateGemma-12B**：Q4 约 7.3GB 几乎占满 8GB，分数还低于 9B Index，Gemma 许可也更严格。
- **NLLB-200**：虽然 CT2 原生支持，但 2022 年模型，中英质量明显落后，保留作回退。
- **API**：质量上限最高（GPT-5.6-Sol Judge 89.10），但依赖网络/Key/费用，作为增强路线而非默认。

> 注意：上表是“同一评测集”的横向对比，Index 的分数由其团队自报。落地前必须用本项目自己的语料 A/B（第 7 节 P3）。

---

## 3. 目标架构

~~~text
mk.exe
  └─ CMKCommandStrategy
       └─ TranslateCommand
            └─ std::unique_ptr<ITranslator>        ← 新接口
                 ├─ LlamaTranslator   (默认)       GGUF + llama.cpp
                 ├─ Ct2Translator     (回退)       NLLB / 旧 Opus
                 └─ ApiTranslator     (可选)       DeepL / Qwen-MT / Gemini
~~~

**职责划分**

- ITranslator：只负责“一段文本 + 语言对 → 译文”，不关心模型格式。
- LlamaTranslator：加载 GGUF、拼 ChatML、贪心解码、清理输出。
- Ct2Translator：包装现有 SentencePiece + CT2 代码，保持 zhen/enzh 可用。
- TranslateConfig：从 model/config.json 读引擎、路径、显存参数。
- TranslateCommand：解析 CLI，选后端，输出（编码转换沿用现逻辑）。

**模型目录**

~~~text
resources/model/
  config.json
  index-9b/Index-Translate-9B.Q4_K_M.gguf      5.78GB
  nllb/nllb-200-3.3B-int8/                     可选回退
  opus-2020-07-17zhen/                         旧模型保留
  opus-2020-07-17enzh/
~~~

---

## 4. 改造清单

| 文件 | 动作 | 约行数 |
|---|---|---:|
| command_list/include/translate_command/translator.h | 新增 ITranslator / LangPair / TranslateOptions | 60 |
| command_list/include/translate_command/llama_translator.h | 新增类声明 | 50 |
| command_list/src/llama_translator.cpp | 新增 llama.cpp 实现（加载/提示/解码/清理） | 260 |
| command_list/include/translate_command/translate_config.h | 新增配置结构 + JSON 读取 | 60 |
| command_list/src/translate_config.cpp | 新增 Boost.JSON 解析 | 80 |
| command_list/src/translate_command.cpp | 改：删硬编码，改持 ITranslator；保留 StrPrse | -120/+70 |
| command_list/include/translate_command/translate_command.h | 改：成员换成 unique_ptr | -15/+10 |
| mk_command/src/commands/translate_command.cpp | 改：去掉重复 get_model_root | -3 |
| designmode/src/mk_command_strategy.cpp | 改：支持 src-tgt / --engine 解析 | +40 |
| CMakeLists.txt | 改：3.24、CUDA 120、FetchContent llama.cpp | +20 |
| command_list/CMakeLists.txt | 改：链接 llama，条件链接 CT2 | +5 |
| test/src/test.cpp | 改：模型检查路径；新增 prompt/后处理单测 | +60 |
| scripts/fetch-model.ps1 | 新增：hf-mirror + aria2c 下载校验 | 60 |

**不做（本次范围外）**：图片翻译（mmproj）、语音、流式输出、术语表 UI、API 后端。先把文本主链路跑通。

---

## 5. 关键实现

### 5.1 接口

~~~cpp
// translator.h
struct LangPair { std::string src; std::string tgt; };

struct TranslateOptions {
    int max_new_tokens = 2048;
    int max_input_chars = 1500;
    bool greedy = true;
};

class ITranslator {
public:
    virtual ~ITranslator() = default;
    virtual std::string translate(std::string_view text,
                                  const LangPair& pair,
                                  const TranslateOptions& opt) = 0;
    virtual const char* name() const noexcept = 0;
};

std::unique_ptr<ITranslator> make_translator(const TranslateConfig& cfg);
~~~

### 5.2 LlamaTranslator

~~~cpp
// llama_translator.h（骨架）
class LlamaTranslator final : public ITranslator {
public:
    explicit LlamaTranslator(const TranslateConfig& cfg);
    ~LlamaTranslator() override;

    std::string translate(std::string_view text, const LangPair& pair,
                          const TranslateOptions& opt) override;
    const char* name() const noexcept override { return "llama.cpp"; }

private:
    std::string build_prompt(std::string_view text, const LangPair& pair) const;
    std::string generate(std::string_view prompt, const TranslateOptions& opt);
    std::string postprocess(std::string s) const;

    struct ModelDeleter { void operator()(llama_model* m) const { llama_model_free(m); } };
    struct CtxDeleter   { void operator()(llama_context* c) const { llama_free(c); } };
    std::unique_ptr<llama_model, ModelDeleter> model_;
    std::unique_ptr<llama_context, CtxDeleter> ctx_;
    const llama_vocab* vocab_ = nullptr;
    TranslateConfig cfg_;
};
~~~

初始化（GPU 层数、KV 量化、4k 上下文）：

~~~cpp
llama_backend_init();

auto mp = llama_model_default_params();
mp.n_gpu_layers = cfg_.n_gpu_layers;     // 99 = 全部上卡
mp.use_mmap     = true;
model_.reset(llama_model_load_from_file(cfg_.model_path.c_str(), mp));
if (!model_) throw std::runtime_error("load model failed: " + cfg_.model_path);
vocab_ = llama_model_get_vocab(model_.get());

auto cp = llama_context_default_params();
cp.n_ctx     = cfg_.n_ctx;               // 4096
cp.n_batch   = cfg_.n_batch;             // 512
cp.n_threads = cfg_.n_threads;           // 8
cp.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_AUTO;
cp.type_k = GGML_TYPE_Q8_0;
cp.type_v = GGML_TYPE_Q8_0;
ctx_.reset(llama_init_from_model(model_.get(), cp));
if (!ctx_) throw std::runtime_error("create context failed");
~~~

解码循环（贪心，遇 EOG 停）：

~~~cpp
std::string LlamaTranslator::generate(std::string_view prompt, const TranslateOptions& opt) {
    std::vector<llama_token> tokens(prompt.size() + 32);
    int n = llama_tokenize(vocab_, prompt.data(), (int)prompt.size(),
                           tokens.data(), (int)tokens.size(),
                           /*add_special=*/false, /*parse_special=*/true);
    if (n < 0) {
        tokens.resize(-n);
        n = llama_tokenize(vocab_, prompt.data(), (int)prompt.size(),
                           tokens.data(), (int)tokens.size(), false, true);
    }
    if (n <= 0) throw std::runtime_error("tokenize failed");
    tokens.resize(n);

    llama_memory_clear(llama_get_memory(ctx_.get()), true);
    if (llama_decode(ctx_.get(), llama_batch_get_one(tokens.data(), n)) != 0)
        throw std::runtime_error("decode prompt failed");

    llama_sampler* smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(smpl, llama_sampler_init_greedy());

    std::string out;
    for (int i = 0; i < opt.max_new_tokens; ++i) {
        llama_token id = llama_sampler_sample(smpl, ctx_.get(), -1);
        if (llama_vocab_is_eog(vocab_, id)) break;
        char buf[256];
        int len = llama_token_to_piece(vocab_, id, buf, sizeof(buf), 0, true);
        if (len > 0) out.append(buf, len);
        if (llama_decode(ctx_.get(), llama_batch_get_one(&id, 1)) != 0) break;
    }
    llama_sampler_free(smpl);
    return postprocess(std::move(out));
}
~~~

### 5.3 提示词：手写 ChatML 并关闭 thinking

官方 GGUF 要求用 chat template + enable_thinking=false。llama.cpp 的 C API 不做 Jinja 渲染，所以直接按 tokenizer_config.json 里的模板手写。模板的这条分支是（\n 表示换行符）：

~~~text
<|im_start|>assistant\n<think>\n\n</think>\n\n
~~~

翻译时照抄即可（空思考块 = 不推理，直接出译文）：

~~~cpp
std::string LlamaTranslator::build_prompt(std::string_view text, const LangPair& pair) const {
    static const std::unordered_map<std::string, std::string> name{
        {"zh","中文"},{"en","英文"},{"ja","日语"},{"ko","韩语"},{"fr","法语"},
        {"de","德语"},{"es","西班牙语"},{"ru","俄语"},{"pt","葡萄牙语"},
        {"it","意大利语"},{"ar","阿拉伯语"},{"th","泰语"},{"vi","越南语"}
    };
    const std::string tgt = name.count(pair.tgt) ? name.at(pair.tgt) : pair.tgt;

    std::string user = "请将以下文本翻译为" + tgt +
        "，直接输出翻译结果，不要进行任何解释。\n\n" + std::string(text);

    return "<|im_start|>user\n" + user + "<|im_end|>\n"
           "<|im_start|>assistant\n<think>\n\n</think>\n\n";
}
~~~

- 停止符：<|im_end|>（248046）、<|endoftext|>（248044）；llama_vocab_is_eog 已覆盖 EOS。
- 采样：温度 0 / 贪心，与官方客户端默认一致。
- 若切换 Hy-MT2：提示词换成官方模板（“将以下文本翻译为 X …”），不要用 ChatML。

### 5.4 分块与后处理

分块（替代现有 140/240 字符逻辑）：

1. 先按空行分段落；单段超过 1500 字符再按 。！？.!? 切句。
2. 单句仍超限时按 UTF-8 码点硬切，绝不切在多字节字符中间。
3. 每块独立翻译，保留原有换行结构，最后按顺序拼接。

后处理：

- trim 首尾空白；
- 若模型仍输出思考块，整段删除（保险）；
- 去掉开头的“译文：/翻译：”；
- 输出/源文长度比 <0.3 或 >3 时记日志，可用 repetition_penalty=1.1 重试一次；
- 不再对 LLM 输出做 ▁/⁇ 替换（仅 CT2 后端保留）。

### 5.5 CMake

根 CMakeLists.txt：

~~~cmake
cmake_minimum_required(VERSION 3.24)
project("mk")

set(CMAKE_CUDA_ARCHITECTURES 120 CACHE STRING "" FORCE)   # RTX 5060 = sm_120
set(GGML_CUDA ON CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(LLAMA_CURL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  llama_cpp
  GIT_REPOSITORY https://github.com/ggml-org/llama.cpp
  GIT_TAG        <pin-2026-10-master-sha>   # 必须支持 qwen3_5（官方 GGUF 用 2026-10 master 转换）
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(llama_cpp)
~~~

command_list/CMakeLists.txt：

~~~cmake
target_link_libraries(command_list PUBLIC llama)
# CT2 只给回退后端用；确认不再需要时再删
target_link_libraries(command_list PUBLIC CTranslate2::ctranslate2)
~~~

> 若 configure 报 CUDA 语言未启用，在 project() 后加 enable_language(CUDA)。
> CMAKE_CUDA_ARCHITECTURES=120 需要较新的 CMake（3.31+）；旧版本会报 Invalid CUDA architecture，改用 120-real 或升级 CMake。
> 若 GitHub 拉取失败：先试 gitee 镜像/代理；再退到预编译 llama.dll + import lib；最后才考虑 sidecar。

### 5.6 配置与 CLI

model/config.json：

~~~json
{
  "engine": "llama",
  "model_path": "model/index-9b/Index-Translate-9B.Q4_K_M.gguf",
  "n_ctx": 4096,
  "n_batch": 512,
  "n_gpu_layers": 99,
  "n_threads": 8,
  "max_new_tokens": 2048,
  "fallback": {
    "engine": "ct2",
    "model_path": "model/opus-2020-07-17zhen"
  }
}
~~~

CLI 兼容与扩展：

~~~text
mk zhen "你好世界"                  # 兼容旧命令
mk enzh "Hello world"               # 兼容旧命令
mk tr --from zh --to en "你好"      # 新：任意语言对
mk tr --engine ct2 --to en "你好"   # 新：强制回退
mk tr --model <gguf> --to ja "你好" # 新：临时换模型
~~~

策略层把 zhen/enzh 映射成 {zh,en}/{en,zh} 再交给 ITranslator，避免再写死方向分支。

---

## 6. 模型准备

### 6.1 下载（先镜像）

~~~powershell
$env:HF_ENDPOINT = "https://hf-mirror.com"
New-Item -ItemType Directory -Force resources/model/index-9b | Out-Null
aria2c -x 8 -s 8 -c -d resources/model/index-9b -o Index-Translate-9B.Q4_K_M.gguf "https://hf-mirror.com/IndexTeam/Index-Translate-9B-GGUF/resolve/main/Index-Translate-9B.Q4_K_M.gguf?download=true"
aria2c -x 8 -s 8 -c -d resources/model/index-9b -o SHA256SUMS.txt "https://hf-mirror.com/IndexTeam/Index-Translate-9B-GGUF/resolve/main/SHA256SUMS.txt?download=true"
~~~

校验：certutil -hashfile 对比 SHA256SUMS.txt。直连失败再按 7897 代理；不要写持久化代理变量。

备选轻量（1.31GB，先打通管线）：

~~~powershell
aria2c -x 8 -s 8 -c -d resources/model/index-2b -o Index-Translate-2B.Q4_K_M.gguf "https://hf-mirror.com/IndexTeam/Index-Translate-2B-GGUF/resolve/main/Index-Translate-2B.Q4_K_M.gguf?download=true"
~~~

### 6.2 目录与版本控制

- 模型放 resources/model，.gitignore 加 resources/model/**/*.gguf 和 *.safetensors。
- 不要用 POST_BUILD copy_directory 拷 6GB（每次构建都扫盘）；改为首次运行按配置路径找模型，或只在文件不存在时拷贝。
- 下载脚本放 scripts/fetch-model.ps1，提交脚本不提交权重。

### 6.3 冷启动

CLI 每次启动加载 5.78GB（mmap，约 1–3 秒）。可接受就先接受；要优化再加：

- mk serve：常驻进程 + 命名管道（Boost.Asio），CLI 转发；
- 或 llama-server sidecar + Boost.Beast HTTP。

---

## 7. 实施计划

| 阶段 | 内容 | 产出 | 工作量 |
|---|---|---|---:|
| P0 可行性 | 下载 Q4_K_M；用预编译 llama-cli 跑 20 句中英；测 tok/s、显存、冷启动 | 实测数据，确认 qwen3_5 支持 | 0.5 天 |
| P1 引擎接入 | ITranslator + LlamaTranslator + CMake 链接 | mk zhen/enzh 能出新译文 | 1.5 天 |
| P2 管线 | prompt/分块/后处理/配置/CLI/回退 | 稳定可用 | 1 天 |
| P3 测试评测 | gtest + FLORES-200 + COMET + 人工 50 句 | 对比报告 | 1 天 |
| P4 打包 | 模型目录、下载脚本、文档、回滚开关 | 可发布 | 0.5 天 |

**MVP 路线（1.5 天）**：不做抽象和配置，直接把 translate_command.cpp 里的 CT2 调用换成 llama.cpp，提示词写死，先验证质量；再补 P2。

P0 冒烟命令：

~~~powershell
llama-cli -m resources/model/index-9b/Index-Translate-9B.Q4_K_M.gguf --jinja --chat-template-kwargs '{\"enable_thinking\":false}' -ngl 99 -c 4096 --temp 0 -p "请将以下文本翻译为英文，直接输出翻译结果，不要进行任何解释。\n\n人工智能正在改变世界。"
~~~

---

## 8. 验收标准

| 维度 | 指标 | 方法 |
|---|---|---|
| 质量 | zh→en COMET-22 ≥ 0.87；en→zh ≥ 0.86；相对 Opus 基线至少 +0.02 | FLORES-200 devtest 1012 句 + COMET-22 |
| 质量 | 术语/数字/代码/占位符保留率 100% | 50 句构造集 |
| 性能 | 首 token < 1s；decode ≥ 30 tok/s；100 字 < 3s | 计时脚本，10 次取中位数 |
| 显存 | 峰值 < 7.5GB | nvidia-smi 采样 |
| 稳定 | 1 万字文本不 OOM；空串/emoji/超长句不崩 | 边界用例 |
| 兼容 | mk zhen / mk enzh 参数与输出格式不变 | 现有 gtest + 手工 |
| 回退 | engine=ct2 一键切回，旧模型仍可跑 | 配置切换验证 |

---

## 9. 风险与对策

| 风险 | 影响 | 对策 |
|---|---|---|
| llama.cpp 版本不支持 qwen3_5 | 加载失败 | P0 先验证；pin 官方 GGUF 同期的 2026-10 master；退 Hy-MT2-7B |
| 8GB 显存 OOM | 崩溃/降速 | Q4_K_M + 4k ctx + KV q8；关闭浏览器/游戏；OOM 自动降级 2B 或 CPU |
| 量化掉点 | 译文质量下降 | 50 句对比 Q4_K_M vs Q5_K_S；不行换 Q5（6.47GB，注意余量） |
| 自报分数偏差 | 期望落空 | 用自有语料 A/B，不只看榜单 |
| 长文截断/漏译 | 内容缺失 | 1500 字分块；长度比校验；重试 |
| 提示注入（源文含指令） | 输出跑偏 | 固定模板 + 分隔符；输出长度/语言检测 |
| 构建时间长/网络失败 | 阻塞 | 预编译 sidecar 兜底；gitee 镜像/代理 |
| 冷启动 2–4s | CLI 体验下降 | 先接受；后续 mk serve |
| 仓库体积 | clone 变慢 | .gitignore 权重；LFS 不需要 |

---

## 10. 备选路线

**A. 留在 CTranslate2（最小改动）**
换 NLLB-200-3.3B（int8 3.5GB，200 语言），继续用现有 CT2 管线，只改分词与 target_prefix。约 0.5–1 天。质量是 2022 水平，明显低于 Index；适合作为回退后端。

**B. 云 API（质量上限最高）**
DeepL / Qwen-MT / Gemini 3.5 Flash Lite（Judge 79.52）/ GPT-5.6-Sol（89.10）。零显存、无构建，C++ 用 Boost.Beast 发 HTTPS 即可。代价：联网、Key、按量计费、隐私。约 0.5–1 天。若“最强”指绝对质量且不介意联网，这条才是答案。

**C. 渐进替换**
先上 Index-Translate-2B（1.31GB）打通 llama.cpp 管线，验证提示词/分块/打包，再换 9B 文件。风险最低，多花半天。

---

## 11. 回滚

1. 改动前打 tag：git tag pre-index-9b。
2. 配置 engine 改 ct2 即回旧管线；旧 opus 模型目录保留。
3. 新代码只新增文件 + 薄改 TranslateCommand；CT2 实现原样保留，随时可切。
4. 若 llama.cpp 构建失败，删 FetchContent 段与 target_link_libraries(llama) 即恢复原构建。

---

## 12. 需要你确认的三件事

1. **接受 5.78GB 模型 + 首次 15–30 分钟编译吗？** 不接受就走第 10 节 B（云 API），质量上限更高。
2. **CLI 是否扩成 mk tr --from/--to（150 语言）？** 还是先只保 zhen/enzh，内部再抽象。
3. **是否保留 CT2 回退？** 保留要多约 1 天联调；不保留可以删掉 SentencePiece/CT2 依赖，构建更干净。

---

## 13. 实施记录（2026-11-06）

### 已完成

- 新增 translate/ 组件：ITranslator、LlamaTranslator、Ct2Translator、
  TranslatorFactory、TranslateConfig、TextChunker、clean_translation。
- command_list/translate_command 改为薄适配器；mk_command 与 designmode
  的裸指针 map 改为 unique_ptr，execute 增加未知命令检查。
- CMake 升到 C++23；translate 独立 target；llama.cpp 以导入库方式链接；
  mk/test 构建后自动拷贝 llama 运行库。
- 测试重写为 23 个用例（分块、语言对、后处理、注册、模型集成），全部通过。

### 实测（RTX 5060 Laptop 8GB）

2B Q4_K_M：

| 输入 | 输出 | 端到端耗时 |
|---|---|---|
| zhen：人工智能正在改变世界。 | Artificial intelligence is changing the world. | 1.56 s |
| enzh：Hello world. How are you today? | 你好，世界。你今天怎么样？ | 1.51 s |

9B Q4_K_M（5.78 GB）：

| 输入 | 输出 | 端到端耗时 |
|---|---|---|
| zhen：人工智能正在改变世界。 | Artificial intelligence is changing the world. | 3.64 s |
| enzh：Hello world. How are you today? | 你好，世界。你今天怎么样？ | 3.50 s |
| tr --to ja：你好，世界。 | こんにちは、世界。 | 3.44 s |

9B 加载 2.3-2.4 s；200 字长文峰值显存 6883 MiB；23 个测试全部通过。

### 与原方案的偏差

1. **llama.cpp 使用官方预编译包（b12144, CUDA 13.4）+ 生成 llama.lib**，
   而不是 FetchContent 源码构建。原因：CMake 4.3.1 + CUDA 13.3 + Ninja
   会把依赖参数 -MD 包成 -Xcompiler，nvcc 报
   "--dependency-file requires -M/-MD" 或 "single input file"。
   后续改为把 llama.cpp 独立编译成静态库安装到 C:\Cpp_Libs\llamacpp，
   项目只 find_package 链接，不再源码构建。
2. **命令行统一为 <src><tgt>**：mk zhja / mk jaen / mk enfr 与
   zhen/enzh 同风格；表外代码和 --engine/--model 覆盖仍走 mk tr。
3. 9B 模型已下载并校验（5,780,090,304 字节，SHA256 9cc6758b...），
   放入 resources/model/index-9b。expected_bytes 仍用于防止加载半截文件，
   不完整时自动回退 2B。

### 依赖静态化（2026-11-06 晚）

- Boost 1.91 用 b2 自编：link=static、runtime-link=static，安装到
  C:\Cpp_Libs\Boost（versioned layout，lib/cmake/Boost-1.91.0 可用）。
- llama.cpp 静态 /MT 安装到 C:\Cpp_Libs\llamacpp（GGML_STATIC=ON）。
- mk 改为 find_package(Boost CONFIG) + find_package(llama CONFIG)，
  CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded，构建目录 out/build/x64-installed。
- mk.exe 依赖只剩 KERNEL32、ADVAPI32、cublas64_13；25 个测试全部通过。
- Boost.Locale 未启用 ICU（b2 检测 icu: no），conv::between 走 Win32
  后端；已用 GBK 转换验证（你好 -> C4 E3 BA C3）。

### 下一步

- 用 FLORES-200 devtest 做 COMET-22 基线对比（Opus-MT vs 2B vs 9B）。
- 视需要补 mk serve 常驻模式（省掉每次 2.5 s 模型加载）。
- 视质量/显存再调 Q4_K_M 与 Q5_K_M。

---

## 附录：参考

- Index-Translate-9B：https://huggingface.co/IndexTeam/Index-Translate-9B
- 官方 GGUF（Q4_K_M 5.78GB）：https://huggingface.co/IndexTeam/Index-Translate-9B-GGUF
- 代码/提示词参考：https://github.com/bilibili/Index-Translate
- 技术报告：https://arxiv.org/abs/2609.40181
- Hy-MT2-7B（备选）：https://huggingface.co/tencent/Hy-MT2-7B
- llama.cpp：https://github.com/ggml-org/llama.cpp
- CTranslate2 模型支持：https://opennmt.net/CTranslate2/