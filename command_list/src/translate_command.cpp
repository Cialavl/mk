#define WIN32_LEAN_AND_MEAN
#include <translate_command/translate_command.h>
#include<boost/locale.hpp>
#include<boost/stacktrace/stacktrace.hpp>
#include<boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

atomizationCmd_translate::AtomCmdTranslate::AtomCmdTranslate(const std::string& _type) : l_type(_type) {
	get_model_root();
	options.beam_size = 5;
	options.length_penalty = 1.0;
	options.max_decoding_length = 500;

	if("zhen" == l_type) {
		target_prefix.push_back({{"__en__"}});
	} else if("enzh" == l_type) {
		target_prefix.push_back({{"zh_CN"}});
	}

	if(l_type == "zhen") {
		languages.processor = v_languages.at(0).processor;
		languages.resprocessor = v_languages.at(0).resprocessor;
		languages.Translator = v_languages.at(0).Translator;
	} else if(l_type == "enzh") {
		languages.processor = v_languages.at(1).processor;
		languages.resprocessor = v_languages.at(1).resprocessor;
		languages.Translator = v_languages.at(1).Translator;
	} else {
		languages.processor = {};
		languages.resprocessor = {};
		languages.Translator = {};
	}

	const auto status = processor.Load(languages.processor);
	if(!status.ok()) {
		std::cerr << status.ToString() << std::endl;
	}
	const auto resstatus = resprocessor.Load(languages.resprocessor);
	if(!resstatus.ok()) {
		std::cerr << resstatus.ToString() << std::endl;
	}
        set_ctranslate2();
}

void atomizationCmd_translate::AtomCmdTranslate::set_processor_languages() {}

int atomizationCmd_translate::AtomCmdTranslate::set_ctranslate2(ctranslate2::Device _device) {
	translator = new ctranslate2::Translator(languages.Translator, _device);
	return 0;
}

void atomizationCmd_translate::AtomCmdTranslate::translation(std::string _input) {
	StrPrse strpres(std::move(_input), std::move(l_type));

	std::vector<std::string> vout{};
	for(auto i : strpres.strslice()) {
		vout.push_back(translationSentences(i));
	}
	output(std::move(vout));
}

void atomizationCmd_translate::AtomCmdTranslate::output(std::vector<std::string>&& _output) {
	for(auto& text : _output) {
		boost::algorithm::replace_all(text, "▁", "");
		boost::algorithm::replace_all(text, "?", "");
		boost::algorithm::replace_all(text, "⁇", "");

#if defined(_WIN32) || defined(_WIN64)
		std::cout << boost::locale::conv::between(text, "GBK", "UTF-8") << std::endl;
#elif defined(__linux__)
		std::cout << text << "\n";
#endif

	}
}

std::string atomizationCmd_translate::AtomCmdTranslate::translationSentences(std::string& src) {
	pieces.clear();
	batch.clear();
	results.clear();
	res.clear();
	auto is_invisible_char_except_space = [](unsigned char c) -> bool { return std::iscntrl(c) && c != ' '; };
	src.erase(std::remove_if(src.begin(), src.end(), is_invisible_char_except_space), src.end());

	processor.Encode(src, &pieces);
	if("zhen" == l_type) {
		pieces.emplace(pieces.begin(), "__zh__");
	} else if("enzh" == l_type) {
		pieces.emplace(pieces.begin(), "en");
	}
	batch.push_back(pieces);
	results = translator->translate_batch(batch, target_prefix, options);

	for(const auto& token : results[0].output()) {
		res.push_back(token);
	}
	resprocessor.Decode(res, &text);

	return text;
}

void atomizationCmd_translate::AtomCmdTranslate::get_model_root() {
	//std::filesystem::path currentPath = std::filesystem::current_path();
	//std::string current_path = currentPath.string();
    boost::filesystem::path full_path = boost::dll::program_location();
        addlanguage(full_path.parent_path().string());
}

void atomizationCmd_translate::AtomCmdTranslate::get_model_root(std::string _path) {
	addlanguage(_path);
}

void atomizationCmd_translate::AtomCmdTranslate::addlanguage(std::string _path) {
	//_path = stringCharacterReplace(_path, '\\', "/");
	// std::cout << _path << std::endl;
    boost::algorithm::replace_all(_path, "\\", "/");
	languages.processor = _path + "/model/opus-2020-07-17zhen/source.spm";
	languages.resprocessor = _path + "/model/opus-2020-07-17enzh/source.spm";
	languages.Translator = _path + "/model/zhen_ctranslate2";
	v_languages.push_back(languages);
	languages.processor = _path + "/model/opus-2020-07-17enzh/source.spm";
	languages.resprocessor = _path + "/model/opus-2020-07-17zhen/source.spm";
	languages.Translator = _path + "/model/enzh_ctranslate2";
	v_languages.push_back(languages);
}

atomizationCmd_translate::AtomCmdTranslate::~AtomCmdTranslate() {
	delete translator;
	translator = nullptr;
}

std::vector<std::string> atomizationCmd_translate::StrPrse::strslice() {
	return [&]() -> std::vector<std::string> {
		if("zhen" == l_type) {
            //strs = boost::locale::conv::from_utf(strs, "gb2312");
			vstrs = truncateIntoSentencesUtf8(strs, 140);
                 
		} else if("enzh" == l_type) {
            //strs = boost::locale::conv::from_utf(strs, "gb2312");
			vstrs = truncateIntoSentencesUtf8(strs, 240);
                   
		}
		return vstrs;
	}();
}
std::vector<std::string> atomizationCmd_translate::StrPrse::truncateIntoSentencesUtf8(
    const std::string& text, size_t max_len) {
    std::vector<std::string> sentences;
    std::wstring wtext = utf8_to_wstring(text); // 转换为宽字符

    size_t start = 0;
    while (start < wtext.length()) {
        // 如果剩余字符小于等于 max_len，直接截取
        if (wtext.length() - start <= max_len) {
            sentences.push_back(wstring_to_utf8(wtext.substr(start)));
            break;
        }

        // 查找在当前范围内最后一个句号（。或.）
        size_t pos = wtext.find_last_of(L"。.", start + max_len);
        if (pos == std::wstring::npos || pos < start) {
            // 如果没有找到句号，直接截取 max_len 长度的字符
            size_t pos2 = wtext.find_last_of(L"，", start + max_len);
            if (pos2 == std::wstring::npos || pos2 < start) {
                sentences.push_back(wstring_to_utf8(wtext.substr(start, max_len)));
                start += max_len; // 更新起始位置
            } else {

                sentences.push_back(wstring_to_utf8(wtext.substr(start, pos2 - start + 1)));
                start = pos2 + 1; // 更新起始位置
            }

        } else {
            // 找到句号，截取到句号位置
            sentences.push_back(wstring_to_utf8(wtext.substr(start, pos - start + 1)));
            start = pos + 1; // 跳过句号，继续处理
        }
    }

    return sentences;
}

 std::string atomizationCmd_translate::StrPrse::wstring_to_utf8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}
 std::wstring atomizationCmd_translate::StrPrse::utf8_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}
