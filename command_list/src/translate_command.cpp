#define WIN32_LEAN_AND_MEAN
#include <translate_command/translate_command.h>



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
	std::filesystem::path currentPath = std::filesystem::current_path();
	std::string current_path = currentPath.string();
	addlanguage(current_path);
}

void atomizationCmd_translate::AtomCmdTranslate::get_model_root(std::string _path) {
	addlanguage(_path);
}

void atomizationCmd_translate::AtomCmdTranslate::addlanguage(std::string _path) {
	_path = stringCharacterReplace(_path, '\\', "/");
	// std::cout << _path << std::endl;
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
			vstrs = truncateIntoSentencesUtf8(strs, 140);
		} else if("enzh" == l_type) {
			vstrs = truncateIntoSentencesUtf8(strs, 240);
		}
		return vstrs;
	}();
}
