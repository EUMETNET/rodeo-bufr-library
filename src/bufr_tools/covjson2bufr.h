#ifndef _COVJSON2BUFR_H_
#define _COVJSON2BUFR_H_

#include <map>
#include <string>

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
struct val_lev
find_standard_value(std::pair<std::string, std::map<std::string, double>> t,
                    std::string standard_name, std::string level,
                    std::string method, std::string period);

#endif
