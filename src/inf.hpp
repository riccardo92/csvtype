/**
 * inf.h
 *
 * Header file for inf.cpp
 *
 * Author: Riccardo Tiebax
 * Contact: riccardo.t@gmail.com
* Copyright 2020 R. Tiebax
 *
 */

#ifndef SRC_INF_HPP_
#define SRC_INF_HPP_

#include <string>
#include <cstdlib>
#include <iostream>
#include <regex> // NOLINT
#include <bitset>
#include <cstdint>
#include <vector>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <boost/regex.hpp>
#include <boost/algorithm/string/join.hpp>
#include "../vendor/csv-parser/parser.hpp"

class Inf {
 public:
  std::string filepath;
  char delimiter;
  std::unordered_map<std::string, std::vector<boost::regex>> colTypePatterns;
  std::unordered_set<std::string> naValues;
  bool multithreading;
  bool saveTypesFile;
  std::string typesFilepath;
  int rollingCacheWindow;

  Inf(
    std::string filepath,
    char delimiter,
    std::unordered_map<std::string, std::vector<std::string>> colTypePatterns,
    std::unordered_set<std::string> naValues,
    bool multithreading,
    bool safeTypesFile,
    std::string typesFilepath,
    int rollingCacheWindow);
  void inferTypes();
  void getColType(
    const std::vector<std::string> &fields,
    int c,
    std::unordered_map<std::string, std::vector<boost::regex>>
      &colTypePatterns, // NOLINT
    std::vector<std::string> &types); // NOLINT
  void setColTypePatterns(
    std::unordered_map<std::string, std::vector<std::string>>);
  void setNaValues(std::unordered_set<std::string>);
  void setFilepath(std::string);
  void setDelimiter(char);
  void setMultithreading(bool);
  void setSaveTypesFile(bool);
  void setTypesFilepath(std::string);
  void setRollingCacheWindow(int);
  std::unordered_map<std::string, std::vector<std::string>>
    getColTypePatterns();
  std::unordered_set<std::string> getNaValues();
  std::string getFilepath();
  char getDelimiter();
  bool getMultithreading();
  bool getSaveTypesFile();
  std::string getTypesFilepath();
  int getRollingCacheWindow();
  int getNumCols();
  uint64_t getNumRows();
  std::vector<std::string> getColNames();
  std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>>
    getColTypeCandidates();

 private:
  std::unordered_map<std::string, std::string> typeCache;
  std::unordered_map<std::string, std::vector<std::string>>
    regexOrdering;
  std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>>
    colTypeCandidates;
  uint64_t n;
  int untitledCols;
  std::vector<std::string> columns;
};

#endif  // SRC_INF_HPP_
