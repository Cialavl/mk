# 翻译引擎设计与实施记录

> 日期：2026-11-06
> 当前实现：Index-Translate-9B（GGUF Q4_K_M）+ llama.cpp 静态链接
> 构建：MSVC /MT，Boost 与 llama.cpp 均来自 C:/Cpp_Libs

## 1. 现状

旧实现是 Opus-MT 2020（Marian，约 77M）+ CTranslate2 + SentencePiece，
模型路径写死在 addlanguage() 里，只支持 zhen/enzh 两个方向。

现在改为：

| 项 | 内容 |
|---|---|
| 模型 | Index-Translate-9B Q4_K_M（5.78 GB，Apache-2.0，150 语言） |
| 引擎 | llama.cpp（静态库，编进 mk.exe） |
| 语言对 | 任意 <src><tgt>，内置 35 个常用代码 |
| 回退 | 9B 不完整时用 index-2b；CTranslate2 后端已移除 |

选型对比（COMET-22 / WMT26 Judge 来自 Index 官方报告，自报口径）：

| 模型 | 规模 | 许可 | COMET-22 | Judge | 8GB 显存 |
|---|---|---:|---:|---:|---|
| Index-Translate-9B | 9B | Apache-2.0 | 0.8789 | 75.35 | Q4_K_M 5.78GB 可跑 |
| Hy-MT2-7B | 7B | Apache-2.0 | 0.8747 | 60.51 | 4.62GB 可跑 |
| Hy-MT2-30B-A3B | 30B MoE | Apache-2.0 | 0.8787 | 66.81 | 约 18GB，8GB 不够 |
| TranslateGemma-12B | 12B | Gemma | 0.8732 | 71.19 | Q4 约 7.3GB，偏紧 |
| NLLB-200-3.3B | 3.3B | CC-BY-NC | 2022 水平 | — | CT2 原生，已淘汰 |

选择 Index-9B 的原因：同规模分数最高、150 语言、Apache-2.0、
官方提供 llama.cpp GGUF，8GB 显存可以全量上卡。

## 2. 代码结构

~~~text
translate/
  include/translate/
    i_translator.h          翻译接口
    llama_translator.h      GGUF 后端（pimpl）
    translator_factory.h    引擎选择与模型回退
    translate_config.h      JSON 配置、模型查找
    text_chunker.h          UTF-8 长文分块
    text_postprocess.h      输出清理
    translate_types.h       语言对、语言名
  src/                      对应实现
~~~

调用链：

~~~text
mk.exe
  -> CMKCommandStrategy
       -> TranslateCommand（mk_command）
            -> AtomCmdTranslate（command_list 适配器）
                 -> ITranslator
                      -> LlamaTranslator
~~~

各层职责：

| 文件 | 职责 |
|---|---|
| mk/mk.cpp | 参数入口、异常兜底 |
| designmode/ | 命令注册，支持 zhen/enzh/tr 和 <src><tgt> |
| mk_command/ | 解析 tr 参数、调用适配器、打印错误 |
| command_list/translate_command | 读配置、启动后端、分块、终端编码输出 |
| translate/ | 纯翻译逻辑，不依赖命令行 |

## 3. 数据流

1. 解析命令名：zhen -> zh-en，zhja -> zh-ja，tr 走 --from/--to。
2. 找 model root：exe/model、resources/model、当前目录。
3. 读 config.json（Boost.JSON）；路径相对 model root。
4. 校验模型大小 expected_bytes；9B 不完整则回退 index-2b。
5. TextChunker 按 UTF-8 码点和句末标点切分，默认 1500 字符。
6. LlamaTranslator：
   - Qwen3.5 ChatML：<|im_start|>user ... <|im_start|>assistant
     加空思考块 <think></think> 关闭 thinking
   - 贪心解码，停止符 <|im_end|> / <|im_start|>
   - 清理标签、思考块、多余空行
7. 输出：终端不是 UTF-8 时转成控制台代码页，否则原样输出。

## 4. 命令行

~~~powershell
mk zhen "人工智能正在改变世界。"
mk enzh "Hello world. How are you today?"
mk zhja "你好，世界。"
mk jaen "こんにちは、世界。"
mk enfr "Hello world."

mk tr --to ja "你好，世界。"
mk tr --to en --model path\to\other.gguf "文本"
~~~

- 命令名 = 源语言代码 + 目标语言代码
- 支持 35 个常用代码；表外代码用 mk tr --to
- engine 目前只有 llama；--engine 保留参数位，传其他值会报错

## 5. 构建与依赖

依赖位置：

| 依赖 | 位置 | 说明 |
|---|---|---|
| Boost 1.91 | C:/Cpp_Libs/Boost | b2 自编，link=static runtime-link=static |
| llama.cpp | C:/Cpp_Libs/llamacpp | llama.lib + ggml*.lib，GGML_STATIC=ON |
| gtest | C:/Cpp_Libs/gtest-mt | /MT 静态 CRT，CMake 包在 lib/cmake/GTest |

CMake 关键设置：

~~~cmake
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(Boost_DIR "C:/Cpp_Libs/Boost/lib/cmake/Boost-1.91.0")
set(GTest_DIR "C:/Cpp_Libs/gtest-mt/lib/cmake/GTest")
set(llama_DIR "C:/Cpp_Libs/llamacpp/lib/cmake/llama")
find_package(GTest CONFIG REQUIRED)
find_package(llama CONFIG REQUIRED)
~~~

构建：

~~~powershell
cmake -S . -B out/build/x64-installed -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/x64-installed --parallel 8
~~~

结果：mk.exe 约 66 MB，/MT，静态 Boost + 静态 llama/ggml。
dumpbin 依赖只有 KERNEL32、ADVAPI32、cublas64_13。

## 6. 测试与性能

~~~powershell
out\build\x64-installed\test\test.exe
~~~

25 个用例：分块、语言对、后处理、命令注册、模型集成，全部通过。

实测（RTX 5060 Laptop 8GB）：

| 模型 | 输入 | 输出 | 端到端 |
|---|---|---|---|
| 2B | zhen：人工智能正在改变世界。 | Artificial intelligence is changing the world. | 1.56 s |
| 2B | enzh：Hello world. How are you today? | 你好，世界。你今天怎么样？ | 1.51 s |
| 9B | zhen：人工智能正在改变世界。 | Artificial intelligence is changing the world. | 3.64 s |
| 9B | enzh：Hello world. How are you today? | 你好，世界。你今天怎么样？ | 3.50 s |
| 9B | zhja：你好，世界。 | こんにちは、世界。 | 3.44 s |

9B 加载 2.3-2.5 秒；200 字长文峰值显存 6883 MiB。

## 7. 部署包

全静态版：

| 路径 | 大小 | 说明 |
|---|---:|---|
| dist/mk-win64-static/ | 5.5 GB | 目录，模型硬链接 |
| dist/mk-win64-static-runtime.zip | 47.7 MB | 无模型 |
| dist/mk-win64-static-full.zip | 5.74 GB | 含 9B 模型 |

包内：mk.exe、README.txt、model/config.json、model/index-9b/*.gguf。
目标机需要 NVIDIA 驱动和 CUDA 13.x 的 cublas64_13.dll / cudart64_13.dll；
不需要 VC++ Redistributable。

## 8. 关键决策与历史

- **必须换引擎**：CTranslate2 只支持编码器-解码器 Transformer，
  不支持 Qwen3.5 这类 decoder-only LLM，所以 Index/Hy-MT2 都用不了旧引擎。
- **移除 CT2**：llama 路径稳定后删掉 Ct2Translator、sentencepiece 依赖，
  包内不再需要 ctranslate2.dll / cudnn64_9.dll。
- **/MT 静态 CRT**：目标机不装 VC++ 运行库。Boost 用 b2 重编为
  link=static runtime-link=static；llama.cpp 用 CMAKE_MSVC_RUNTIME_LIBRARY。
- **Boost.Locale 未启用 ICU**：b2 检测 icu: no，conv::between 走 Win32
  后端；已验证 GBK 转换，你好 -> C4 E3 BA C3。
- **nvcc wrapper 已移除**：它只用于源码编译 llama.cpp 时的 CMake/nvcc
  参数兼容；现在 llama.cpp 独立安装，项目不再编 CUDA。
- **清理**：删掉 cmake_command、passwd_manage、空 push 命令、test.jpg、
  path 的空 cpp（改为 INTERFACE 库）和旧动态部署包。

## 9. 后续

- FLORES-200 devtest 上做 COMET-22，对比 Opus-MT / 2B / 9B。
- 可选 mk serve 常驻模式，省掉每次 2.5 秒模型加载。
- 如果需要完整 ICU locale 功能，再编静态 ICU 并重编 Boost.Locale。
