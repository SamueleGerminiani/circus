#pragma once

#include <SQLiteCpp/SQLiteCpp.h>

#include <string>
#include <vector>

class DBPayload;
namespace bibtex {
class BibTeXEntry;
}

SQLite::Database openDB(std::string db_name = "research_papers.db");

void createDB(std::string db_name = "research_papers.db");

// Helper function to insert data into all related tables
void insertPaper(SQLite::Database& db, const DBPayload& payload);

// Function to print the contents of the paper table
void printPapers(SQLite::Database& db);

// Function to print the contents of the citations table
void printCitations(SQLite::Database& db);

// Function to print the contents of the index_term_paper table
void printIndexTerms(SQLite::Database& db);

// Function to print the contents of the author_keyword_paper table
void printAuthorKeywords(SQLite::Database& db);

// Function to print the contents of the area_paper table
void printAreas(SQLite::Database& db);

void printAllTables(SQLite::Database& db);

// Function to convert a BibTeXEntry to a DBPayload
DBPayload toDBPayload(const bibtex::BibTeXEntry& entry);

// Function to get the total number of citations per year for a given keyword
std::vector<std::pair<size_t, size_t>> getCitations(const std::string& keyword,
                                                    SQLite::Database& db);
