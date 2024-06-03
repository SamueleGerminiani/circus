#include "db.hh"

#include <SQLiteCpp/SQLiteCpp.h>

#include <iostream>
#include <regex>
#include <unordered_map>

#include "DBPayload.hh"
#include "bibtexentry.hpp"
#include "dbUtils.hh"
#include "globals.hh"
#include "message.hh"

SQLite::Database db(clc::dbFile, SQLite::OPEN_READWRITE);

void openDB() {
  try {
    // Open a database file in read/write mode
    db = SQLite::Database(clc::dbFile, SQLite::OPEN_READWRITE);
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }
}

void createDB() {
  try {
    // return if the database is already created
    if (std::filesystem::exists(clc::dbFile)) {
      messageInfo("Database already exists, opening the existing database...");
      openDB();
      return;
    }

    // Open a database file in create/write mode
    db = SQLite::Database(clc::dbFile,
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

  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}

void insertPaper(const DBPayload& payload) {
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

  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}

void printPapers() {
  SQLite::Statement query(db, "SELECT * FROM paper");
  while (query.executeStep()) {
    std::cout << "DOI: " << query.getColumn(0)
              << ", Title: " << query.getColumn(1)
              << ", Authors: " << query.getColumn(2)
              << ", Total Citations: " << query.getColumn(3) << std::endl;
  }
}

void printCitations() {
  SQLite::Statement query(db, "SELECT * FROM citations");
  while (query.executeStep()) {
    std::cout << "DOI: " << query.getColumn(0)
              << ", Year: " << query.getColumn(1)
              << ", Number: " << query.getColumn(2) << std::endl;
  }
}

void printIndexTerms() {
  SQLite::Statement query(db, "SELECT * FROM index_term_paper");
  while (query.executeStep()) {
    std::cout << "Index Term:" << query.getColumn(0)
              << ", DOI: " << query.getColumn(1) << std::endl;
  }
}

void printAuthorKeywords() {
  SQLite::Statement query(db, "SELECT * FROM author_keyword_paper");
  while (query.executeStep()) {
    std::cout << "Author Keyword: " << query.getColumn(0)
              << ", DOI: " << query.getColumn(1) << std::endl;
  }
}

void printAreas() {
  SQLite::Statement query(db, "SELECT * FROM area_paper");
  while (query.executeStep()) {
    std::cout << "Area: " << query.getColumn(0)
              << ", DOI: " << query.getColumn(1) << std::endl;
  }
}

void printAllTables() {
  std::cout << "--------------------------------"
            << "\n";
  std::cout << "Paper Table"
            << "\n";
  printPapers();
  std::cout << "--------------------------------"
            << "\n";
  std::cout << "Citations Table"
            << "\n";
  printCitations();
  std::cout << "--------------------------------"
            << "\n";
  std::cout << "Index Term Paper Table"
            << "\n";
  printIndexTerms();
  std::cout << "--------------------------------"
            << "\n";
  std::cout << "Author Keyword Paper Table"
            << "\n";
  printAuthorKeywords();
  std::cout << "--------------------------------"
            << "\n";
  std::cout << "Area Paper Table"
            << "\n";
  printAreas();
  std::cout << "--------------------------------"
            << "\n";
}

DBPayload toDBPayload(const bibtex::BibTeXEntry& entry) {
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
      payload.index_terms = splitFix(field.second.front(), ',');
    } else if (field.first == "author_keywords") {
      payload.author_keywords = splitFix(field.second.front(), ',');
    } else if (field.first == "subject_areas") {
      payload.areas = splitFix(field.second.front(), ',');
    }
  }

  return payload;
}

KeywordQueryResult getCitations(const std::string& keyword) {
  KeywordQueryResult kqr;

  try {
    // Query for all DOIs that match the keyword in index_term_paper
    SQLite::Statement query_index(
        db, "SELECT doi FROM index_term_paper WHERE index_term = ?");
    query_index.bind(1, keyword);
    while (query_index.executeStep()) {
      std::string doi = query_index.getColumn(0).getString();
      SQLite::Statement query_citations(
          db, "SELECT year, number FROM citations WHERE doi = ?");
      query_citations.bind(1, doi);
      while (query_citations.executeStep()) {
        size_t year = query_citations.getColumn(0).getInt();
        size_t number = query_citations.getColumn(1).getInt();
        kqr._yearToCitations[year] += number;
        kqr._type.insert(KeywordType::IndexTerm);
      }
    }

    // Query for all DOIs that match the keyword in author_keyword_paper
    SQLite::Statement query_author(
        db, "SELECT doi FROM author_keyword_paper WHERE author_keyword = ?");
    query_author.bind(1, keyword);
    while (query_author.executeStep()) {
      std::string doi = query_author.getColumn(0).getString();
      SQLite::Statement query_citations(
          db, "SELECT year, number FROM citations WHERE doi = ?");
      query_citations.bind(1, doi);
      while (query_citations.executeStep()) {
        size_t year = query_citations.getColumn(0).getInt();
        size_t number = query_citations.getColumn(1).getInt();
        kqr._yearToCitations[year] += number;
        kqr._type.insert(KeywordType::AuthorKeyword);
      }
    }

    // Query for all DOIs that match the keyword in area_paper
    SQLite::Statement query_area(db,
                                 "SELECT doi FROM area_paper WHERE area = ?");
    query_area.bind(1, keyword);
    while (query_area.executeStep()) {
      std::string doi = query_area.getColumn(0).getString();
      SQLite::Statement query_citations(
          db, "SELECT year, number FROM citations WHERE doi = ?");
      query_citations.bind(1, doi);
      while (query_citations.executeStep()) {
        size_t year = query_citations.getColumn(0).getInt();
        size_t number = query_citations.getColumn(1).getInt();
        kqr._yearToCitations[year] += number;
        kqr._type.insert(KeywordType::SubjectArea);
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  messageWarningIf(kqr._yearToCitations.empty(),
                   "No citations found for keyword: " + keyword);

  return kqr;
}

std::vector<KeywordQueryResult> queryAllKeywords() {
  std::unordered_map<std::string, KeywordQueryResult> word_to_kqr;

  try {
    // Query for all author keywords and calculate total citations
    {
      SQLite::Statement query(
          db, "SELECT author_keyword, doi FROM author_keyword_paper");
      while (query.executeStep()) {
        std::string keyword = query.getColumn(0).getString();
        std::string doi = query.getColumn(1).getString();

        // Query for citations associated with the DOI
        SQLite::Statement query_citations(
            db, "SELECT year, number FROM citations WHERE doi = ?");
        query_citations.bind(1, doi);
        while (query_citations.executeStep()) {
          size_t year = query_citations.getColumn(0).getInt();
          size_t citations = query_citations.getColumn(1).getInt();
          word_to_kqr[keyword]._yearToCitations[year] += citations;
          word_to_kqr[keyword]._totalCitations += citations;
        }
        word_to_kqr[keyword]._type.insert(KeywordType::AuthorKeyword);
        word_to_kqr[keyword]._papers.insert(doi);
        word_to_kqr[keyword]._word = keyword;
      }
    }

    // Query for all index terms and calculate total citations
    {
      SQLite::Statement query(db,
                              "SELECT index_term, doi FROM index_term_paper");
      while (query.executeStep()) {
        std::string keyword = query.getColumn(0).getString();
        std::string doi = query.getColumn(1).getString();

        // Query for citations associated with the DOI
        SQLite::Statement query_citations(
            db, "SELECT year, number FROM citations WHERE doi = ?");
        query_citations.bind(1, doi);
        while (query_citations.executeStep()) {
          size_t year = query_citations.getColumn(0).getInt();
          size_t citations = query_citations.getColumn(1).getInt();
          word_to_kqr[keyword]._yearToCitations[year] += citations;
          word_to_kqr[keyword]._totalCitations += citations;
        }
        word_to_kqr[keyword]._papers.insert(doi);
        word_to_kqr[keyword]._type.insert(KeywordType::IndexTerm);
        word_to_kqr[keyword]._word = keyword;
      }
    }

    // Query for all subject areas and calculate total citations
    {
      SQLite::Statement query(db, "SELECT area, doi FROM area_paper");
      while (query.executeStep()) {
        std::string keyword = query.getColumn(0).getString();
        std::string doi = query.getColumn(1).getString();

        // Query for citations associated with the DOI
        SQLite::Statement query_citations(
            db, "SELECT year, number FROM citations WHERE doi = ?");
        query_citations.bind(1, doi);
        while (query_citations.executeStep()) {
          size_t year = query_citations.getColumn(0).getInt();
          size_t citations = query_citations.getColumn(1).getInt();
          word_to_kqr[keyword]._yearToCitations[year] += citations;
          word_to_kqr[keyword]._totalCitations += citations;
        }
        word_to_kqr[keyword]._papers.insert(doi);
        word_to_kqr[keyword]._type.insert(KeywordType::SubjectArea);
        word_to_kqr[keyword]._word = keyword;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  // Convert the unordered_map to a vector of pairs
  std::vector<KeywordQueryResult> ret;
  for (const auto& entry : word_to_kqr) {
    ret.push_back(entry.second);
  }

  return ret;
}

// Function to filter sentences based on a keyword
std::vector<KeywordQueryResult> filterKeywordsRegex(
    const std::string& keywordRegex,
    const std::vector<KeywordQueryResult>& kqr_vec) {
  std::vector<KeywordQueryResult> filtered_kqr_vec;

  try {
    std::regex reg(keywordRegex);
    for (const auto& kqr : kqr_vec) {
      if (std::regex_search(kqr._word, reg)) {
        filtered_kqr_vec.push_back(kqr);
      }
    }
  } catch (const std::regex_error& e) {
    messageWarning("Invalid regex: " + keywordRegex);
  }

  return filtered_kqr_vec;
}

std::vector<KeywordQueryResult> searchKeywords(
    const std::string& searchString) {
  static std::vector<KeywordQueryResult> all_words;
  if (all_words.empty()) {
    all_words = queryAllKeywords();
  }

  return filterKeywordsRegex(searchString, all_words);
}
