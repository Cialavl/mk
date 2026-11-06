# mk

Windows 命令行工具集：命令策略 + 翻译。翻译引擎基于 llama.cpp 和
Index-Translate-9B GGUF，静态链接，运行时只依赖系统 CUDA 的 cublas。

## 目录

| 目录 | 内容 |
|---|---|
| mk/ | 可执行入口 |
| designmode/ | 命令策略与注册 |
| mk_command/ | 命令适配层 |
| command_list/ | 具体命令实现（translation 适配器在此） |
| translate/ | 翻译核心：ITranslator、GGUF 后端、配置、分块 |
| path/ | 路径模板工具（header-only） |
| test/ | gtest 单元与集成测试 |
| resources/model/ | 模型目录与 config.json（权重不入库） |
| scripts/ | 模型下载脚本 |
| docs/ | 设计说明 |

## 依赖

- Visual Studio 2026（MSVC 19.51）
- CMake 3.24+、Ninja
- CUDA 13.x 运行库：cublas64_13.dll、cudart64_13.dll（在 PATH 中）
- Boost 1.91 静态 /MT：C:/Cpp_Libs/Boost
  - 头文件：include/boost-1_91
  - CMake 包：lib/cmake/Boost-1.91.0
- llama.cpp 静态 /MT：C:/Cpp_Libs/llamacpp
  - 库：llama.lib、ggml.lib、ggml-base.lib、ggml-cpu.lib、ggml-cuda.lib
  - CMake 包：lib/cmake/llama、lib/cmake/ggml
- gtest：优先用 third_party/googletest，本地没有时由 FetchContent 下载

Boost 和 llama.cpp 已经编好并安装，构建时直接 find_package，不需要
再编译 CUDA 源码。

## 准备模型

~~~powershell
# 9B，5.78 GB，默认主模型
pwsh -File scripts/fetch-model.ps1

# 可选：2B，1.31 GB，显存紧张或回退时用
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
KERNEL32、ADVAPI32、cublas64_13 这几个 DLL。

## 运行

命令名 = 源语言代码 + 目标语言代码：

~~~powershell
out\build\x64-installed\mk\mk.exe zhen "人工智能正在改变世界。"
# Artificial intelligence is changing the world.

out\build\x64-installed\mk\mk.exe enzh "Hello world. How are you today?"
# 你好，世界。你今天怎么样？

out\build\x64-installed\mk\mk.exe zhja "你好，世界。"
# こんにちは、世界。

out\build\x64-installed\mk\mk.exe jaen "こんにちは、世界。"
# Hello, world.

out\build\x64-installed\mk\mk.exe enfr "Hello world."
# Bonjour le monde.
~~~

支持的代码：zh en ja ko fr de es ru pt it ar th vi id hi tr nl pl cs uk
fa he bn ta te mr gu ur kk mn bo ms tl km my。表外代码用：

~~~powershell
mk.exe tr --to <code> "文本"
mk.exe tr --to en --model path\to\other.gguf "文本"
~~~

9B 加载约 2.5 秒，单次端到端约 3.5 秒，峰值显存约 6.9 GB。
9B 不完整时自动回退到 2B，并在 stderr 提示。

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
  }
}
~~~

- engine：目前只支持 llama
- expected_bytes：模型完整长度；不匹配时回退 index-2b
- n_gpu_layers=99：全部放 GPU
- 模型路径相对 model root（exe/model 或 resources/model）

## 测试

~~~powershell
out\build\x64-installed\test\test.exe
~~~

25 个用例：UTF-8 分块、语言对解析、提示词后处理、命令注册、模型集成。

## 部署包

全静态版在 dist/（已加入 .gitignore）：

| 路径 | 大小 | 说明 |
|---|---:|---|
| dist/mk-win64-static/ | 5.5 GB | 可直接运行，模型用硬链接 |
| dist/mk-win64-static-runtime.zip | 47.7 MB | 仅 exe + README + config，需自备模型 |
| dist/mk-win64-static-full.zip | 5.74 GB | 含 9B Q4_K_M，解压即用 |

包内只有 mk.exe、README.txt、model/config.json、model/index-9b/*.gguf。
目标机要求：Windows 10/11 x64、NVIDIA 驱动、CUDA 13.x 的
cublas64_13.dll / cudart64_13.dll；不需要 VC++ Redistributable。

## 设计要点

- translate/ITranslator：文本 + 语言对 -> 译文，后端可替换。
- LlamaTranslator：GGUF 加载、Qwen3.5 ChatML（空思考块关闭 thinking）、
  贪心解码、停止符、清理输出。
- TextChunker：按 UTF-8 码点和句末标点切分，默认 1500 字符。
- TranslateConfig：Boost.JSON 读取；路径相对 model root。
- 模型查找：exe/model、构建目录回退到 resources/model、当前目录。
