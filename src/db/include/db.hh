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
  double _zScore = 0;
  size_t _totalCitations = 0;
  std::unordered_set<std::string> _papers;
  std::map<size_t, std::unordered_set<std::string>> _yearToPapers;
  // these are require for the impact factor calculation
  std::map<size_t, size_t>
      _yearToCitationInYearOfPapersPublishedThePreviousTwoYears;
};

extern SQLite::Database db;

void openDB();

void createDB();

// Helper function to insert data into all related tables
void insertPaper(const DBPayload& payload);

std::vector<DBPayload> getPapers(std::string keyword);

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

std::vector<KeywordQueryResult> queryAllKeywords();

std::vector<KeywordQueryResult> searchKeywords(const std::string& searchString);

void addZScore(KeywordQueryResult& result);
KeywordQueryResult getKQR(const std::string& keyword);
void addKQR(const KeywordQueryResult& kqr);
void removeKQR(const KeywordQueryResult& kqr);
