/**
 * inf.cpp
 *
 * CSV file column type inference
 *
 * Author R. Tiebax
 * Contact: riccardo.t@gmail.com
 * Copyright 2020 R. Tiebax
 *
 */

#include "inf.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <thread> // NOLINT
#include <algorithm>
#include <unordered_set>
#include <memory>
#include <boost/algorithm/string/join.hpp>
#include <boost/regex.hpp>
#include "../vendor/csv-parser/parser.hpp"

Inf::Inf(
  std::string filepath,
  char delimiter,
  std::unordered_map<std::string, std::vector<std::string>> colTypePatterns,
  std::unordered_set<std::string> naValues,
  bool multithreading,
  bool saveTypesFile,
  std::string typesFilepath,
  int rollingCacheWindow
) {
  this->filepath = filepath;
  this->delimiter = delimiter;
  this->multithreading = multithreading;
  this->saveTypesFile = saveTypesFile;
  this->typesFilepath = typesFilepath;
  this->rollingCacheWindow = rollingCacheWindow;
  this->naValues = naValues;
  setColTypePatterns(colTypePatterns);
}

void Inf::setFilepath(std::string filepath) {
  this->filepath = filepath;
}

void Inf::setDelimiter(char delimiter) {
  this->delimiter = delimiter;
}

void Inf::setMultithreading(bool multithreading) {
  this->multithreading = multithreading;
}

void Inf::setSaveTypesFile(bool saveTypesFile) {
  this->saveTypesFile = saveTypesFile;
}

void Inf::setTypesFilepath(std::string filepath) {
  this->typesFilepath = filepath;
}

void Inf::setRollingCacheWindow(int window) {
  this->rollingCacheWindow = window;
}

void Inf::setNaValues(std::unordered_set<std::string> values) {
  naValues = values;
}

std::string Inf::getFilepath() {
  return filepath;
}

char Inf::getDelimiter() {
  return delimiter;
}

std::unordered_map<std::string, std::vector<std::string>>
Inf::getColTypePatterns() {
  std::unordered_map<std::string, std::vector<std::string>> patterns;
  for (auto& patternMap : colTypePatterns) {
    patterns[patternMap.first] = std::vector<std::string>();
    for (boost::regex pattern : patternMap.second) {
      patterns[patternMap.first].push_back(pattern.str());
    }
  }
  return patterns;
}

std::unordered_set<std::string> Inf::getNaValues() {
  return naValues;
}

bool Inf::getMultithreading() {
  return multithreading;
}

bool Inf::getSaveTypesFile() {
  return saveTypesFile;
}

std::string Inf::getTypesFilepath() {
  return typesFilepath;
}

int Inf::getRollingCacheWindow() {
  return rollingCacheWindow;
}

int Inf::getNumCols() {
  // Create input file stream
  std::ifstream f(filepath);

  // Initialize csv parser
  aria::csv::CsvParser parser {
    aria::csv::CsvParser(f).delimiter(delimiter)
  };

  // Number of columns
  int numCols{ 0 };

  // Get num cols from first row
  for (auto& row : parser) {
    numCols += row.size();
    break;
  }

  return numCols;
}

void Inf::setColTypePatterns(
  std::unordered_map<std::string, std::vector<std::string>> patterns
) {
  // Clear patterns
  colTypePatterns.clear();

  // Define regex patterns
  for (auto& patternMap : patterns) {
    colTypePatterns[patternMap.first] = std::vector<boost::regex>();
    for (std::string pattern : patternMap.second) {
      colTypePatterns[patternMap.first].push_back(
          boost::regex(pattern, boost::regex::perl));
    }
  }
}

void
Inf::getColType(
  const std::vector<std::string> &fields,
  int c,
  std::unordered_map<std::string, std::vector<boost::regex>> &colTypePatterns, // NOLINT
  std::vector<std::string> &types // NOLINT
) {
  // We define a reference to the type cache item here
  // Note that every time we access a map, the hash of
  // the key is recalculated. With a ref, we prevent this.
  std::string &typeCacheEntry = typeCache[fields[c]];

  if (naValues.find(fields[c]) != naValues.end()) {
    // Set NA type if a match was found
    types[c] = "NA";
  } else {
    // Check if in type cache
    if (!typeCacheEntry.empty()) {
        // Retrieve type from cache
        types[c] = typeCacheEntry;
    } else {
      // Set default type to "other"
      types[c] = "other";
      // We make use of a lambda here so that we can
      // return from it and break the two nested loop
      // in once, without makin use of boolean flags
      // or whatever. This makes it a lot faster.
      [&] {
        // Maintain type index
        int ti = 0;
          for (std::string typeKey : regexOrdering[columns[c]]) {
            for (boost::regex typeRegex : colTypePatterns[typeKey]) {
              if (boost::regex_match(fields[c], typeRegex))  {
                types[c] = typeKey;
                if (ti != 0) {
                  // Update most frequently matched
                  // type regex in the ordering,
                  // i.e. set recently matched regex
                  // as element 0.
                  // NOLINT TODO: only do this after x matches
                  // and push elements down instead of
                  // simple swapping.
                  std::swap(
                      regexOrdering[columns[c]][0],
                      regexOrdering[columns[c]][ti]);
                }
                return;
            }
          }
          ti++;
        }
      }();

      // Add to cache
      typeCacheEntry = types[c];
    }
  }

  // Update type candidate count
  colTypeCandidates[columns[c]][types[c]]++;
}

uint64_t Inf::getNumRows() {
  return this->n;
}

std::unordered_map<std::string,
std::unordered_map<std::string, uint64_t>>
Inf::getColTypeCandidates() {
  return this->colTypeCandidates;
}

std::vector<std::string> Inf::getColNames() {
  return this->columns;
}

void
Inf::inferTypes() {
  // Set row counter var
  n = 0;

  // Create input file stream
  std::ifstream file(filepath);
  // Initialize csv parser
  aria::csv::CsvParser parser {
    aria::csv::CsvParser(file).delimiter(delimiter)
  };

  // Create output file stream
  std::ofstream cTypeFile;

  if (saveTypesFile) {
    // Open output file
    cTypeFile.open(typesFilepath);
  }

  // Column counter
  int c{ 0 };

  // Get the number of columns
  int numCols{ getNumCols() };

  // Intialize column name vector
  columns = std::vector<std::string>(numCols);

  // Initialize currLine vector.
  std::string currLine;

  // Initialize types
  std::vector<std::string> types(numCols);

  // Initialize fields
  std::vector<std::string> fields(numCols);

  // Initialize threads vector
  std::vector<std::thread> ft(numCols);

  // Walk through csv lines
  for (auto& row : parser) {
    // Empty currLine
    currLine = "";
    c = 0;

    fields.clear();
    fields = std::vector<std::string> (numCols);
    types.clear();
    types = std::vector<std::string> (numCols);
    if (multithreading) {
      ft.clear();
      ft = std::vector<std::thread>(numCols);
    }

    // Walk through columns
    for (auto& field : row) {
      // Coerce field to strings
      fields[c] = (std::string) field;

      if (n == 0) {
        // Update column arrays
        columns[c] = std::string(fields[c]);
        if (fields[c].empty()) {
          untitledCols++;
          columns[c] = "Unitled_" + std::to_string(untitledCols);
        }

        // Initialize col type candidate map for column
        colTypeCandidates[columns[c]] = std::unordered_map
          <std::string, uint64_t>{};
        // Initialize col type candidate map for column
        for (auto& patternMap : colTypePatterns) {
          colTypeCandidates[columns[c]][patternMap.first] = 0;
        }

        // Manually add counter for "other" and "NA" types because they
        // are non user-defined types.
        colTypeCandidates[columns[c]]["other"] = 0;
        colTypeCandidates[columns[c]]["NA"] = 0;

        // Initialize regex ordering per column
        // We keep this regex ordering because a certain column
        // might be frequently matched by a certain regex pattern,
        // and by accessing the most frequent one first every time,
        // we save time
        regexOrdering[columns[c]] = std::vector<std::string>{};
        for (auto& patternMap : colTypePatterns) {
          regexOrdering[columns[c]].push_back(patternMap.first);
        }

        // Add column header line
        currLine += (c == 0) ? columns[c] : "," + columns[c];

      } else {
        if (multithreading) {
          // Start thread for this field to get type
          // The lambda [& ,c] means that c is passed
          // by value and the of the arguments by ref.
          // This is important because c is constantly
          // modified and would otherwise lead to seg
          // faults.
          ft[c] = std::thread(
            [&, c] {
              getColType(
                fields,
                c,
                colTypePatterns,
                types);
            });

        } else {
          // Get type for current column
          getColType(
            fields,
            c,
            colTypePatterns,
            types);

          // Add types line
          currLine += (c == 0) ? types[c] : "," + types[c];
        }
      }
      c++;
    }

    // Join field type threads to main thread and build currFile string
    if (multithreading) {
      for (int j = 0; j < numCols; j++) {
        if (ft[j].joinable()) {
          ft[j].join();
          currLine += (j == 0) ? types[j] : "," + types[j];
        }
      }
    }

    // Write line to file
    if (saveTypesFile) {
      cTypeFile << currLine + "\n";
    }
    n++;

    // By emptying the type cache every x rows,
    // we keep a "rolling" cache that speeds up computation
    // a lot (1/6 reduction).
    if (n % rollingCacheWindow == 0) {
      typeCache.clear();
    }
  }

  // Close cell type file stream
  if (saveTypesFile) {
    cTypeFile.close();
  }

  // Reduce n by one (exclude header)
  n--;
}

// Python bindings
PYBIND11_MODULE(csvtype_ext, m) {
  m.doc() = "The csvtype Python interface";
  pybind11::class_<Inf>(m, "Inf").def(
    pybind11::init<
        std::string,
        char,
        std::unordered_map<std::string, std::vector<std::string>>,
        std::unordered_set<std::string>,
        bool,
        bool,
        std::string,
        int>(),
    pybind11::arg("filepath"),
    pybind11::arg("delimiter"),
    pybind11::arg("col_type_patterns"),
    pybind11::arg("na_values"),
    pybind11::arg("multithreading"),
    pybind11::arg("save_types_file"),
    pybind11::arg("types_filepath"),
    pybind11::arg("rolling_cache_window")
  )
  .def("infer_types", &Inf::inferTypes)
  .def("set_col_type_patterns", &Inf::setColTypePatterns)
  .def("set_na_values", &Inf::setNaValues)
  .def("set_filepath", &Inf::setFilepath)
  .def("set_delimiter", &Inf::setDelimiter)
  .def("set_multithreading", &Inf::setMultithreading)
  .def("set_save_types_file", &Inf::setSaveTypesFile)
  .def("set_types_filepath", &Inf::setTypesFilepath)
  .def("set_rolling_cache_window", &Inf::setRollingCacheWindow)
  .def("get_col_type_patterns", &Inf::getColTypePatterns)
  .def("get_na_values", &Inf::getNaValues)
  .def("get_filepath", &Inf::getFilepath)
  .def("get_delimiter", &Inf::getDelimiter)
  .def("get_multithreading", &Inf::getMultithreading)
  .def("get_save_types_file", &Inf::getSaveTypesFile)
  .def("get_types_filepath", &Inf::getTypesFilepath)
  .def("get_rolling_cache_window", &Inf::getRollingCacheWindow)
  .def("get_col_type_candidates", &Inf::getColTypeCandidates)
  .def_property_readonly("num_rows", &Inf::getNumRows)
  .def_property_readonly("col_names", &Inf::getColNames);
}
