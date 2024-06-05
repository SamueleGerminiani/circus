#pragma once
#include <algorithm>
#include <ctime>
#include <numeric>
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

inline double calculateMean(const std::vector<size_t>& values) {
  double sum = std::accumulate(values.begin(), values.end(), 0.0);
  return sum / values.size();
}

inline double calculateStandardDeviation(const std::vector<size_t>& values,
                                         double mean) {
  double sq_sum =
      std::inner_product(values.begin(), values.end(), values.begin(), 0.0);
  double variance = sq_sum / values.size() - mean * mean;
  return std::sqrt(variance);
}

inline size_t getCurrentYear() {
  time_t now = time(0);
  tm* ltm = localtime(&now);
  return 1900 + ltm->tm_year;
}

