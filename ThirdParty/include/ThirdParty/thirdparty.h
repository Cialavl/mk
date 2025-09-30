#pragma once
#define WIN32_LEAN_AND_MEAN
#include <array>
#include <codecvt>
#include <cryptopp/aes.h>
#include <cryptopp/base64.h>
#include <cryptopp/config.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/files.h>
#include <cryptopp/hex.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <cryptopp/salsa.h>
#include <cryptopp/secblock.h>
#include <ctranslate2/translator.h>
#include <iostream>
#include <locale>
#include <memory>
#include <ThirdParty/sentencepiece_processor.h>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <boost/algorithm/string.hpp>
#include <boost/locale/encoding.hpp>

int run(const char *cmd);
void TextTurnsSound(std::string _txt);
int create_txt(std::string _txtPath, std::string _nameTxt);

std::string stringCharacterReplace(std::string str, char _oldch,
                                   std::string _newch);
//std::string exec_cmd(const char *cmd);

inline std::string wstring_to_utf8(const std::wstring &wstr) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  return converter.to_bytes(wstr);
}
inline std::wstring utf8_to_wstring(const std::string &str) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  return converter.from_bytes(str);
}
// 按字符（而非字节）截取字符串
inline std::vector<std::string>
truncateIntoSentencesUtf8(const std::string &text, size_t max_len) {
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

        sentences.push_back(
            wstring_to_utf8(wtext.substr(start, pos2 - start + 1)));
        start = pos2 + 1; // 更新起始位置
      }

    } else {
      // 找到句号，截取到句号位置
      sentences.push_back(
          wstring_to_utf8(wtext.substr(start, pos - start + 1)));
      start = pos + 1; // 跳过句号，继续处理
    }
  }

  return sentences;
}
