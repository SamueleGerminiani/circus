#include "db.hh"

#include <SQLiteCpp/SQLiteCpp.h>

#include <iostream>
#include <regex>
#include <unordered_map>
#include <unordered_set>

#include "DBPayload.hh"
#include "bibtexentry.hpp"
#include "dbUtils.hh"
#include "globals.hh"
#include "message.hh"

SQLite::Database db(clc::dbFile, SQLite::OPEN_READWRITE);
static std::vector<KeywordQueryResult> all_words;

void addKQR(const KeywordQueryResult& kqr) { all_words.push_back(kqr); }
void removeKQR(const KeywordQueryResult& kqr) {
  all_words.erase(std::remove_if(all_words.begin(), all_words.end(),
                                 [&kqr](const KeywordQueryResult& kqr2) {
                                   return kqr2._word == kqr._word;
                                 }),
                  all_words.end());
}

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
        "year INTEGER NOT NULL, "
        "authors_list TEXT NOT NULL, "
        "abstract TEXT NOT NULL, "
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

std::vector<DBPayload> getPapers(std::string keyword) {
  std::vector<DBPayload> results;
  std::set<std::string> unique_dois;

  try {
    // Prepare a query to search in index_term_paper
    SQLite::Statement indexTermQuery(
        db,
        "SELECT paper.doi, paper.title, paper.year, paper.authors_list, "
        "paper.abstract, paper.total_citations "
        "FROM index_term_paper "
        "JOIN paper ON index_term_paper.doi = paper.doi "
        "WHERE index_term_paper.index_term = ?");

    // Prepare a query to search in author_keyword_paper
    SQLite::Statement authorKeywordQuery(
        db,
        "SELECT paper.doi, paper.title, paper.year, paper.authors_list, "
        "paper.abstract, paper.total_citations "
        "FROM author_keyword_paper "
        "JOIN paper ON author_keyword_paper.doi = paper.doi "
        "WHERE author_keyword_paper.author_keyword = ?");

    // Prepare a query to search in area_paper
    SQLite::Statement areaQuery(
        db,
        "SELECT paper.doi, paper.title, paper.year, paper.authors_list, "
        "paper.abstract, paper.total_citations "
        "FROM area_paper "
        "JOIN paper ON area_paper.doi = paper.doi "
        "WHERE area_paper.area = ?");

    // Bind the exact keyword to all queries
    indexTermQuery.bind(1, keyword);
    authorKeywordQuery.bind(1, keyword);
    areaQuery.bind(1, keyword);

    // Helper lambda to populate DBPayload from query results
    auto populatePayload = [&](SQLite::Statement& query) {
      while (query.executeStep()) {
        DBPayload payload;
        payload.doi = query.getColumn(0).getString();
        payload.title = query.getColumn(1).getString();
        payload.year = query.getColumn(2).getInt();
        payload.authors_list = query.getColumn(3).getString();
        payload.abstract = query.getColumn(4).getString();
        payload.total_citations = query.getColumn(5).getInt();

        // Query for citations associated with the paper
        SQLite::Statement citationQuery(
            db, "SELECT year, number FROM citations WHERE doi = ?");
        citationQuery.bind(1, payload.doi);
        while (citationQuery.executeStep()) {
          int year = citationQuery.getColumn(0).getInt();
          int number = citationQuery.getColumn(1).getInt();
          payload.citations.push_back({year, number});
        }

        // Query for index terms associated with the paper
        SQLite::Statement indexTermQuery(
            db, "SELECT index_term FROM index_term_paper WHERE doi = ?");
        indexTermQuery.bind(1, payload.doi);
        while (indexTermQuery.executeStep()) {
          payload.index_terms.push_back(
              indexTermQuery.getColumn(0).getString());
        }

        // Query for author keywords associated with the paper
        SQLite::Statement authorKeywordQuery(
            db,
            "SELECT author_keyword FROM author_keyword_paper WHERE doi = ?");
        authorKeywordQuery.bind(1, payload.doi);
        while (authorKeywordQuery.executeStep()) {
          payload.author_keywords.push_back(
              authorKeywordQuery.getColumn(0).getString());
        }

        // Query for areas associated with the paper
        SQLite::Statement areaQuery(
            db, "SELECT area FROM area_paper WHERE doi = ?");
        areaQuery.bind(1, payload.doi);
        while (areaQuery.executeStep()) {
          payload.areas.push_back(areaQuery.getColumn(0).getString());
        }

        if (!unique_dois.count(payload.doi)) {
          // Add the populated payload to the results
          results.push_back(payload);
          unique_dois.insert(payload.doi);
        }
      }
    };

    // Execute all queries
    populatePayload(indexTermQuery);
    populatePayload(authorKeywordQuery);
    populatePayload(areaQuery);

  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return results;
}

void insertPaper(const DBPayload& payload) {
  try {
    // Begin a transaction
    SQLite::Transaction transaction(db);

    // Insert into paper table
    {
      SQLite::Statement query(
          db,
          "INSERT INTO paper (doi, title, year, authors_list, abstract, "
          "total_citations) VALUES (?, ?, ?, ?, ?, ?)");
      query.bind(1, payload.doi);
      query.bind(2, payload.title);
      query.bind(3, payload.year);
      query.bind(4, payload.authors_list);
      query.bind(5, payload.abstract);
      query.bind(6, payload.total_citations);
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
  static std::unordered_set<std::string> unique_ids;
  DBPayload payload;

  // Iterate over fields of BibTeXEntry
  for (const auto& field : entry.fields) {
    if (field.first == "doi" || field.first == "eid") {
      if (field.second.front() != "" && payload.doi == "") {
        payload.doi =
            field.second.front();  // Assuming doi/eid is a single value
      }
    } else if (field.first == "title") {
      payload.title = field.second.front();  // Assuming title is a single value
    } else if (field.first == "author") {
      payload.authors_list =
          field.second.front();  // Assuming title is a single value
    } else if (field.first == "year") {
      payload.year =
          stoull(field.second.front());  // Assuming title is a single value
    } else if (field.first == "abstract") {
      payload.abstract =
          field.second.front();  // Assuming asbtract is a single value
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

  messageWarningIf(
      payload.doi == "",
      "DOI/eid not found in BibTeX entry with title: " + payload.title);

  if (unique_ids.count(payload.doi)) {
    messageWarning("Repeated DOI/eid found in BibTeX entry with title: " +
                   payload.title);
  } else {
    unique_ids.insert(payload.doi);
  }

  return payload;
}

KeywordQueryResult getKQR(const std::string& keyword) {
  auto kqr = std::find_if(all_words.begin(), all_words.end(),
                          [&keyword](const KeywordQueryResult& kqr) {
                            return kqr._word == keyword;
                          });
  messageErrorIf(kqr == all_words.end(),
                 "No data found for keyword: " + keyword);
  return *kqr;
}

std::vector<KeywordQueryResult> queryAllKeywords() {
  std::unordered_map<std::string, KeywordQueryResult> word_to_kqr;

  try {
    // Query for all author keywords and calculate total citations
    {
      SQLite::Statement query(db,
                              "SELECT author_keyword, paper.doi, paper.year "
                              "FROM author_keyword_paper join "
                              "paper on author_keyword_paper.doi = paper.doi");

      while (query.executeStep()) {
        std::string keyword = query.getColumn(0).getString();
        std::string doi = query.getColumn(1).getString();
        int pubYear = query.getColumn(2).getInt();

        // Query for citations associated with the DOI
        SQLite::Statement query_citations(
            db, "SELECT year, number FROM citations WHERE doi = ?");
        query_citations.bind(1, doi);
        while (query_citations.executeStep()) {
          size_t year = query_citations.getColumn(0).getInt();
          size_t citations = query_citations.getColumn(1).getInt();
          word_to_kqr[keyword]._yearToCitations[year] += citations;
          word_to_kqr[keyword]._totalCitations += citations;
          if (year == pubYear + 1 || year == pubYear + 2) {
            word_to_kqr[keyword]
                ._yearToCitationInYearOfPapersPublishedThePreviousTwoYears
                    [year] += citations;
          }
        }

        word_to_kqr[keyword]._yearToPapers[pubYear].insert(doi);
        word_to_kqr[keyword]._type.insert(KeywordType::AuthorKeyword);
        word_to_kqr[keyword]._papers.insert(doi);
        // repeated to avoid checking for empty string
        word_to_kqr[keyword]._word = keyword;
      }
    }

    //// Query for all index terms and calculate total citations
    {
      SQLite::Statement query(db,
                              "SELECT index_term, paper.doi, paper.year "
                              "FROM index_term_paper join "
                              "paper on index_term_paper.doi = paper.doi");
      while (query.executeStep()) {
        std::string keyword = query.getColumn(0).getString();
        std::string doi = query.getColumn(1).getString();
        int pubYear = query.getColumn(2).getInt();

        // Query for citations associated with the DOI
        SQLite::Statement query_citations(
            db, "SELECT year, number FROM citations WHERE doi = ?");
        query_citations.bind(1, doi);
        while (query_citations.executeStep()) {
          size_t year = query_citations.getColumn(0).getInt();
          size_t citations = query_citations.getColumn(1).getInt();
          word_to_kqr[keyword]._yearToCitations[year] += citations;
          word_to_kqr[keyword]._totalCitations += citations;
          if (year == pubYear + 1 || year == pubYear + 2) {
            word_to_kqr[keyword]
                ._yearToCitationInYearOfPapersPublishedThePreviousTwoYears
                    [year] += citations;
          }
        }
        word_to_kqr[keyword]._yearToPapers[pubYear].insert(doi);
        word_to_kqr[keyword]._papers.insert(doi);
        word_to_kqr[keyword]._type.insert(KeywordType::IndexTerm);
        word_to_kqr[keyword]._word = keyword;
      }
    }

    // Query for all subject areas and calculate total citations
    {
      SQLite::Statement query(db,
                              "SELECT area, paper.doi, paper.year "
                              "FROM area_paper join "
                              "paper on area_paper.doi = paper.doi");
      while (query.executeStep()) {
        std::string keyword = query.getColumn(0).getString();
        std::string doi = query.getColumn(1).getString();
        size_t pubYear = query.getColumn(2).getInt();

        // Query for citations associated with the DOI
        SQLite::Statement query_citations(
            db, "SELECT year, number FROM citations WHERE doi = ?");
        query_citations.bind(1, doi);
        while (query_citations.executeStep()) {
          size_t year = query_citations.getColumn(0).getInt();
          size_t citations = query_citations.getColumn(1).getInt();
          word_to_kqr[keyword]._yearToCitations[year] += citations;
          word_to_kqr[keyword]._totalCitations += citations;
          if (year == pubYear + 1 || year == pubYear + 2) {
            word_to_kqr[keyword]
                ._yearToCitationInYearOfPapersPublishedThePreviousTwoYears
                    [year] += citations;
          }
        }
        word_to_kqr[keyword]._yearToPapers[pubYear].insert(doi);
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

void addZScore(KeywordQueryResult& result) {
  if (result._yearToCitations.size() < 2) {
    // Not enough data points to calculate Z-score
    result._zScore = 0;
    return;
  }

  std::vector<size_t> citations;
  for (const auto& [year, count] : result._yearToCitations) {
    citations.push_back(count);
  }

  double mean = calculateMean(citations);
  double stdDev = calculateStandardDeviation(citations, mean);

  // Calculate Z-score for the last element
  size_t zSCoreYear = getCurrentYear() - 1;
  messageWarningIf(!result._yearToCitations.count(zSCoreYear),
                   "No data "
                   "found "
                   "for "
                   "year: " +
                       std::to_string(zSCoreYear));
  double lastYearCitations = result._yearToCitations.at(zSCoreYear);
  double lastButOneYearCitations =
      result._yearToCitations.count(zSCoreYear - 1)
          ? result._yearToCitations.at(zSCoreYear - 1)
          : lastYearCitations;
  double avgCitations = (lastYearCitations + lastButOneYearCitations) / 2;

  if (stdDev != 0) {
    result._zScore = (avgCitations - mean) / stdDev;
  } else {
    result._zScore = 0;  // Avoid division by zero
  }
}

std::vector<KeywordQueryResult> searchKeywords(
    const std::string& searchString) {
  if (all_words.empty()) {
    all_words = queryAllKeywords();
    for (auto& kqr : all_words) {
      addZScore(kqr);
    }
  }

  return filterKeywordsRegex(searchString, all_words);
}
