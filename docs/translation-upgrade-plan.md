# 翻译 / OCR / 语音子系统设计与实施记录

> 日期：2026-11-14
> 翻译：Index-Translate-9B（GGUF Q4_K_M）+ llama.cpp 静态链接
> OCR：PaddleOCR v3.7.0 C++ API（PP-OCRv5 det + PP-OCRv6 rec）
> 语音：tvlib 音色克隆，生成后播放不落盘

## 1. 命令一览

| 命令 | 作用 |
|---|---|
| mk <src><tgt> <text> | 翻译，如 zhen / enzh / zhja |
| mk tr --to LANG [--from LANG] <text> | 通用翻译 |
| mk ocr <image> [--to LANG] [--speak] | 识别，可选翻译 / 朗读 |
| mk say <text> | 语音朗读，不生成文件 |
| mk serve | 常驻模式，模型只加载一次 |

## 2. 模块结构

```text
mk/          入口、serve.cpp（常驻循环）
assist/      ocr_engine（PaddleOCR）、tts_engine（tvlib）、utf8_path
translate/   ITranslator 与 llama 后端
command_list/ 适配层
```

OCR 与 TTS 都在 assist 里，对外只暴露两个类：

```cpp
mk::assist::OcrEngine::recognize(path) -> std::string
mk::assist::TtsEngine::speak(text)
```

## 3. PaddleOCR 接入要点

### 依赖

| 项 | 位置 |
|---|---|
| ppocr_core.lib | C:/Cpp_Libs/mt/Release/PaddleOCR/lib（45MB，/MT） |
| 模型 | PaddleOCR/models/PP-OCRv5_mobile_det_infer、PP-OCRv6_medium_rec_infer |
| 运行时 DLL | PaddleOCR/bin：paddle_inference、phi、mkldnn、mklml、common |
| abseil | C:/Cpp_Libs/mt/Release/abseil（20220623.1，/MT，C++17） |
| OpenCV | mt/Release/opencv 静态库 |
| 第三方头 | polyclipping、nlohmann 已 vendor 到 assist/vendor |

### 三个坑

1. **头文件用 C++17 才能编**：`src/common/thread_pool.h` 用了 `std::result_of`，
   C++23 下被移除。因此 `assist_ocr` 目标单独设 `CXX_STANDARD 17`，
   其余目标保持 C++23。
2. **产线配置必须显式给**：`Utility::GetDefaultConfig("OCR")` 用编译期 `__FILE__`
   反推 `.../src/configs/OCR.yaml`，机器上没这个源码树就报
   `Could not find OCR pipeline config file`。解法：从 PaddleOCR v3.7.0 仓库取
   `OCR.yaml` 放进 `assist/configs/`，构建时拷到 exe 同级 `configs/`，
   运行时通过 `params.paddlex_config` 传绝对路径。
3. **运行时库不拷贝**：用 `SetDefaultDllDirectories` + `AddDllDirectory` 把
   `PaddleOCR/bin`、`paddle_inference/paddle/lib`、`mklml/lib` 加进本进程搜索路径；
   `paddle_inference.dll` 用 `/DELAYLOAD`，只做翻译时完全不加载 Paddle。

### 链接冲突

- `ittnotify.lib`：OpenCV 与 libtorch 各带一份，去掉 OpenCV 的。
- `libprotobuf.lib`：同上，去掉 OpenCV 的。
- `openblas.lib` 是 Intel Fortran 编的，要求 `flang_rt.runtime.dynamic.lib`；
  Windows 上只缺 `_FortranACharacterCompareScalar1` 一个符号，
  `assist/vendor/flang_stub.c` 提供空实现，链接时 `/NODEFAULTLIB` 掉运行时。

## 4. tvlib 接入要点

- 直接 `add_subdirectory(Desktop/tvlib)`，用 `tvlib` + `tvlib_link_options`
  （含 /WHOLEARCHIVE 与 16MB 栈）。
- 每次 synthesize 都会重新加载模型（约 30s），所以默认 `steps=4`，
  短句约 30-60s 出音；`MK_TTS_STEPS` 可调。
- 输出到临时 wav → `PlaySoundW(SND_SYNC)` 播放 → 删除。
- 路径一律 UTF-8：`tvlib::Request` 收 UTF-8，`std::filesystem` 在 Windows 上收 ANSI，
  统一走 `assist/utf8_path.h` 转换。

## 5. 常驻模式

`mk serve` 从 stdin 逐行读命令，一个 llama 模型服务所有语言对：

```text
> zhen 人工智能正在改变世界。
Artificial intelligence is changing the world.
> zhja 你好，世界。
こんにちは、世界。
> quit
```

实测：首句加载模型 2.4s，之后每句 0.3-0.5s。

## 6. 临时目录

`working_temp_dir()` 依次尝试 `MK_TMP`、`%LOCALAPPDATA%/mk/tmp`、系统 temp、
`<cwd>/.mk-tmp`，用写入探针确认可写后返回。

## 7. 构建

```powershell
cmake -S . -B out/build/x64-installed -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/x64-installed --parallel 8
```

25 个 gtest 用例全部通过。
