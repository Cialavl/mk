# mk

Windows 命令行工具集：命令策略 + 翻译 + OCR + 语音。
翻译基于 llama.cpp 与 Index-Translate-9B GGUF；OCR 基于 PaddleOCR C++ API；
语音基于 tvlib（音色克隆），全程静态 /MT 链接。

## 目录

| 目录 | 内容 |
|---|---|
| mk/ | 可执行入口、常驻模式（serve.cpp） |
| designmode/ | 命令策略与注册 |
| mk_command/ | 命令适配层 |
| command_list/ | 具体命令实现 |
| translate/ | 翻译核心：ITranslator、GGUF 后端、配置、分块 |
| assist/ | OCR（PaddleOCR）与 TTS（tvlib）封装，第三方头在 vendor/、configs/ |
| path/ | 路径模板工具（header-only） |
| test/ | gtest 单元与集成测试 |
| resources/model/ | 模型目录与 config.json（权重不入库） |
| scripts/ | 模型下载脚本 |

## 依赖

全部位于 C:/Cpp_Libs/mt/Release：

| 依赖 | 说明 |
|---|---|
| Boost 1.91 | 静态 /MT，lib/cmake/Boost-1.91.0 |
| llamacpp | llama.lib + ggml*.lib，静态 /MT |
| gtest | 静态 /MT，lib/cmake/GTest |
| abseil | 20220623.1，静态 /MT（PaddleOCR 需要） |
| PaddleOCR | ppocr_core.lib + models（PP-OCRv5 det / PP-OCRv6 rec） |
| paddle_inference | paddle 3.x，运行时 DLL 在 PaddleOCR/bin |
| opencv | 5.1 静态库（opencv_world510.lib 等） |
| tvlib | Desktop/tvlib，语音库（外部工程，CMake 里 add_subdirectory） |

CUDA 13.3 运行库、NVIDIA 驱动为运行前提。

## 构建

```powershell
cmake -S . -B out/build/x64-installed -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/x64-installed --parallel 8
```

构建后 mk.exe 旁边会自动生成 configs/OCR.yaml（PaddleOCR 的产线配置）。

## 用法

### 翻译

命令名 = 源语言 + 目标语言：

```powershell
mk zhen "人工智能正在改变世界。"
mk enzh "Hello world. How are you today?"
mk zhja "你好，世界。"
mk tr --to en --from zh "文本"
```

### OCR

```powershell
mk ocr page.png                    # 只识别
mk ocr page.png --to en            # 识别后翻译（默认 from=zh）
mk ocr page.png --from en --to zh  # 英文图转中文
mk ocr page.png --to en --speak    # 识别、翻译、朗读
```

### 语音

```powershell
mk say "你好，世界。"               # 生成后立即播放，不落盘
```

### 常驻模式

模型只加载一次，适合连续使用：

```text
mk serve
> zhen 人工智能正在改变世界。
> zhja 你好，世界。
> ocr scan.png --to en --speak
> say 你好
> quit
```

翻译命令加 --speak 后缀可朗读结果：`zhen 你好 --speak`。

## 环境变量

| 变量 | 作用 |
|---|---|
| MK_TMP | 临时目录（TTS 中间 wav、OCR 结果），默认 LOCALAPPDATA/mk/tmp |
| MK_PADDLEOCR_ROOT | PaddleOCR 安装根目录 |
| MK_PADDLEOCR_CONFIG | OCR 产线 yaml，默认 exe 同级 configs/OCR.yaml |
| MK_TTS_MODELS | 语音模型根目录 |
| MK_TTS_VOICE | 音色包目录，默认爱莉希雅_一般_平常 |
| MK_TTS_STEPS | 采样步数，默认 4（越大越慢） |

## 测试

```powershell
out/build/x64-installed/test/test.exe
```

25 个用例：分块、语言对、后处理、命令注册、模型集成。
