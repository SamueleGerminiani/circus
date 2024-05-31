#pragma once
#include <string>
#include <vector>
namespace bibtex {
struct BibTeXEntry;
}

typedef std::vector<bibtex::BibTeXEntry> BibTeXEntryVector;

BibTeXEntryVector parseBib(const std::string& str);
