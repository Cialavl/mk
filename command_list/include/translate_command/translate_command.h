#pragma once
#include<ThirdParty/thirdparty.h>
#include<abstraction_command/abstraction_command.h>
#include <template_path/template_path.h> 

namespace atomizationCmd_translate { 

	struct Languages
	{
		std::string processor;
		std::string resprocessor;
		std::string Translator;
	};


	class AtomCmdTranslate : public AbsCommand::Abs_Command
	{
	public:

		AtomCmdTranslate(std::string _type);
		~AtomCmdTranslate();
		void translation(std::string _input);
		void addlanguage(std::string _disk_path);
		void get_model_root(std::string _path);
		void get_model_root();
		void output(std::vector<std::string> &&_output);
		std::string translationSentences(std::string &src);
		int set_ctranslate2(ctranslate2::Device _device = ctranslate2::Device::CPU);
		void set_processor_languages();
		

	private: 
		std::map<std::string, std::string>  processor_map;
		template_path::Template_Path<fs::path, std::string> paths;


		std::string type; 

		std::vector<std::string> pieces;

		std::vector<std::vector<std::string>> batch ;

		std::vector<ctranslate2::TranslationResult> results;

		std::vector<std::string> res;
		std::string l_type;
		std::string text;
		std::vector<Languages> v_languages;
		Languages languages;
		ctranslate2::Translator *translator=nullptr;
		sentencepiece::SentencePieceProcessor processor;
		sentencepiece::SentencePieceProcessor resprocessor;
		std::vector < std::vector<std::string>> target_prefix{  };
		ctranslate2::TranslationOptions options;
	};

	class StrPrse {
	public:
        StrPrse(std::string&& str,std::string && _type) :l_type(_type),strs(str){
			
		}
		std::vector<std::string> strslice();
	private:
		std::string l_type;
		std::string strs;
		std::vector<std::string> vstrs{};
	};

}

