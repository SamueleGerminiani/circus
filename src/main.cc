#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "DBPayload.hh"
#include "bibParser.hh"
#include "bibtexentry.hpp"
#include "commandLineParser.hh"
#include "db.hh"
#include "globals.hh"
#include "gui.hh"
#include "message.hh"

/// @brief handle all the command line arguments
static void parseCommandLineArguments(int argc, char *args[]);

/// @brief handles all the unhandled exceptions
void exceptionHandler();

/// @brief handles all the unhandled errors
void handleErrors();

int main(int arg, char *argv[]) {
  //#ifndef DEBUG
  //  handleErrors();
  //#endif

  // enforce deterministic rand
  srand(1);

  parseCommandLineArguments(arg, argv);

  std::string db_name = "research_papers.db";
  createDB();
  openDB();

  if (!clc::bibFiles.empty()) {
    for (auto &bf : clc::bibFiles) {
      auto bev = parseBib(bf);
      for (bibtex::BibTeXEntry &e : bev) {
        DBPayload payload = toDBPayload(e);
        insertPaper(payload);
      }
    }
  }

  // print welcome message
  // std::cout << getIcon() << "\n";

  // printAllTables(db);
  // printIndexTerms(db);
  // std::string keyword = "energy efficient";
  //// measure time with chrono in milliseconds
  // auto y_c = getCitations(keyword, db);

  // std::cout << "Citations for keyword: " << keyword << "\n";
  //// backwards iteration
  // for (auto it = y_c.rbegin(); it != y_c.rend(); ++it) {
  //   std::cout << it->first << " : " << it->second << "\n";
  // }
  runGui(arg, argv);
  return 0;
}

void exceptionHandler() {
  hlog::dumpErrorToFile("Exception received", errno, -1, true);
  exit(EXIT_FAILURE);
}

void handleErrors() {
  pid_t pid = fork();

  if (pid == -1) {
    messageError("Fork failed");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    // handle catchable errors
    std::set_terminate(exceptionHandler);
    return;
  } else {
    // handle uncatchable errors
    int status;
    waitpid(pid, &status, 0);

    if (WIFSIGNALED(status)) {
      // abnormal termination with a signal
      hlog::dumpErrorToFile("Abnormal termination with a signal",
                            WIFEXITED(status) ? WEXITSTATUS(status) : -1,
                            WTERMSIG(status));
      std::cerr << "Abnormal termination with signal " << WTERMSIG(status)
                << "\n";
      exit(WTERMSIG(status));
    } else if (WIFEXITED(status)) {
      // normal exit
      exit(WEXITSTATUS(status));
    } else {
      // abnormal exit
      hlog::dumpErrorToFile("Abnormal termination");
      std::cerr << "Abnormal termination with status " << WEXITSTATUS(status)
                << "\n";
      exit(WEXITSTATUS(status));
    }
  }

  exit(0);
}

void parseCommandLineArguments(int argc, char *args[]) {
  // parse the cmd using an external library
  auto result = parseCircus(argc, args);

  if (result.count("load-bib-data")) {
    std::string bibPath = result["load-bib-data"].as<std::string>();
    messageErrorIf(bibPath.back() == '/' && !std::filesystem::exists(bibPath),
                   "Can not find directory path '" + bibPath + "'");
    if (std::filesystem::is_directory(bibPath)) {
      for (const auto &entry : std::filesystem::directory_iterator(bibPath)) {
        if (entry.path().extension() == ".bib") {
          clc::bibFiles.push_back(entry.path().u8string());
        }
      }
    } else {
      clc::bibFiles.push_back(bibPath);
    }
  }
}
