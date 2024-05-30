#pragma once
#include <sstream>
#include <string>
#include <vector>

inline std::string toLower(const std::string& str) {
  std::string lower_str = str;
  std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return lower_str;
}
inline std::vector<std::string> split(const std::string& str, char delimiter) {
  std::vector<std::string> tokens;
  std::istringstream ss(str);
  std::string token;
  while (std::getline(ss, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}
inline std::string removeLeading(const std::string& str) {
  size_t start = 0;
  while (start < str.size() && std::isspace(str[start])) {
    start++;
  }
  return str.substr(start);
}
inline std::vector<std::string> splitFix(const std::string& str,
                                         char delimiter) {
  std::vector<std::string> tokens;
  std::istringstream ss(str);
  std::string token;
  while (std::getline(ss, token, delimiter)) {
    tokens.push_back(removeLeading(toLower(token)));
  }
  return tokens;
}

