#include <gtest/gtest.h>

#include <commands/translate_command.h>
#include <mk_command_strategy/mk_command_strategy.h>
#include <translate/text_chunker.h>
#include <translate/text_postprocess.h>
#include <translate/translate_config.h>
#include <translate/translate_types.h>
#include <translate/translator_factory.h>

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

TEST(TextChunkerTest, EmptyInput) {
    mk::translate::TextChunker chunker(240);
    EXPECT_TRUE(chunker.split("").empty());
}

TEST(TextChunkerTest, ShortTextStaysWhole) {
    mk::translate::TextChunker chunker(240);
    const auto chunks = chunker.split("Hello world");
    ASSERT_EQ(chunks.size(), 1U);
    EXPECT_EQ(chunks[0], "Hello world");
}

TEST(TextChunkerTest, SplitsAtEnglishPeriod) {
    std::string text = std::string(240, 'a') + ". rest";
    mk::translate::TextChunker chunker(240);
    const auto chunks = chunker.split(text);
    ASSERT_EQ(chunks.size(), 2U);
    EXPECT_EQ(chunks[0], std::string(240, 'a') + ".");
}

TEST(TextChunkerTest, HardSplitWithoutBoundary) {
    const std::string text(500, 'x');
    mk::translate::TextChunker chunker(240);
    const auto chunks = chunker.split(text);
    ASSERT_EQ(chunks.size(), 3U);
    EXPECT_EQ(chunks[0], std::string(240, 'x'));
    EXPECT_EQ(chunks[1], std::string(240, 'x'));
    EXPECT_EQ(chunks[2], std::string(20, 'x'));
}

TEST(TextChunkerTest, NeverCutsUtf8Sequence) {
    const std::string one = "\xE4\xB8\xB8";  // 丸
    std::string text;
    for (int i = 0; i < 100; ++i) text += one;

    mk::translate::TextChunker chunker(30);
    const auto chunks = chunker.split(text);
    ASSERT_FALSE(chunks.empty());
    for (const auto& chunk : chunks) {
        ASSERT_FALSE(chunk.empty());
        ASSERT_EQ(chunk.size() % 3, 0U);
        for (std::size_t i = 0; i < chunk.size(); i += 3) {
            EXPECT_EQ(static_cast<unsigned char>(chunk[i]), 0xE4U);
        }
    }
}

TEST(TextChunkerTest, HandlesNewlines) {
    mk::translate::TextChunker chunker(20);
    const auto chunks = chunker.split("first line\nsecond line\nthird line");
    EXPECT_GE(chunks.size(), 2U);
    for (const auto& chunk : chunks) EXPECT_FALSE(chunk.empty());
}

TEST(LangPairTest, LegacyCodes) {
    auto zhen = mk::translate::parse_lang_pair("zhen");
    ASSERT_TRUE(zhen.has_value());
    EXPECT_EQ(zhen->src, "zh");
    EXPECT_EQ(zhen->tgt, "en");

    auto enzh = mk::translate::parse_lang_pair("enzh");
    ASSERT_TRUE(enzh.has_value());
    EXPECT_EQ(enzh->src, "en");
    EXPECT_EQ(enzh->tgt, "zh");
}

TEST(LangPairTest, GenericCodes) {
    auto pair = mk::translate::parse_lang_pair("zh-en");
    ASSERT_TRUE(pair.has_value());
    EXPECT_EQ(pair->code(), "zh-en");

    auto underscore = mk::translate::parse_lang_pair("en_ja");
    ASSERT_TRUE(underscore.has_value());
    EXPECT_EQ(underscore->tgt, "ja");
}

TEST(LangPairTest, CompactCommands) {
    auto zhja = mk::translate::parse_pair_code("zhja");
    ASSERT_TRUE(zhja.has_value());
    EXPECT_EQ(zhja->src, "zh");
    EXPECT_EQ(zhja->tgt, "ja");

    auto jaen = mk::translate::parse_pair_code("jaen");
    ASSERT_TRUE(jaen.has_value());
    EXPECT_EQ(jaen->code(), "ja-en");

    auto enfr = mk::translate::parse_pair_code("enfr");
    ASSERT_TRUE(enfr.has_value());
    EXPECT_EQ(enfr->tgt, "fr");

    EXPECT_TRUE(mk::translate::parse_pair_code("zhen").has_value());
    EXPECT_TRUE(mk::translate::parse_pair_code("enzh").has_value());
    EXPECT_FALSE(mk::translate::parse_pair_code("xxyy").has_value());
}

TEST(LangPairTest, RejectsInvalid) {
    EXPECT_FALSE(mk::translate::parse_lang_pair("").has_value());
    EXPECT_FALSE(mk::translate::parse_lang_pair("zh").has_value());
    EXPECT_FALSE(mk::translate::parse_lang_pair("zh-").has_value());
    EXPECT_FALSE(mk::translate::parse_lang_pair("zh-123").has_value());
}

TEST(LangPairTest, LanguageNames) {
    EXPECT_EQ(mk::translate::language_name_zh("zh"), "中文");
    EXPECT_EQ(mk::translate::language_name_zh("EN"), "英文");
    EXPECT_EQ(mk::translate::language_name_zh("xx"), "xx");
}

TEST(PostProcessTest, TrimsAndNormalizes) {
    EXPECT_EQ(mk::translate::clean_translation("  hello\r\nworld  "), "hello\nworld");
}

TEST(PostProcessTest, RemovesThinkingBlock) {
    EXPECT_EQ(mk::translate::clean_translation("<think>noise</think>real answer"), "real answer");
}

TEST(PostProcessTest, RemovesLabel) {
    EXPECT_EQ(mk::translate::clean_translation("译文：你好"), "你好");
    EXPECT_EQ(mk::translate::clean_translation("Translation: hi"), "hi");
}

TEST(PostProcessTest, CollapsesBlankLines) {
    EXPECT_EQ(mk::translate::clean_translation("a\n\n\n\nb"), "a\n\nb");
}

TEST(Utf8DetectionTest, PureAscii) {
    MkCommandStrategy::CMKCommandStrategy strategy("test");
    EXPECT_TRUE(strategy.isUtf8("Hello World"));
}

TEST(Utf8DetectionTest, ValidUtf8Chinese) {
    MkCommandStrategy::CMKCommandStrategy strategy("test");
    EXPECT_TRUE(strategy.isUtf8("\xE4\xBD\xA0\xE5\xA5\xBD"));
}

TEST(Utf8DetectionTest, InvalidUtf8Sequence) {
    MkCommandStrategy::CMKCommandStrategy strategy("test");
    EXPECT_FALSE(strategy.isUtf8("\xC0\xC0"));
}

TEST(Utf8DetectionTest, GBKEncoded) {
    MkCommandStrategy::CMKCommandStrategy strategy("test");
    EXPECT_FALSE(strategy.isUtf8("\xCE\xD2\xCA\xC7"));
}

TEST(CommandRegistrationTest, ZhenRegistered) {
    MkCommandStrategy::CMKCommandStrategy strategy("zhen");
    strategy.initmap();
    EXPECT_EQ(strategy.execute({"zhen"}), 0);
}

TEST(CommandRegistrationTest, EnzhRegistered) {
    MkCommandStrategy::CMKCommandStrategy strategy("enzh");
    strategy.initmap();
    EXPECT_EQ(strategy.execute({"enzh"}), 0);
}

TEST(CommandRegistrationTest, ZhJaRegistered) {
    MkCommandStrategy::CMKCommandStrategy strategy("zhja");
    strategy.initmap();
    EXPECT_EQ(strategy.execute({"zhja"}), 0);
}

TEST(CommandRegistrationTest, TrRegistered) {
    MkCommandStrategy::CMKCommandStrategy strategy("tr");
    strategy.initmap();
    EXPECT_EQ(strategy.execute({"tr"}), 0);
}

TEST(CommandRegistrationTest, UnknownCommand) {
    MkCommandStrategy::CMKCommandStrategy strategy("nope");
    strategy.initmap();
    EXPECT_EQ(strategy.execute({"nope"}), 1);
}

TEST(TranslationIntegrationTest, EnglishToChinese) {
    const auto root = mk::translate::find_model_root();
    const auto config = mk::translate::TranslateConfig::load(root / "config.json")
                            .value_or(mk::translate::TranslateConfig::defaults(root));

    const auto model = fs::exists(config.llama_model)
                           ? config.llama_model
                           : root / "index-2b" / "Index-Translate-2B.Q4_K_M.gguf";
    if (!fs::exists(model)) {
        GTEST_SKIP() << "GGUF model not found under " << root;
    }

    auto resolved = config;
    resolved.llama_model = model;
    const auto pair = *mk::translate::parse_lang_pair("enzh");
    auto translator = mk::translate::make_translator(resolved, pair);
    const auto output = translator->translate("Hello world", pair, {});
    EXPECT_FALSE(output.empty());
}
