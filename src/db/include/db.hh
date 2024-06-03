#pragma once

#include <SQLiteCpp/SQLiteCpp.h>

#include <set>
#include <string>
#include <unordered_set>
#include <vector>

struct DBPayload;
namespace bibtex {
struct BibTeXEntry;
}
enum KeywordType { SubjectArea, IndexTerm, AuthorKeyword, TitleKeyword };
inline std::string toString(KeywordType type) {
  switch (type) {
    case SubjectArea:
      return "SubjectArea";
    case IndexTerm:
      return "IndexTerm";
    case AuthorKeyword:
      return "AuthorKeyword";
    case TitleKeyword:
      return "TitleKeyword";
  }
  return "Unknown";
}

struct KeywordQueryResult {
  std::string _word;
  std::set<KeywordType> _type;
  std::map<size_t, size_t> _yearToCitations;
  size_t _totalCitations = 0;
  std::unordered_set<std::string> _papers;
};

extern SQLite::Database db;

void openDB();

void createDB();

// Helper function to insert data into all related tables
void insertPaper(const DBPayload& payload);

// Function to print the contents of the paper table
void printPapers();

// Function to print the contents of the citations table
void printCitations();

// Function to print the contents of the index_term_paper table
void printIndexTerms();

// Function to print the contents of the author_keyword_paper table
void printAuthorKeywords();

// Function to print the contents of the area_paper table
void printAreas();

void printAllTables();

// Function to convert a BibTeXEntry to a DBPayload
DBPayload toDBPayload(const bibtex::BibTeXEntry& entry);

// Function to get the total number of citations per year for a given keyword
KeywordQueryResult getCitations(const std::string& keyword);

std::vector<KeywordQueryResult> queryAllKeywords();

std::vector<KeywordQueryResult> searchKeywords(const std::string& searchString);

