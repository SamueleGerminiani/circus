import argparse
import os
import bibtexparser
from pybliometrics.scopus import AbstractRetrieval
from pybliometrics.scopus import ScopusSearch
from pybliometrics.scopus import CitationOverview
import pandas as pd
import re
from datetime import datetime


def get_citations_per_year(doi_eid, start_year, end_year):
    idType = ""
    id_list = []

    if doi_eid[0] is not None:
        idType = "doi"
        id_list.append(doi_eid[0])
    elif doi_eid[1] is not None:
        idType = "scopus_id"
        # strip the '2-s2.0-' prefix
        scopus_id = doi_eid[1].split("-")[2]
        id_list.append(scopus_id)

    # Create a CitationOverview instance
    co = CitationOverview(
        identifier=id_list,
        start=start_year,
        end=end_year,
        id_type=idType,
        refresh=True,
    )

    return co.cc[0]


def clean(input_string):
    # Use regular expression to remove non-alphabetic characters
    output_string = re.sub(r"[^a-zA-Z]", "", input_string)
    return output_string


def retrieve_data(entry):
    ab = None

    if "doi" in entry and entry["doi"] != "":
        try:
            ab = AbstractRetrieval(
                identifier=entry["doi"], id_type="doi", view="FULL"
            )
            if not ab:
                print("Scopus search failed:", "ab is None")
        except Exception as e:
            print("Scopus search failed:", e)
    elif "eid" in entry and entry["eid"] != "":
        try:
            ab = AbstractRetrieval(
                identifier=entry["eid"], id_type="eid", view="FULL"
            )
            if not ab:
                print("Scopus search failed:", "ab is None")
        except Exception as e:
            print("Scopus search failed:", e)
    elif "title" in entry and entry["title"] != "":
        eid = retrieve_eid(entry["title"])
        if eid:
            try:
                ab = AbstractRetrieval(
                    identifier=eid, id_type="eid", view="FULL"
                )
                if not ab:
                    print("Scopus cearch failed:", "ab is None")
                if ab and (
                    clean(ab.title.lower()) != clean(entry["title"].lower())
                ):
                    print("---------------------------------")
                    print("Potential error due to title mismatch")
                    print(
                        clean(ab.title.lower())
                        + "\n"
                        + clean(entry["title"].lower())
                    )
                    print(entry["title"])
                    print("---------------------------------")
            except Exception as e:
                print("Scopus search failed:", e)

    return ab


def retrieve_eid(title):
    s = None
    try:
        s = ScopusSearch(
            query="TITLE({" + title + "})",
            view="COMPLETE",
            integrity_fields={"eid", "title"},
            integrity_action="warn",
        )
        df = pd.DataFrame(pd.DataFrame(s.results))
        if "eid" in df.columns:
            for index, row in df.iterrows():
                if clean(row["title"].lower()) == clean(title.lower()):
                    return row["eid"]
        else:
            print("Scopus search failed:", "eid column missing")
    except Exception as e:
        print("Scopus search failed:", e)

    return None


def main():
    parser = argparse.ArgumentParser(
        description="Retrieve abstracts using DOI from .bib files"
    )
    parser.add_argument(
        "-in", "--input", nargs="+", required=True, help="Input .bib file(s)"
    )
    parser.add_argument(
        "-out", "--output", required=True, help="Output .bib file"
    )
    args = parser.parse_args()

    input_files = args.input
    output_file = args.output

    ignored_entries = 0
    entries_fixed = 0
    scopus_error = 0

    result_database = bibtexparser.bibdatabase.BibDatabase()
    for file_path in input_files:
        if not os.path.exists(file_path):
            print(f"Warning: File not found: {file_path}")
            continue

        with open(file_path, "r", encoding="utf-8") as f:
            bib_database = bibtexparser.load(f)
            result_database.entries.extend(bib_database.entries)

    fields_to_fix = [
        "doi",
        "author",
        "title",
        "year",
        "author_keywords",
        "index_terms",
        "subject_areas",
        "per_year_citations",
    ]

    for entry in result_database.entries:
        if ("doi" not in entry or entry["doi"] == "") and (
            "eid" not in entry or entry["eid"] == ""
        ):
            ignored_entries += 1
            continue

        retrieved_data = retrieve_data(entry)

        if retrieved_data:
            entries_fixed += 1
            for field in fields_to_fix:
                entry[field] = ""
                if field == "doi":
                    if retrieved_data.doi:
                        entry[field] = retrieved_data.doi
                elif field == "author":
                    if retrieved_data.authors:
                        auth_string = ""
                        for nt in retrieved_data.authors:
                            if nt.surname and nt.given_name is not None:
                                auth_string += (
                                    nt.surname + ", " + nt.given_name + " and "
                                )
                        entry[field] = auth_string[: len(auth_string) - 5]
                elif field == "title":
                    if retrieved_data.title:
                        entry[field] = retrieved_data.title
                elif field == "author_keywords":
                    if retrieved_data.authkeywords:
                        entry[field] = ", ".join(retrieved_data.authkeywords)
                elif field == "index_terms":
                    if retrieved_data.idxterms:
                        entry[field] = ", ".join(retrieved_data.idxterms)
                elif field == "subject_areas":
                    if retrieved_data.subject_areas:
                        subject_areas = []
                        for sa in retrieved_data.subject_areas:
                            subject_areas.append(sa.area)
                        entry[field] = ", ".join(subject_areas)
                elif field == "year":
                    if retrieved_data.coverDate:
                        entry[field] = retrieved_data.coverDate.split("-")[0]
                elif field == "per_year_citations":
                    if retrieved_data.coverDate:
                        year = retrieved_data.coverDate.split("-")[0]
                        doi_eid = (retrieved_data.doi, retrieved_data.eid)
                        pyc = get_citations_per_year(
                            doi_eid, year, datetime.now().year
                        )
                        for year, count in pyc:
                            entry[field] += f"{year}: {count}, "
                    else:
                        ignored_entries += 1
                        continue

                    # remove last comma
                    entry[field] = entry[field][:-2]
        else:
            scopus_error += 1

    with open(output_file, "w", encoding="utf-8") as bibfile:
        bibtexparser.dump(result_database, bibfile)

    print(f"Entries fixed: {entries_fixed}")
    print(f"Ignored entries: {ignored_entries}")
    print(f"Could not fix {scopus_error} entries due to scopus errors")


if __name__ == "__main__":
    main()
