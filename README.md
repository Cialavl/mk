# mk

Windows 命令行工具集：命令策略 + 翻译。翻译子系统已重构为
llama.cpp + Index-Translate GGUF，保留 CTranslate2 回退后端。

## 目录

| 目录 | 内容 |
|---|---|
| mk/ | 可执行入口 |
| designmode/ | 命令策略与注册 |
| mk_command/ | 命令适配层 |
| command_list/ | 具体命令实现（translation 适配器在此） |
| translate/ | 翻译核心：ITranslator、GGUF/CT2 后端、配置、分块 |
| path/ | 路径模板工具 |
| test/ | gtest 单元与集成测试 |
| resources/model/ | 模型目录与 config.json（权重不入库） |
| scripts/ | 模型与 llama.cpp 下载脚本 |
| tools/ | nvcc wrapper（源码编译 llama.cpp 时用） |
| docs/ | 设计与升级方案 |

## 依赖

- Visual Studio 2026（MSVC 19.51）
- CMake 3.24+、Ninja
- CUDA 13.3 运行库（运行时需要 cublas64_13.dll；cudart 已静态编入）
- Boost 1.91 静态 /MT：C:/Cpp_Libs/Boost
  - 头文件：include/boost-1_91
  - CMake 包：lib/cmake/Boost-1.91.0
- llama.cpp 静态 /MT：C:/Cpp_Libs/llamacpp
  - 库：llama.lib、ggml*.lib
  - CMake 包：lib/cmake/llama、lib/cmake/ggml

## 准备依赖

llama.cpp 和 Boost 已经编好并安装在 C:/Cpp_Libs，构建时直接
find_package 使用，不再需要 CUDA 源码编译。

模型（Q4_K_M）：

~~~powershell
# 9B，5.78 GB，默认主模型
pwsh -File scripts/fetch-model.ps1

# 可选：2B，1.31 GB，显存紧张或先验证管线时用
pwsh -File scripts/fetch-model.ps1 -Repo IndexTeam/Index-Translate-2B-GGUF -File Index-Translate-2B.Q4_K_M.gguf -Dest resources/model/index-2b -ExpectedSize 1312164352 -Sha256 044b313d29342bd3b2c77cbb64023ca0d209bd9b3247763f9d162767ef2d746a
~~~

脚本默认走 hf-mirror，断点续传并校验 SHA256。

## 构建

在 "Developer PowerShell for VS 2026" 中执行：

~~~powershell
cmake -S . -B out/build/x64-installed -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/x64-installed --parallel 8
~~~

CMake 里固定：

~~~cmake
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(Boost_DIR "C:/Cpp_Libs/Boost/lib/cmake/Boost-1.91.0")
set(llama_DIR "C:/Cpp_Libs/llamacpp/lib/cmake/llama")
~~~

所以 mk.exe 是 /MT + 静态 Boost + 静态 llama/ggml，运行时只剩
KERNEL32、ADVAPI32、cublas64_13 这几个系统 DLL。

## 运行

命令名 = 源语言代码 + 目标语言代码，和 zhen / enzh 完全一致：

~~~powershell
out\build\x64-prebuilt\mk\mk.exe zhja "你好，世界。"
# こんにちは、世界。

out\build\x64-prebuilt\mk\mk.exe jaen "こんにちは、世界。"
# Hello, world.

out\build\x64-prebuilt\mk\mk.exe enfr "Hello world."
# Bonjour le monde.

out\build\x64-prebuilt\mk\mk.exe zhen "人工智能正在改变世界。"
# Artificial intelligence is changing the world.

out\build\x64-prebuilt\mk\mk.exe enzh "Hello world. How are you today?"
# 你好，世界。你今天怎么样？
~~~

支持的代码：zh en ja ko fr de es ru pt it ar th vi id hi tr nl pl cs uk
fa he bn ta te mr gu ur kk mn bo ms tl km my。表外代码用
mk tr --to <code>，需要临时换引擎/模型时加 --engine 或 --model。

9B 加载约 2.5 秒，单次端到端约 3.5 秒，峰值显存约 6.9 GB。
9B 未下载完时自动回退到 2B 并提示；也可用 --model 临时指定其他 GGUF。

## 配置

resources/model/config.json：

~~~json
{
  "engine": "llama",
  "llama": {
    "model": "index-9b/Index-Translate-9B.Q4_K_M.gguf",
    "expected_bytes": 5780090304,
    "n_ctx": 4096,
    "n_batch": 2048,
    "n_gpu_layers": 99,
    "n_threads": 8,
    "flash_attn": true,
    "max_new_tokens": 2048
  },
  "ct2": { "device": "cuda", "zhen": { }, "enzh": { } }
}
~~~

engine 改为 ct2 即回退到旧的 SentencePiece + CTranslate2 管线。
expected_bytes 用于判断模型是否下载完整；不完整时用 index-2b。

## 测试

~~~powershell
out\build\x64-prebuilt\test\test.exe
~~~

覆盖：UTF-8 分块、语言对解析、提示词后处理、命令注册、模型集成。

## 部署包

构建产物在 dist/（已加入 .gitignore）：

| 路径 | 大小 | 说明 |
|---|---:|---|
| dist/mk-win64/ | 6.0 GB | 可直接运行，模型用硬链接（复制时按真实文件展开） |
| dist/mk-win64-runtime.zip | 153 MB | 仅 exe + DLL + config，需自备 model/index-9b |
| dist/mk-win64-full.zip | 5.85 GB | 含 9B Q4_K_M 模型，解压即用 |

包内包含：mk.exe、llama/ggml 运行库、ctranslate2.dll、libomp/libiomp5md、
config.json 和模型。

按约定不打包系统组件：MSVC 运行库（msvcp140/vcruntime140）、UCRT、
CUDA 13.x 的 cublas64_13 / cublasLt64_13 / cudart64_13 / cudnn64_9。
目标机需已安装这些运行库，并在 PATH 中能找到 CUDA 的 bin 目录。

## 设计要点

- translate/ITranslator：文本 + 语言对 -> 译文，后端可替换。
- LlamaTranslator：GGUF 加载、Qwen3.5 ChatML（空思考块关闭 thinking）、
  贪心解码、停止符、清理输出。
- Ct2Translator：包装原有 SentencePiece + CTranslate2 逻辑。
- TextChunker：按 UTF-8 码点和句末标点切分，默认 1500 字符。
- TranslateConfig：Boost.JSON 读取；路径相对 model root。
- 模型查找：exe/model、构建目录回退到 resources/model、当前目录。
