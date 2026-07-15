#include <gtest/gtest.h>

#include <commands/translate_command.h>
#include <mk_command_strategy/mk_command_strategy.h>
#include <translate_command/translate_command.h>

#include <boost/algorithm/string.hpp>
#include <filesystem>

// ============================================================
// StrPrse::truncateIntoSentencesUtf8 单元测试
// ============================================================

TEST(StrPrseTest, EmptyInput) {
    atomizationCmd_translate::StrPrse parser("", "enzh");
    auto result = parser.truncateIntoSentencesUtf8("", 240);
    EXPECT_TRUE(result.empty());
}

TEST(StrPrseTest, ShortTextWithinLimit) {
    atomizationCmd_translate::StrPrse parser("", "enzh");
    auto result = parser.truncateIntoSentencesUtf8("Hello world", 240);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "Hello world");
}

TEST(StrPrseTest, SplitAtEnglishPeriod) {
    std::string text = std::string(240, 'a') + ". rest of the sentence";
    atomizationCmd_translate::StrPrse parser("", "enzh");
    auto result = parser.truncateIntoSentencesUtf8(text, 240);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], std::string(240, 'a') + ".");
    EXPECT_EQ(result[1], " rest of the sentence");
}

TEST(StrPrseTest, SplitAtExclamationMark) {
    std::string text = std::string(240, 'a') + "! after exclamation";
    atomizationCmd_translate::StrPrse parser("", "enzh");
    auto result = parser.truncateIntoSentencesUtf8(text, 240);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], std::string(240, 'a') + "!");
    EXPECT_EQ(result[1], " after exclamation");
}

TEST(StrPrseTest, SplitAtQuestionMark) {
    std::string text = std::string(240, 'a') + "? after question mark";
    atomizationCmd_translate::StrPrse parser("", "enzh");
    auto result = parser.truncateIntoSentencesUtf8(text, 240);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], std::string(240, 'a') + "?");
    EXPECT_EQ(result[1], " after question mark");
}

TEST(StrPrseTest, SplitAtChinesePeriod) {
    std::string text = std::string(240, 'a') + "\xE3\x80\x82"  // UTF-8 for 。
                           + "after chinese period";
    atomizationCmd_translate::StrPrse parser("", "enzh");
    auto result = parser.truncateIntoSentencesUtf8(text, 240);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[1], "after chinese period");
}

TEST(StrPrseTest, FallbackToComma) {
    std::string text = std::string(240, 'a') + ", after comma";
    atomizationCmd_translate::StrPrse parser("", "enzh");
    auto result = parser.truncateIntoSentencesUtf8(text, 240);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], std::string(240, 'a') + ",");
    EXPECT_EQ(result[1], " after comma");
}

TEST(StrPrseTest, HardTruncateWhenNoBoundary) {
    std::string text = std::string(500, 'x');
    atomizationCmd_translate::StrPrse parser("", "enzh");
    auto result = parser.truncateIntoSentencesUtf8(text, 240);
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], std::string(240, 'x'));
    EXPECT_EQ(result[1], std::string(240, 'x'));
    EXPECT_EQ(result[2], std::string(20, 'x'));
}

TEST(StrPrseTest, ChineseTextZhenMode) {
    atomizationCmd_translate::StrPrse parser("", "zhen");
    std::string one_char = "\xE4\xB8\xB8";                 // UTF-8 Chinese char 丸
    std::string text = one_char;
    for (int i = 1; i < 140; ++i) text += one_char;       // 140个 = 420 bytes
    text += "\xE3\x80\x82";                                  // 。
    text += "tail";
    auto result = parser.truncateIntoSentencesUtf8(text, 140);
    EXPECT_GE(result.size(), 2);
}

// ============================================================
// 输出后处理逻辑测试
// ============================================================

std::string apply_post_processing(std::string text) {
    boost::algorithm::replace_all(text, "\xE2\x96\x81", " ");   // ▁ → 空格
    boost::algorithm::replace_all(text, "\xE2\x81\x87", "");     // ⁇ → 移除
    return text;
}

TEST(PostProcessingTest, ReplaceSentencePieceMarkerWithSpace) {
    std::string input = "Hello\xE2\x96\x81World";  // "Hello▁World"
    std::string result = apply_post_processing(input);
    EXPECT_EQ(result, "Hello World");
}

TEST(PostProcessingTest, RemoveDoubleQuestionMarkArtifact) {
    std::string input = "text\xE2\x81\x87""more";  // "text⁇more"
    std::string result = apply_post_processing(input);
    EXPECT_EQ(result, "textmore");
}

TEST(PostProcessingTest, NormalQuestionMarkPreserved) {
    std::string input = "How are you?";
    std::string result = apply_post_processing(input);
    EXPECT_EQ(result, "How are you?");
}

TEST(PostProcessingTest, ChineseTextWithMarkers) {
    std::string input = "\xE2\x96\x81\xE4\xBD\xA0\xE5\xA5\xBD\xE2\x96\x81\xE4\xB8\x96\xE7\x95\x8C";
    std::string result = apply_post_processing(input);
    EXPECT_EQ(result, " 你好 世界");
}

// ============================================================
// UTF-8 检测测试
// ============================================================

TEST(Utf8DetectionTest, PureAscii) {
    MkCommandStrategy::CMKCommandStrategy strategy("test");
    EXPECT_TRUE(strategy.isUtf8("Hello World"));
}

TEST(Utf8DetectionTest, ValidUtf8Chinese) {
    MkCommandStrategy::CMKCommandStrategy strategy("test");
    std::string chinese = "\xE4\xBD\xA0\xE5\xA5\xBD";  // 你好
    EXPECT_TRUE(strategy.isUtf8(chinese));
}

TEST(Utf8DetectionTest, InvalidUtf8Sequence) {
    MkCommandStrategy::CMKCommandStrategy strategy("test");
    std::string invalid = "\xC0\xC0";  // invalid overlong encoding
    EXPECT_FALSE(strategy.isUtf8(invalid));
}

TEST(Utf8DetectionTest, TruncatedUtf8Sequence) {
    MkCommandStrategy::CMKCommandStrategy strategy("test");
    std::string truncated = "\xE4\xBD";  // incomplete 3-byte sequence
    EXPECT_FALSE(strategy.isUtf8(truncated));
}

TEST(Utf8DetectionTest, GBKEncoded) {
    MkCommandStrategy::CMKCommandStrategy strategy("test");
    std::string gbk = "\xCE\xD2\xCA\xC7";  // 我是 in GBK
    EXPECT_FALSE(strategy.isUtf8(gbk));
}

// ============================================================
// 命令注册测试
// ============================================================

TEST(CommandRegistrationTest, EnzhCommandRegistered) {
    MkCommandStrategy::CMKCommandStrategy strategy("enzh");
    strategy.initmap();
    EXPECT_NO_THROW(strategy.execute({"enzh"}));
}

TEST(CommandRegistrationTest, ZhenCommandRegistered) {
    MkCommandStrategy::CMKCommandStrategy strategy("zhen");
    strategy.initmap();
    EXPECT_NO_THROW(strategy.execute({"zhen"}));
}

// ============================================================
// 集成测试：英文→中文翻译 (依赖模型文件)
// ============================================================

#define ENZH_MODEL_CHECK()                                                                                             \
    do {                                                                                                               \
        if (!std::filesystem::exists(std::filesystem::current_path() / "model" / "enzh_ctranslate2")) {                \
            GTEST_SKIP() << "enzh model not found";                                                                    \
        }                                                                                                              \
    } while (0)

TEST(TranslationIntegrationTest, EnzhHelloWorld) {
    ENZH_MODEL_CHECK();
    MkCommandStrategy::CMKCommandStrategy strategy("enzh");
    strategy.initmap();
    std::vector<std::string> commands{"enzh", "Hello world"};
    EXPECT_NO_THROW(strategy.execute(commands));
}

TEST(TranslationIntegrationTest, EnzhQuestion) {
    ENZH_MODEL_CHECK();
    MkCommandStrategy::CMKCommandStrategy strategy("enzh");
    strategy.initmap();
    std::vector<std::string> commands{"enzh", "How are you?"};
    EXPECT_NO_THROW(strategy.execute(commands));
}

TEST(TranslationIntegrationTest, EnzhMultiSentence) {
    ENZH_MODEL_CHECK();
    MkCommandStrategy::CMKCommandStrategy strategy("enzh");
    strategy.initmap();
    std::vector<std::string> commands{"enzh", "Good morning. How are you today? I am fine, thank you."};
    EXPECT_NO_THROW(strategy.execute(commands));
}

TEST(TranslationIntegrationTest, EnzhLongText) {
    ENZH_MODEL_CHECK();
    MkCommandStrategy::CMKCommandStrategy strategy("enzh");
    strategy.initmap();
    std::string text = "Proxy was created by Microsoft engineers and incubated at Microsoft from 2018 to Feb 2026, "
                       "and has been used in the Windows operating system since 2022. "
                       "It is now maintained by the Next Gen C++ Foundation (ngcpp).";
    std::vector<std::string> commands{"enzh", text};
    EXPECT_NO_THROW(strategy.execute(commands));
}

// ============================================================
// 集成测试：中文→英语翻译 (依赖模型文件)
// ============================================================

#define ZHEN_MODEL_CHECK()                                                                                             \
    do {                                                                                                               \
        if (!std::filesystem::exists(std::filesystem::current_path() / "model" / "zhen_ctranslate2")) {                \
            GTEST_SKIP() << "zhen model not found";                                                                    \
        }                                                                                                              \
    } while (0)

TEST(TranslationIntegrationTest, ZhenHelloWorld) {
    ZHEN_MODEL_CHECK();
    MkCommandStrategy::CMKCommandStrategy strategy("zhen");
    strategy.initmap();
    std::vector<std::string> commands{"zhen", "\xE4\xBD\xA0\xE5\xA5\xBD\xE4\xB8\x96\xE7\x95\x8C"};  // 你好世界
    EXPECT_NO_THROW(strategy.execute(commands));
}

TEST(TranslationIntegrationTest, ZhenQuestion) {
    ZHEN_MODEL_CHECK();
    MkCommandStrategy::CMKCommandStrategy strategy("zhen");
    strategy.initmap();
    std::vector<std::string> commands{"zhen", "\xE4\xBD\xA0\xE5\xA5\xBD\xE5\x90\x97\xEF\xBC\x9F"};  // 你好吗？
    EXPECT_NO_THROW(strategy.execute(commands));
}

TEST(TranslationIntegrationTest, ZhenMultiSentence) {
    ZHEN_MODEL_CHECK();
    MkCommandStrategy::CMKCommandStrategy strategy("zhen");
    strategy.initmap();
    std::string input = "\xE6\x97\xA9\xE4\xB8\x8A\xE5\xA5\xBD\xE3\x80\x82"   // 早上好。
                        "\xE4\xBD\xA0\xE4\xBB\x8A\xE5\xA4\xA9\xE5\xA5\xBD\xE5\x90\x97\xEF\xBC\x9F"  // 你今天好吗？
                        "\xE6\x88\x91\xE5\xBE\x88\xE5\xA5\xBD\xEF\xBC\x8C\xE8\xB0\xA2\xE8\xB0\xA2\xE3\x80\x82";  // 我很好，谢谢。
    std::vector<std::string> commands{"zhen", input};
    EXPECT_NO_THROW(strategy.execute(commands));
}

TEST(TranslationIntegrationTest, ZhenShortPhrase) {
    ZHEN_MODEL_CHECK();
    MkCommandStrategy::CMKCommandStrategy strategy("zhen");
    strategy.initmap();
    std::vector<std::string> commands{"zhen", "\xE8\xB0\xA2\xE8\xB0\xA2"};  // 谢谢
    EXPECT_NO_THROW(strategy.execute(commands));
}

// ============================================================
// 边界条件测试
// ============================================================

TEST(StrPrseTest, SingleChar) {
    atomizationCmd_translate::StrPrse parser("", "enzh");
    auto result = parser.truncateIntoSentencesUtf8("A", 240);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "A");
}

TEST(StrPrseTest, ExactlyAtLimit) {
    std::string text = std::string(120, 'a');
    atomizationCmd_translate::StrPrse parser("", "enzh");
    auto result = parser.truncateIntoSentencesUtf8(text, 120);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], text);
}

TEST(PostProcessingTest, MultipleSpacesFromMarkers) {
    std::string input = "a\xE2\x96\x81\xE2\x96\x81\xE2\x96\x81""b";
    std::string result = apply_post_processing(input);
    EXPECT_EQ(result, "a   b");
}

TEST(PostProcessingTest, NoMarkersUnchanged) {
    std::string input = "plain text without any markers";
    std::string result = apply_post_processing(input);
    EXPECT_EQ(result, input);
}
