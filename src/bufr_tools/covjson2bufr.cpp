/*
 * (C) Copyright 2023, met.no
 *
 * This file is part of the Norbufr BUFR en/decoder
 *
 * Author: istvans@met.no
 *
 */

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

#include "Descriptor.h"
#include "NorBufr.h"
#include "Tables.h"

#include "covjson2bufr.h"

struct ret_bufr covjson2bufr(std::string covjson_str, std::string bufr_template,
                             NorBufr *bufr, bool time_now) {
  struct ret_bufr ret;
  if (bufr_template == "default")
    return covjson2bufr_default(covjson_str, bufr, time_now);
  std::cerr << "Unknown BUFR template name: " << bufr_template << "\n";
  return ret;
}

struct ret_bufr covjson2bufr_default(std::string covjson_str, NorBufr *bufr,
                                     bool time_now) {

  bool delete_bufr = false;
  if (bufr == nullptr) {
    bufr = new NorBufr;
    delete_bufr = true;
  }

  struct ret_bufr ret = {nullptr, 0};

  rapidjson::Document covjson;

  if (covjson.Parse(covjson_str.c_str()).HasParseError()) {
    std::cerr << "E-SOH covjson message parsing Error!!!\n";
    return ret;
  }

  // meas[rodeo:wigosId][time][parameter] = value
  std::map<std::string, std::map<std::string, std::map<std::string, double>>>
      meas;

  // unit_str[rodeo:wigosId][parameter] = unit
  std::map<std::string, std::map<std::string, std::string>> unit;

  // geo_loc[rodeo:wigosId][axis_x] = latitude
  // geo_loc[rodeo:wigosId][axis_y] = longitude
  std::map<std::string, std::map<std::string, double>> geo_loc;

  if (covjson.HasMember("coverages") && covjson["coverages"].IsArray()) {
    for (rapidjson::Value::ConstValueIterator it = covjson["coverages"].Begin();
         it != covjson["coverages"].End(); ++it) {

      std::string wigosId;
      if (it->HasMember("metocean:wigosId")) {
        wigosId = (*it)["metocean:wigosId"].GetString();
      }

      for (rapidjson::Value::ConstMemberIterator cov_it = it->MemberBegin();
           cov_it != it->MemberEnd(); ++cov_it) {

        if (!strcmp(cov_it->name.GetString(), "type")) {
          if (strcmp(cov_it->value.GetString(), "Coverage")) {
            std::cerr << "WARNING: Unknown coverage type: "
                      << cov_it->value.GetString() << "[Coverage]\n";
            continue;
          } else {
            // COVERAGE type OK
            continue;
          }
        }

        double axis_x;
        double axis_y;
        std::vector<std::string> axis_t;

        if (!strcmp(cov_it->name.GetString(), "domain")) {
          if (cov_it->value.HasMember("type") &&
              cov_it->value["type"] == "Domain") {

            if (cov_it->value.HasMember("domainType") &&
                cov_it->value["domainType"] == "PointSeries") {

              axis_x = cov_it->value["axes"]["x"]["values"][0].GetDouble();
              axis_y = cov_it->value["axes"]["y"]["values"][0].GetDouble();
              geo_loc[wigosId]["lat"] = axis_x;
              geo_loc[wigosId]["lon"] = axis_y;

              // Units
              std::map<std::string, std::string> unit_str;
              if (it->HasMember("parameters")) {
                for (rapidjson::Value::ConstMemberIterator par_it =
                         ((*it)["parameters"]).MemberBegin();
                     par_it != ((*it)["parameters"]).MemberEnd(); ++par_it) {

                  std::string param_name = par_it->name.GetString();
                  std::string unit_str =
                      par_it->value["unit"]["label"]["en"].GetString();
                  unit[wigosId][param_name] = unit_str;
                }
              }

              int t_index = 0;
              for (rapidjson::Value::ConstValueIterator tit =
                       cov_it->value["axes"]["t"]["values"].Begin();
                   tit != cov_it->value["axes"]["t"]["values"].End(); ++tit) {

                axis_t.push_back(tit->GetString());
                if (it->HasMember("ranges")) {

                  for (rapidjson::Value::ConstMemberIterator rng_it =
                           ((*it)["ranges"]).MemberBegin();
                       rng_it != ((*it)["ranges"]).MemberEnd(); ++rng_it) {
                    double dvalue =
                        rng_it->value["values"][t_index].GetDouble();

                    std::string standard_name = rng_it->name.GetString();
                    meas[wigosId][tit->GetString()][standard_name] = dvalue;
                  }
                }

                ++t_index;
              }
            }
          }
        }
      }
    }
  }

  int subsets = 0;
  // Count subsets
  for (auto w = meas.begin(); w != meas.end(); ++w) {
    // std::cerr << w->first << "\n";
    for (auto t = w->second.begin(); t != w->second.end(); ++t) {
      // std::cerr << "\t" << t->first << "\n";
      ++subsets;
      /*
      for (auto s = t->second.begin(); s != t->second.end(); ++s) {
        std::cerr << "\t\t" << s->first << " = " << s->second;
        std::cerr << " unit: " << unit[w->first][s->first] << "\n";
      }
      */
    }
  }

  int test_max_subset = 30000;
  bufr->setSubset(subsets);
  subsets = 0;
  for (auto w = meas.begin(); w != meas.end(); ++w) {
    for (auto t = w->second.begin(); t != w->second.end(); ++t) {
      std::stringstream ss;
      ss << w->first;
      if (!subsets) {
        bufr->addDescriptor("301150");
      }
      for (int we = 0; we < 4; ++we) {
        std::string wi;
        getline(ss, wi, '-');
        bufr->addValue(wi);
      }

      if (!subsets) {
        bufr->addDescriptor("301090");
      }
      bufr->addValue("MISSING");              // WMO Block, TODO: OSCAR
      bufr->addValue("MISSING");              // WMO Station, TODO: OSCAR
      bufr->addValue("MISSING");              // Station or Site name
      bufr->addValue("MISSING");              // Type of Station
      bufr->addValue(t->first.substr(0, 4));  // Year
      bufr->addValue(t->first.substr(5, 2));  // Month
      bufr->addValue(t->first.substr(8, 2));  // Day
      bufr->addValue(t->first.substr(11, 2)); // Hour
      bufr->addValue(t->first.substr(14, 2)); // Minute

      bufr->addValue(geo_loc[w->first]["lat"]); // Latitude
      bufr->addValue(geo_loc[w->first]["lon"]); // Longitude

      bufr->addValue("MISSING"); // Height of station
      bufr->addValue("MISSING"); // Height if arometer

      if (!subsets) {
        bufr->addDescriptor("302031");
      }

      std::string press_value = "MISSING";
      struct val_lev press =
          find_standard_value(*t, "air_pressure", "", "point", "PT0S");
      if (press.level.size()) {
        if (!std::isnan(press.value)) {
          if (unit[w->first]["air_pressure:0.0:point:PT0S"] == "hPa") {
            press.value *= 100;
          }
          press_value = std::to_string(press.value);
        }
      }
      bufr->addValue(press_value);

      std::string press_msl_value = "MISSING";
      struct val_lev press_msl = find_standard_value(
          *t, "air_pressure_at_mean_sea_level", "", "point", "PT0S");
      if (press_msl.level.size()) {
        if (!std::isnan(press_msl.value)) {
          if (unit[w->first]["air_pressure:0.0:point:PT0S"] == "hPa") {
            press_msl.value *= 100;
          }
          press_msl_value = std::to_string(press_msl.value);
        }
      }
      bufr->addValue(press_msl_value);

      bufr->addValue("MISSING"); // 3-HOUR PRESSURE CHANGE
      bufr->addValue("MISSING"); // CHARACTERISTIC OF PRESSURE TENDENCY
      bufr->addValue("MISSING"); // 24-HOUR PRESSURE CHANGE
      bufr->addValue("MISSING"); // PRESSURE
      bufr->addValue("MISSING"); // GEOPOTENTIAL HEIGHT

      // Temperature
      if (!subsets) {
        bufr->addDescriptor("302035");
      }

      std::string temp_value = "MISSING";
      std::string temp_sensor_level = "MISSING";

      struct val_lev temp =
          find_standard_value(*t, "air_temperature", "", "point", "PT0S");
      if (temp.level.size()) {
        temp_sensor_level = temp.level;
        if (!std::isnan(temp.value)) {
          double kelvin_value = unit[w->first]["air_temperature"] == "K"
                                    ? temp.value
                                    : temp.value + 273.16;
          temp_value = std::to_string(kelvin_value);
        }
      }

      bufr->addValue(temp_sensor_level);
      bufr->addValue(temp_value);

      std::string dew_value = "MISSING";
      struct val_lev dew =
          find_standard_value(*t, "dew_point_temperature", "", "point", "PT0S");
      if (dew.level.size()) {
        if (!std::isnan(dew.value)) {
          double kelvin_value = unit[w->first]["dew_point_temperature"] == "K"
                                    ? dew.value
                                    : dew.value + 273.16;
          dew_value = std::to_string(kelvin_value);
        }
      }
      bufr->addValue(dew_value);

      std::string hum_value = "MISSING";
      struct val_lev hum =
          find_standard_value(*t, "relative_humidity", "", "point", "PT0S");
      if (hum.level.size()) {
        if (!std::isnan(hum.value)) {
          hum_value = std::to_string(hum.value);
        }
      }
      bufr->addValue(hum_value);

      // visibility
      bufr->addValue("MISSING"); // visibility sensor height
      bufr->addValue("MISSING"); // visibility

      // 24-H precipitation
      std::string prec24_value = "MISSING";
      std::string prec24_sensor_level = "MISSING";
      struct val_lev prec24 =
          find_standard_value(*t, "precipitation_amount", "", "sum", "PT24H");
      if (prec24.level.size()) {
        if (prec24.level != 0.0)
          prec24_sensor_level = prec24.level;
        if (!std::isnan(prec24.value)) {
          prec24_value = std::to_string(prec24.value);
        }
      }
      bufr->addValue(prec24_sensor_level);
      bufr->addValue(prec24_value);

      // Ceilometer sensor heigth
      bufr->addValue("MISSING"); // cloud sensor hei

      // Cloud layers
      bufr->addValue("MISSING"); // cloud cover total
      bufr->addValue("MISSING"); // vertical significant
      bufr->addValue("MISSING"); // cloud amiount
      bufr->addValue("MISSING"); // cloud base hei
      bufr->addValue("MISSING"); // cloud type
      bufr->addValue("MISSING"); // cloud type
      bufr->addValue("MISSING"); // cloud type

      bufr->addValue(1); // DELAYED DESCRIPTOR REPLICATION FACTOR

      bufr->addValue("MISSING"); // vertical significant
      bufr->addValue("MISSING"); // cloud amiount
      bufr->addValue("MISSING"); // cloud type
      bufr->addValue("MISSING"); // cloud base hei

      if (!subsets) {
        bufr->addDescriptor("302036");
      }
      bufr->addValue(1); // DELAYED DESCRIPTOR REPLICATION FACTOR

      bufr->addValue("MISSING"); // vertical significant
      bufr->addValue("MISSING"); // cloud amiount
      bufr->addValue("MISSING"); // cloud type
      bufr->addValue("MISSING"); // cloud base hei
      bufr->addValue("MISSING"); // cloud top description

      // WIND
      if (!subsets) {
        bufr->addDescriptor("302042");
      }

      std::string wind_speed_value = "MISSING";
      std::string wind_sensor_level = "MISSING";
      struct val_lev wind_s =
          find_standard_value(*t, "wind_speed", "", "point", "PT10M");
      if (wind_s.level.size()) {
        wind_sensor_level = wind_s.level;
        if (!std::isnan(wind_s.value)) {
          wind_speed_value = std::to_string(wind_s.value);
        }
      }
      std::string wind_dir_value = "MISSING";
      struct val_lev wind_d =
          find_standard_value(*t, "wind_from_direction", "", "point", "PT10M");
      if (wind_d.level.size()) {
        if (!std::isnan(wind_d.value)) {
          wind_dir_value = std::to_string(wind_d.value);
        }
      }

      bufr->addValue(wind_sensor_level); // HEIGHT OF SENSOR ABOVE LOCAL GROUND
      bufr->addValue("MISSING"); // TYPE OF INSTRUMENTATION FOR WIND MEASUREMENT
      bufr->addValue("MISSING"); // TIME SIGNIFICANCE
      bufr->addValue("MISSING"); // TIME PERIOD OR DISPLACEMENT
      bufr->addValue(wind_dir_value);   // WIND DIRECTION
      bufr->addValue(wind_speed_value); // WIND SPEED
      bufr->addValue("MISSING");        // TIME SIGNIFICANCE

      // repeat
      bufr->addValue("MISSING"); // TIME PERIOD OR DISPLACEMEN
      bufr->addValue("MISSING"); // MAXIMUM WIND GUST DIRECTION
      bufr->addValue("MISSING"); // MAXIMUM WIND GUST SPEED

      bufr->addValue("MISSING"); // TIME PERIOD OR DISPLACEMEN
      bufr->addValue("MISSING"); // MAXIMUM WIND GUST DIRECTION
      bufr->addValue("MISSING"); // MAXIMUM WIND GUST SPEED

      if (!subsets) {
        bufr->addDescriptor("302040");
      }

      std::string prec1_value = "MISSING";
      std::string prec12_value = "MISSING";
      struct val_lev prec1 =
          find_standard_value(*t, "precipitation_amount", "", "sum", "PT1H");
      if (prec1.level.size()) {
        if (!std::isnan(prec1.value)) {
          prec1_value = std::to_string(prec1.value);
        }
      }
      struct val_lev prec12 =
          find_standard_value(*t, "precipitation_amount", "", "sum", "PT12H");
      if (prec12.level.size()) {
        if (!std::isnan(prec12.value)) {
          prec12_value = std::to_string(prec12.value);
        }
      }

      bufr->addValue(prec24_sensor_level);

      bufr->addValue(-1);
      bufr->addValue(prec1_value);

      bufr->addValue(-12);
      bufr->addValue(prec12_value);

      if (!subsets) {
        bufr->addDescriptor("101002");
        // bufr->addDescriptor("031001");
      }

      // bufr->addValue(2); // DELAYED DESCRIPTOR REPLICATION FACTOR

      if (!subsets) {
        bufr->addDescriptor("302045");
      }

      // LONG-WAVE RADIATION PT1H
      std::string ldrad1_value = "MISSING";
      // LONG-WAVE RADIATION PT12H
      std::string ldrad24_value = "MISSING";

      struct val_lev ldrad24 = find_standard_value(
          *t, "integral_wrt_time_of_surface_downwelling_longwave_flux_in_air",
          "", "sum", "PT24H");
      if (ldrad24.level.size()) {
        if (!std::isnan(ldrad24.value)) {
          ldrad24_value = std::to_string(ldrad24.value);
        }
      }

      struct val_lev ldrad1 = find_standard_value(
          *t, "integral_wrt_time_of_surface_downwelling_longwave_flux_in_air",
          "", "sum", "PT1H");
      if (ldrad1.level.size()) {
        if (!std::isnan(ldrad1.value)) {
          ldrad1_value = std::to_string(ldrad1.value);
        }
      }

      bufr->addValue(-1);           // TIME PERIOD OR DISPLACEMENT
      bufr->addValue(ldrad1_value); // LONG-WAVE RADIATION INTEGRATED OVER 1H
      bufr->addValue(
          "MISSING"); // SHORT-WAVE RADIATION, INTEGRATED OVER PERIOD SPECIFIED
      bufr->addValue(
          "MISSING"); // NET RADIATION, INTEGRATED OVER PERIOD SPECIFIED
      bufr->addValue("MISSING"); // GLOBAL SOLAR RADIATION (HIGH ACCURACY),
                                 // INTEGRATED OVER PERIOD SPECIFIED
      bufr->addValue("MISSING"); //  DIFFUSE SOLAR RADIATION (HIGH ACCURACY),
                                 //  INTEGRATED OVER PERIOD SPECIFIED
      bufr->addValue("MISSING"); // DIRECT SOLAR RADIATION (HIGH ACCURACY),
                                 // INTEGRATED OVER PERIOD SPECIFIED

      bufr->addValue(-24);           // TIME PERIOD OR DISPLACEMENT
      bufr->addValue(ldrad24_value); // LONG-WAVE RADIATION INTEGRATED OVER 12H
      bufr->addValue("MISSING");
      bufr->addValue("MISSING");
      bufr->addValue("MISSING");
      bufr->addValue("MISSING");
      bufr->addValue("MISSING");

      // END of FIRST SUBSET, subset end indicator
      if (!subsets) {
        bufr->addDescriptor(0);
      }

      if (subsets == test_max_subset) {
        goto stream_end;
      }
      ++subsets;
    }
  }

stream_end:

  bufr->encodeBufr();

  // Set Section1 datetime
  if (time_now) {
    time_t now = time(0);
    struct tm curr_dt;
    memset(&curr_dt, 0, sizeof(curr_dt));
#if defined(_MSC_VER)
    curr_dt = *(gmtime(reinterpret_cast<const time_t *const>(&now)));
#else
    gmtime_r(&now, &curr_dt);
#endif

    bufr->setYear(curr_dt.tm_year + 1900);
    bufr->setMonth(curr_dt.tm_mon + 1);
    bufr->setDay(curr_dt.tm_mday);
    bufr->setHour(curr_dt.tm_hour);
    bufr->setMinute(curr_dt.tm_min);
    bufr->setSecond(curr_dt.tm_sec);
  }

  const uint8_t *rbe = bufr->toBuffer();

  ret.buffer = new char[bufr->length()];
  memcpy(ret.buffer, reinterpret_cast<const char *>(rbe), bufr->length());

  if (delete_bufr) {
    delete bufr;
  }
  ret.size = bufr->length();

  return ret;
}

struct val_lev
find_standard_value(std::pair<std::string, std::map<std::string, double>> t,
                    std::string standard_name, std::string level,
                    std::string method, std::string period) {
  struct val_lev ret;

  auto range = std::find_if(
      t.second.begin(), t.second.end(),
      [standard_name, method,
       period](const std::pair<std::string, double> &tt) -> bool {
        bool retr =
            (tt.first.substr(0, standard_name.size()) == standard_name &&
             tt.first.substr(tt.first.size() - method.size() - period.size() -
                                 2,
                             method.size() + period.size() + 2) ==
                 (":" + method + ":" + period));
        return retr;
      });

  if (range != t.second.end()) {
    // std::cerr << "TEMP VALUE: " << prec24->second << "\n";
    auto level_str_beg = range->first.find(':');
    if (level_str_beg != std::string::npos) {
      auto level_str_end = range->first.find(':', level_str_beg + 1);
      if (level_str_end != std::string::npos) {
        ret.level = range->first.substr(level_str_beg + 1,
                                        level_str_end - level_str_beg - 1);
      } else {
        ret.level = "";
      }
    }
    // Different level ?
    if (level.size() && ret.level != level) {
      ret.value = std::numeric_limits<double>::quiet_NaN();
    } else {
      ret.value = range->second;
    }
  } else {
    ret.value = std::numeric_limits<double>::quiet_NaN();
  }

  return ret;
}
