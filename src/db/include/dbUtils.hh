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

inline size_t levenshteinDistance(const std::string& s1,
                                  const std::string& s2) {
  size_t len1 = s1.length();
  size_t len2 = s2.length();
  std::vector<std::vector<size_t>> dp(len1 + 1,
                                      std::vector<size_t>(len2 + 1, 0));

  for (size_t i = 0; i <= len1; ++i) dp[i][0] = i;

  for (size_t j = 0; j <= len2; ++j) dp[0][j] = j;

  for (size_t i = 1; i <= len1; ++i) {
    for (size_t j = 1; j <= len2; ++j) {
      size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
      dp[i][j] = std::min(
          {dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
    }
  }

  return dp[len1][len2];
}

inline bool sortByLevenshtein(const std::pair<std::string, size_t>& a,
                              const std::pair<std::string, size_t>& b,
                              const std::string& token) {
  size_t distA = levenshteinDistance(a.first, token);
  size_t distB = levenshteinDistance(b.first, token);
  return distA < distB;
}

inline void sortByLevenshteinDistance(
    std::vector<std::pair<std::string, size_t>>& wordList,
    const std::string& token) {
  std::sort(wordList.begin(), wordList.end(),
            [&token](const auto& a, const auto& b) {
              return sortByLevenshtein(a, b, token);
            });
}

// Function to count the number of ordered subsequence with holes
inline size_t countOrderedSubsequenceWithHoles(const std::string& s1,
                                               const std::string& s2) {
  size_t count = 0;
  size_t i = 0;
  size_t j = 0;

  while (i < s1.length() && j < s2.length()) {
    if (s1[i] == s2[j]) {
      ++count;
      ++i;
      ++j;
    } else {
      ++j;  // Skip the character in s2
    }
  }

  return count;
}

// Comparator function for sorting
inline bool sortByOrderedSubsequenceWithHoles(
    const std::pair<std::string, size_t>& a,
    const std::pair<std::string, size_t>& b, const std::string& token) {
  size_t countA = countOrderedSubsequenceWithHoles(a.first, token);
  size_t countB = countOrderedSubsequenceWithHoles(b.first, token);
  return countA > countB;  // Sorting in decreasing order of common ordered
                           // subsequence with holes
}

// Function to sort the word list based on ordered subsequence with holes with
// the token
inline void sortByOrderedSubsequenceWithHoles(
    std::vector<std::pair<std::string, size_t>>& wordList,
    const std::string& token) {
  std::sort(wordList.begin(), wordList.end(),
            [&token](const auto& a, const auto& b) {
              return sortByOrderedSubsequenceWithHoles(a, b, token);
            });
}
inline size_t longestCommonSubsequenceWithGaps(const std::string& s1,
                                               const std::string& s2) {
  size_t m = s1.length();
  size_t n = s2.length();
  std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1, 0));

  for (size_t i = 1; i <= m; ++i) {
    for (size_t j = 1; j <= n; ++j) {
      if (s1[i - 1] == s2[j - 1]) {
        dp[i][j] = dp[i - 1][j - 1] + 1;
      } else {
        dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
      }
    }
  }

  return dp[m][n];
}

