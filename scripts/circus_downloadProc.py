import argparse
from pybliometrics.scopus import ScopusSearch
from bibtexparser.bwriter import BibTexWriter
from bibtexparser.bibdatabase import BibDatabase


def search_scopus(query):
    s = ScopusSearch(query, verbose=True)
    return s.results


def convert_to_bibtex(results):
    bib_entries = []
    for result in results:
        entry = {
            "ENTRYTYPE": "article",
            "ID": result.eid,
            "doi": result.doi,
            "eid": result.eid,
            "title": result.title,
            "journal": result.publicationName,
            "year": result.coverDate[:4],
        }

        if entry["doi"] is None:
            entry["doi"] = ""

        bib_entries.append(entry)

    bib_database = BibDatabase()
    bib_database.entries = bib_entries
    return bib_database


def save_to_bibtex(bib_database, filename):
    writer = BibTexWriter()
    with open(filename, "w") as bibfile:
        bibfile.write(writer.write(bib_database))


def main():
    parser = argparse.ArgumentParser(
        description="Search Scopus and save results to a bib file"
    )
    parser.add_argument(
        "-query", required=True, help="Search string for Scopus"
    )
    parser.add_argument(
        "-out", default="results.bib", help="Output bib file name"
    )
    args = parser.parse_args()

    results = search_scopus(args.query)
    bib_database = convert_to_bibtex(results)
    save_to_bibtex(bib_database, args.out)
    print(f"Results saved to {args.out}")


if __name__ == "__main__":
    main()
