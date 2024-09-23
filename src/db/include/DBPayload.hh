#pragma once
#include <string>
#include <vector>
// Struct to hold payload for insertPaper function
struct DBPayload {
  std::string doi;
  std::string title;
  std::string authors_list;
  int year = 0;
  int total_citations = 0;
  std::vector<std::pair<int, int>> citations;
  std::vector<std::string> index_terms;
  std::vector<std::string> author_keywords;
  std::vector<std::string> areas;

  // Constructor
  DBPayload(const std::string& doi, const std::string& title, const int& year,
            const std::string& authors_list, int total_citations,
            const std::vector<std::pair<int, int>>& citations,
            const std::vector<std::string>& index_terms,
            const std::vector<std::string>& author_keywords,
            const std::vector<std::string>& areas)
      : doi(doi),
        title(title),
        year(year),
        authors_list(authors_list),
        total_citations(total_citations),
        citations(citations),
        index_terms(index_terms),
        author_keywords(author_keywords),
        areas(areas) {}
  DBPayload() = default;
};

