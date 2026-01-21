#pragma once
#define WIN32_LEAN_AND_MEAN
#include <abstraction_command/abstraction_command.h>
#include <template_path/template_path.h>
#include <ctranslate2/translator.h>
#include <translate_command/sentencepiece_processor.h>
#include <translate_command/sentencepiece_trainer.h>
#include<boost/locale.hpp>
#include<boost/stacktrace/stacktrace.hpp>
#include<boost/algorithm/algorithm.hpp>
#include <codecvt>
namespace atomizationCmd_translate {

    struct Languages {
        std::string processor;
        std::string resprocessor;
        std::string Translator;
    };


    class AtomCmdTranslate : public AbsCommand::Abs_Command {
    public:
        AtomCmdTranslate(const std::string& _type);
        ~AtomCmdTranslate();
        void translation(std::string _input);
        void addlanguage(std::string _disk_path);
        void get_model_root(std::string _path);
        void get_model_root();
        void output(std::vector<std::string>&& _output);
        std::string translationSentences(std::string& src);
        int set_ctranslate2(ctranslate2::Device _device = ctranslate2::Device::CUDA);
        void set_processor_languages();


    private:
        std::map<std::string, std::string> processor_map;
        template_path::Template_Path<fs::path, std::string> paths;


        std::string type;

        std::vector<std::string> pieces;

        std::vector<std::vector<std::string>> batch;

        std::vector<ctranslate2::TranslationResult> results;

        std::vector<std::string> res;
        std::string l_type;
        std::string text;
        std::vector<Languages> v_languages;
        Languages languages;
        ctranslate2::Translator* translator = nullptr;
        sentencepiece::SentencePieceProcessor processor;
        sentencepiece::SentencePieceProcessor resprocessor;
        std::vector<std::vector<std::string>> target_prefix{};
        ctranslate2::TranslationOptions options;
    };

    class StrPrse {
    public:
        StrPrse(std::string&& str, std::string&& _type) : l_type(_type), strs(str) {}
        std::vector<std::string> strslice();
        std::string wstring_to_utf8(const std::wstring& wstr);
        std::wstring utf8_to_wstring(const std::string& str);
        std::vector<std::string> truncateIntoSentencesUtf8(const std::string& text, size_t max_len);
    private:
        std::string l_type;
        std::string strs;
        std::vector<std::string> vstrs{};
    };

} // namespace atomizationCmd_translate
