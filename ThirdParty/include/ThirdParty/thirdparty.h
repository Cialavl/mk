#pragma once
#include <ctranslate2/translator.h>
#include <sentencepiece_processor.h>
#include <qstring.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <array>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <cryptopp/cryptlib.h>
#include <cryptopp/base64.h>
#include <cryptopp/files.h>
#include <cryptopp/aes.h> 
#include <cryptopp/hex.h> 
#include <cryptopp/modes.h> 
#include <cryptopp/salsa.h>
#include <cryptopp/osrng.h>
#include <cryptopp/secblock.h>
#include <boost/locale/encoding.hpp>
#include <boost/algorithm/string.hpp>
#include <locale>
#include <codecvt>

std::string GbkToUtf8(const char* src_str);
int run(const char* cmd);
void TextTurnsSound(std::string _txt);
int create_txt(std::string _txtPath, std::string _nameTxt);
std::string Utf8ToGbk(const char* src_str);
std::string stringCharacterReplace(std::string str, char _oldch, std::string _newch);
std::string exec_cmd(const char* cmd);

inline std::string getcwd() {
   
	wchar_t buffer[MAX_PATH];

	GetModuleFileName(NULL, buffer, MAX_PATH);

	QString s = QString::fromWCharArray(buffer).remove(s.length() - 6, 6).replace("\\", "/");

	return s.toStdString();
    
}
inline std::string wstring_to_utf8(const std::wstring& wstr) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(wstr);
}
inline std::wstring utf8_to_wstring(const std::string& str) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(str);
}
// ���ַ��������ֽڣ���ȡ�ַ���
inline std::vector<std::string> truncateIntoSentencesUtf8(const std::string& text, size_t max_len) {
	std::vector<std::string> sentences;
	std::wstring wtext = utf8_to_wstring(text);  // ת��Ϊ���ַ�

	size_t start = 0;
	while (start < wtext.length()) {
		// ���ʣ���ַ�С�ڵ��� max_len��ֱ�ӽ�ȡ
		if (wtext.length() - start <= max_len) {
			sentences.push_back(wstring_to_utf8(wtext.substr(start)));
			break;
		}

		// �����ڵ�ǰ��Χ�����һ����ţ�����.��
		size_t pos = wtext.find_last_of(L"��.", start + max_len);
		if (pos == std::wstring::npos || pos < start) {
			// ���û���ҵ���ţ�ֱ�ӽ�ȡ max_len ���ȵ��ַ�
			size_t pos2 = wtext.find_last_of(L"��", start + max_len);
			if (pos2 == std::wstring::npos || pos2 < start) {
				sentences.push_back(wstring_to_utf8(wtext.substr(start, max_len)));
				start += max_len; // ������ʼλ��
			}
			else {

				sentences.push_back(wstring_to_utf8(wtext.substr(start, pos2 - start + 1)));
				start = pos2 + 1; // ������ʼλ��
			}

		}
		else {
			// �ҵ���ţ���ȡ�����λ��
			sentences.push_back(wstring_to_utf8(wtext.substr(start, pos - start + 1)));
			start = pos + 1; // ������ţ���������
		}
	}

	return sentences;
}
