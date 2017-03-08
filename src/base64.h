#pragma once

class Base64
{
  static const char b64_table[65];
  static const char reverse_table[128];
public:
  static std::string Encode(const std::string &bindata);
  static std::string Decode(const char* data, const char* data_end);
};
