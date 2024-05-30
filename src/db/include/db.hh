#pragma once

#include <SQLiteCpp/SQLiteCpp.h>

#include <iostream>
#include <string>

#include "DBPayload.hh"
#include "bibtexreader.hpp"
#include "message.hh"

inline SQLite::Database openDB(std::string db_name = "research_papers.db") {
  try {
    // Open a database file in read/write mode
    SQLite::Database db(db_name, SQLite::OPEN_READWRITE);
    return db;
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }
}

inline void createDB(std::string db_name = "research_papers.db") {
  try {
    // return if the database is already created
    if (std::filesystem::exists("research_papers.db")) {
      messageInfo("Database already exists");
      return;
    }

    // Open a database file in create/write mode
    SQLite::Database db("research_papers.db",
                        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

    // Create the paper table
    db.exec(
        "CREATE TABLE IF NOT EXISTS paper ("
        "doi TEXT PRIMARY KEY, "
        "title TEXT NOT NULL, "
        "authors_list TEXT NOT NULL, "
        "total_citations INTEGER NOT NULL);");

    // Create the citations table
    db.exec(
        "CREATE TABLE IF NOT EXISTS citations ("
        "doi TEXT, "
        "year INTEGER, "
        "number INTEGER, "
        "PRIMARY KEY (doi, year), "
        "FOREIGN KEY (doi) REFERENCES paper(doi));");

    // Create the index_term_paper table
    db.exec(
        "CREATE TABLE IF NOT EXISTS index_term_paper ("
        "index_term TEXT, "
        "doi TEXT, "
        "PRIMARY KEY (index_term, doi), "
        "FOREIGN KEY (doi) REFERENCES paper(doi));");

    // Create the author_keyword_paper table
    db.exec(
        "CREATE TABLE IF NOT EXISTS author_keyword_paper ("
        "author_keyword TEXT, "
        "doi TEXT, "
        "PRIMARY KEY (author_keyword, doi), "
        "FOREIGN KEY (doi) REFERENCES paper(doi));");

    // Create the area_paper table
    db.exec(
        "CREATE TABLE IF NOT EXISTS area_paper ("
        "area TEXT, "
        "doi TEXT, "
        "PRIMARY KEY (area, doi), "
        "FOREIGN KEY (doi) REFERENCES paper(doi));");

    std::cout << "Database tables created successfully." << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}

// Helper function to insert data into all related tables
inline void insertPaper(SQLite::Database& db, const DBPayload& payload) {
  try {
    // Begin a transaction
    SQLite::Transaction transaction(db);

    // Insert into paper table
    {
      SQLite::Statement query(db,
                              "INSERT INTO paper (doi, title, authors_list, "
                              "total_citations) VALUES (?, ?, ?, ?)");
      query.bind(1, payload.doi);
      query.bind(2, payload.title);
      query.bind(3, payload.authors_list);
      query.bind(4, payload.total_citations);
      query.exec();
    }

    // Insert into citations table
    for (const auto& citation : payload.citations) {
      SQLite::Statement query(
          db, "INSERT INTO citations (doi, year, number) VALUES (?, ?, ?)");
      query.bind(1, payload.doi);
      query.bind(2, citation.first);
      query.bind(3, citation.second);
      query.exec();
    }

    // Insert into index_term_paper table
    for (const auto& index_term : payload.index_terms) {
      SQLite::Statement query(
          db, "INSERT INTO index_term_paper (index_term, doi) VALUES (?, ?)");
      query.bind(1, index_term);
      query.bind(2, payload.doi);
      query.exec();
    }

    // Insert into author_keyword_paper table
    for (const auto& author_keyword : payload.author_keywords) {
      SQLite::Statement query(db,
                              "INSERT INTO author_keyword_paper "
                              "(author_keyword, doi) VALUES (?, ?)");
      query.bind(1, author_keyword);
      query.bind(2, payload.doi);
      query.exec();
    }

    // Insert into area_paper table
    for (const auto& area : payload.areas) {
      SQLite::Statement query(
          db, "INSERT INTO area_paper (area, doi) VALUES (?, ?)");
      query.bind(1, area);
      query.bind(2, payload.doi);
      query.exec();
    }

    // Commit the transaction
    transaction.commit();

    std::cout << "Paper and related data inserted successfully." << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}

// Function to print the contents of the paper table
void inline printPapers(SQLite::Database& db) {
  SQLite::Statement query(db, "SELECT * FROM paper");
  while (query.executeStep()) {
    std::cout << "DOI: " << query.getColumn(0)
              << ", Title: " << query.getColumn(1)
              << ", Authors: " << query.getColumn(2)
              << ", Total Citations: " << query.getColumn(3) << std::endl;
  }
}

// Function to print the contents of the citations table
void inline printCitations(SQLite::Database& db) {
  SQLite::Statement query(db, "SELECT * FROM citations");
  while (query.executeStep()) {
    std::cout << "DOI: " << query.getColumn(0)
              << ", Year: " << query.getColumn(1)
              << ", Number: " << query.getColumn(2) << std::endl;
  }
}

// Function to print the contents of the index_term_paper table
void inline printIndexTerms(SQLite::Database& db) {
  SQLite::Statement query(db, "SELECT * FROM index_term_paper");
  while (query.executeStep()) {
    std::cout << "Index Term: " << query.getColumn(0)
              << ", DOI: " << query.getColumn(1) << std::endl;
  }
}

// Function to print the contents of the author_keyword_paper table
void inline printAuthorKeywords(SQLite::Database& db) {
  SQLite::Statement query(db, "SELECT * FROM author_keyword_paper");
  while (query.executeStep()) {
    std::cout << "Author Keyword: " << query.getColumn(0)
              << ", DOI: " << query.getColumn(1) << std::endl;
  }
}

// Function to print the contents of the area_paper table
void inline printAreas(SQLite::Database& db) {
  SQLite::Statement query(db, "SELECT * FROM area_paper");
  while (query.executeStep()) {
    std::cout << "Area: " << query.getColumn(0)
              << ", DOI: " << query.getColumn(1) << std::endl;
  }
}

void inline printAllTables(SQLite::Database& db) {
  std::cout << "--------------------------------"
            << "\n";
  std::cout << "Paper Table"
            << "\n";
  printPapers(db);
  std::cout << "--------------------------------"
            << "\n";
  std::cout << "Citations Table"
            << "\n";
  printCitations(db);
  std::cout << "--------------------------------"
            << "\n";
  std::cout << "Index Term Paper Table"
            << "\n";
  printIndexTerms(db);
  std::cout << "--------------------------------"
            << "\n";
  std::cout << "Author Keyword Paper Table"
            << "\n";
  printAuthorKeywords(db);
  std::cout << "--------------------------------"
            << "\n";
  std::cout << "Area Paper Table"
            << "\n";
  printAreas(db);
  std::cout << "--------------------------------"
            << "\n";
}

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
    tokens.push_back(toLower(token));
  }
  return tokens;
}

inline DBPayload toDBPayload(const bibtex::BibTeXEntry& entry) {
  DBPayload payload;

  // Iterate over fields of BibTeXEntry
  for (const auto& field : entry.fields) {
    if (field.first == "doi") {
      payload.doi = field.second.front();  // Assuming doi is a single value
    } else if (field.first == "title") {
      payload.title = field.second.front();  // Assuming title is a single value
    } else if (field.first == "author") {
      payload.authors_list =
          field.second.front();  // Assuming title is a single value
    } else if (field.first == "per_year_citations") {
      // Extract per-year citations as pairs of year and number
      for (const auto& citation : split(field.second.front(), ',')) {
        auto citation_pair = split(citation, ':');
        payload.citations.emplace_back(std::stoi(citation_pair[0]),
                                       std::stoi(citation_pair[1]));
      }
      for (const auto& citation : payload.citations) {
        payload.total_citations += citation.second;
      }
    } else if (field.first == "index_terms") {
      payload.index_terms = split(field.second.front(), ',');
    } else if (field.first == "author_keywords") {
      payload.author_keywords = split(field.second.front(), ',');
    } else if (field.first == "subject_areas") {
      payload.areas = split(field.second.front(), ',');
    }
  }

  return payload;
}
