#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "bibParser.hh"
#include "commandLineParser.hh"
#include "db.hh"
#include "globals.hh"
#include "message.hh"

/// @brief handle all the command line arguments
static void parseCommandLineArguments(int argc, char *args[]);

/// @brief handles all the unhandled exceptions
void exceptionHandler();

/// @brief handles all the unhandled errors
void handleErrors();

int main(int arg, char *argv[]) {
#ifndef DEBUG
  handleErrors();
#endif

  // enforce deterministic rand
  srand(1);

  parseCommandLineArguments(arg, argv);

  std::string db_name = "research_papers.db";
  createDB(db_name);
  auto db = openDB(db_name);

  for (auto &bf : clc::bibFiles) {
    auto bev = parseBib(bf);
    for (auto &e : bev) {
      auto payload = toDBPayload(e);
      insertPaper(db, payload);
    }
  }

  // print welcome message
  // std::cout << getIcon() << "\n";

  printAllTables(db);
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

  if (result.count("in_bib")) {
    clc::bibFiles.push_back(result["in_bib"].as<std::string>());
    messageErrorIf(!std::filesystem::exists(clc::bibFiles[0]),
                   "Can not find bib file '" + clc::bibFiles[0] + "'");
  }
}
