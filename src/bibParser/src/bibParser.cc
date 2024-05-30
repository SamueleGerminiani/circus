#include "bibParser.hh"

#include <fstream>

#include "bibtexreader.hpp"
#include "message.hh"

BibTeXEntryVector parseBib(const std::string& str) {
  using bibtex::BibTeXEntry;

  boost::optional<BibTeXEntryVector::size_type> expected;

  std::ifstream in(str.c_str());

  messageErrorIf(!in, "could not open file " + str);

  BibTeXEntryVector ev;
  bool result = read(in, ev);
  messageErrorIf(!result, "error parsing file " + str);
  in.close();

  return ev;
}
