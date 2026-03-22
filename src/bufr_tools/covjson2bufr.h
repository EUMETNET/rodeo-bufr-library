#ifndef _COVJSON2BUFR_H_
#define _COVJSON2BUFR_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "NorBufr.h"

struct ret_bufr {
  char *buffer = nullptr;
  size_t size = 0;
};

struct val_lev {
  double value = 0.0;
  std::string level = "0";
};

struct ret_bufr covjson2bufr(std::string covjson_str,
                             std::string bufr_template = "default",
                             NorBufr *bufr = nullptr, bool time_now = false);
struct ret_bufr covjson2bufr_default(std::string covjson_str,
                                     NorBufr *bufr = nullptr,
                                     bool time_bow = false);

bool encoding_coverage(rapidjson::Value::ConstValueIterator it,
                       std::string wigosId);

std::map<std::string, double>::iterator
find_standard_value(std::map<std::string, double>::iterator beg,
                    std::map<std::string, double>::iterator end,
                    std::string standard_name, std::string level,
                    std::string method, std::string period);

                    std::vector<struct val_lev>
find_standard_value(std::pair<std::string, std::map<std::string, double>> t,
                    std::string standard_name, std::string level,
                    std::string method, std::string period);

std::set<std::string>
find_parameter_names(std::pair<std::string, std::map<std::string, double>> t,
                     std::string standard_name, std::string level,
                     std::string method, std::string period);

int periodstr_to_int(std::string);

#endif
