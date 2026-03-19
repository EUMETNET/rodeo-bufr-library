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

// meas[rodeo:wigosId][time][parameter] = value
std::map<std::string, std::map<std::string, std::map<std::string, double>>>
    meas;

// unit_str[rodeo:wigosId][parameter] = unit
std::map<std::string, std::map<std::string, std::string>> unit;

// geo_loc[rodeo:wigosId][axis_x] = latitude
// geo_loc[rodeo:wigosId][axis_y] = longitude
std::map<std::string, std::map<std::string, double>> geo_loc;

struct ret_bufr covjson2bufr(std::string covjson_str, std::string bufr_template,
                             NorBufr *bufr, bool time_now) {
  struct ret_bufr ret;
  meas.clear();
  geo_loc.clear();
  unit.clear();

  if (bufr_template == "default")
    return covjson2bufr_default(covjson_str, bufr, time_now);
  std::cerr << "Unknown BUFR template name: " << bufr_template << "\n";
  return ret;
}

bool encoding_coverage(rapidjson::Value::ConstValueIterator it,
                       std::string wigosId) {

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
    double axis_z;
    std::vector<std::string> axis_t;

    if (!strcmp(cov_it->name.GetString(), "domain")) {
      if (cov_it->value.HasMember("type") &&
          cov_it->value["type"] == "Domain") {

        if (cov_it->value.HasMember("domainType") &&
            cov_it->value["domainType"] == "PointSeries") {

          axis_x = cov_it->value["axes"]["x"]["values"][0].GetDouble();
          axis_y = cov_it->value["axes"]["y"]["values"][0].GetDouble();
          if (cov_it->value["axes"].HasMember("z")) {
            axis_z = cov_it->value["axes"]["z"]["values"][0].GetDouble();
          } else
            axis_z = 0.0;
          geo_loc[wigosId]["lat"] = axis_x;
          geo_loc[wigosId]["lon"] = axis_y;
          geo_loc[wigosId]["hei"] = axis_z;

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
                double dvalue = rng_it->value["values"][t_index].GetDouble();

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

  return true;
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

  std::string wigosId;

  if (covjson.HasMember("coverages") && covjson["coverages"].IsArray()) {
    for (rapidjson::Value::ConstValueIterator it = covjson["coverages"].Begin();
         it != covjson["coverages"].End(); ++it) {

      if (it->HasMember("metocean:wigosId")) {
        wigosId = (*it)["metocean:wigosId"].GetString();
      }
      encoding_coverage(it, wigosId);
    }
  } else {
    if (covjson.HasMember("type") && covjson["type"] == "Coverage") {
      if (covjson.HasMember("metocean:wigosId")) {
        wigosId = covjson["metocean:wigosId"].GetString();

        rapidjson::Value rjarray(rapidjson::kArrayType);
        rapidjson::Document::AllocatorType &allocator = covjson.GetAllocator();
        rjarray.PushBack(covjson, allocator);
        rapidjson::Value::ConstValueIterator it = rjarray.Begin();
        encoding_coverage(it, wigosId);
      }
    }
  }

  std::set<std::string> params_prec;
  std::set<std::string> params_temp;
  std::set<std::string> params_rad;
  std::set<std::string> params_rad_minmax;

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
      auto params_prec_m =
          find_parameter_names(*t, "precipitation_amount", "", "sum", "");
      params_prec.merge(params_prec_m);

      auto params_max_m =
          find_parameter_names(*t, "air_temperature", "", "maximum", "");
      params_temp.merge(params_max_m);

      auto params_min_m =
          find_parameter_names(*t, "air_temperature", "", "minimum", "");
      params_temp.merge(params_min_m);

      auto params_ldrad_sum = find_parameter_names(
          *t, "integral_wrt_time_of_surface_downwelling_longwave_flux_in_air",
          "", "sum", "");
      params_rad.merge(params_ldrad_sum);
      auto params_ldrad_min = find_parameter_names(
          *t, "integral_wrt_time_of_surface_downwelling_longwave_flux_in_air",
          "", "minimum", "");
      params_rad_minmax.merge(params_ldrad_min);
      auto params_ldrad_max = find_parameter_names(
          *t, "integral_wrt_time_of_surface_downwelling_longwave_flux_in_air",
          "", "maximum", "");
      params_rad_minmax.merge(params_ldrad_max);

      auto params_sdrad_sum = find_parameter_names(
          *t, "",
          "integral_wrt_time_of_surface_downwelling_shortwave_flux_in_air",
          "sum", "");
      params_rad.merge(params_sdrad_sum);
      auto params_sdrad_min = find_parameter_names(
          *t, "integral_wrt_time_of_surface_downwelling_shortwave_flux_in_air",
          "", "minimum", "");
      params_rad_minmax.merge(params_sdrad_min);
      auto params_sdrad_max = find_parameter_names(
          *t, "integral_wrt_time_of_surface_downwelling_shortwave_flux_in_air",
          "", "maximum", "");
      params_rad_minmax.merge(params_sdrad_max);

      auto params_nlrad_sum = find_parameter_names(
          *t, "", "integral_wrt_time_of_surface_net_downward_longwave_flux",
          "sum", "");
      params_rad.merge(params_nlrad_sum);
      auto params_nsrad_sum = find_parameter_names(
          *t, "", "integral_wrt_time_of_surface_net_downward_shortwave_flux",
          "sum", "");
      params_rad.merge(params_nsrad_sum);
    }
  }

  params_prec.erase("PT1D");
  params_prec.erase("PT24H");

  std::vector<std::string> params_pa;
  if (params_prec.size()) {
    params_pa.assign(params_prec.begin(), params_prec.end());
  }

  std::vector<std::string> params(params_temp.begin(), params_temp.end());

  std::vector<std::string> params_rd(params_rad.begin(), params_rad.end());
  std::vector<std::string> params_rd_mm(params_rad_minmax.begin(),
                                        params_rad_minmax.end());

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
      bufr->addValue(geo_loc[w->first]["hei"]); // Height of station

      bufr->addValue("MISSING"); // Height of arometer

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

      // Extreme temperature data: [0 07 032] [0 04 024] [0 12 111] [0 04 024]
      // [0 12 112] Instead of 3 02 041, because time unit is hour

      if (params.size()) {
        if (!subsets) {
          bufr->addDescriptor("105000");
          bufr->addDescriptor("031001");
        }
        bufr->addValue(params.size());

        for (int i = 0; i < params.size(); ++i) {

          std::string temp_max_value = "MISSING";
          std::string temp_min_value = "MISSING";

          struct val_lev temp_max = find_standard_value(
              *t, "air_temperature", "", "maximum", params[i]);
          if (temp_max.level.size()) {
            temp_sensor_level = temp_max.level;
            if (!std::isnan(temp_max.value)) {
              double kelvin_value = unit[w->first]["air_temperature"] == "K"
                                        ? temp_max.value
                                        : temp_max.value + 273.16;
              temp_max_value = std::to_string(kelvin_value);
            }
          }

          int period = periodstr_to_int(params[i]) / 60;
          if (!subsets && !i) {
            bufr->addDescriptor("007032");
          }
          bufr->addValue(temp_sensor_level); // 0 07 032 Height of sensor above
                                             // local ground
          if (!subsets && !i) {
            bufr->addDescriptor("004025");
          }

          bufr->addValue(period); // 0 04 024 Time period or displacement
          if (!subsets && !i) {
            bufr->addDescriptor("012111");
          }
          bufr->addValue(temp_max_value); // 0 12 111 Maximum temperature, at
                                          // height and over period specified

          struct val_lev temp_min = find_standard_value(
              *t, "air_temperature", "", "minimum", params[i]);
          if (temp_min.level.size()) {
            temp_sensor_level = temp_min.level;
            if (!std::isnan(temp_min.value)) {
              double kelvin_value = unit[w->first]["air_temperature"] == "K"
                                        ? temp_min.value
                                        : temp_min.value + 273.16;
              temp_min_value = std::to_string(kelvin_value);
            }
          }

          if (!subsets && !i) {
            bufr->addDescriptor("004025");
          }
          bufr->addValue(period); // 0 04 024 Time period or displacement
          if (!subsets && !i) {
            bufr->addDescriptor("012112");
          }
          bufr->addValue(temp_min_value); // 0 12 112 Minimum temperature, at
                                          // height and over period specified
        }
      }

      /*

      struct val_lev temp_max =
          find_standard_value(*t, "air_temperature", "", "maximum", "PT10M");
      if (temp_max.level.size()) {
        temp_sensor_level = temp_max.level;
        if (!std::isnan(temp_max.value)) {
          double kelvin_value = unit[w->first]["air_temperature"] == "K"
                                    ? temp_max.value
                                    : temp_max.value + 273.16;
          temp_value = std::to_string(kelvin_value);
          std::cout << temp_value << "[" << temp_max.value << "] ";
        }
      }

      */

      // WIND
      if (!subsets) {
        bufr->addDescriptor("302042");
      }

      std::string wind_speed_value = "MISSING";
      std::string wind_dir_value = "MISSING";
      std::string wind_period_value = "MISSING";
      std::string wind_sensor_level = "MISSING";

      auto params_wind_s =
          find_parameter_names(*t, "wind_speed", "", "point", "");

      std::vector<std::string> params_wind(params_wind_s.begin(),
                                           params_wind_s.end());

      if (params_wind.size()) {
        struct val_lev wind_speed =
            find_standard_value(*t, "wind_speed", "", "point", params_wind[0]);
        struct val_lev wind_dir = find_standard_value(
            *t, "wind_from_direction", "", "point", params_wind[0]);

        if (wind_speed.level.size()) {
          wind_sensor_level = wind_speed.level;
          if (!std::isnan(wind_speed.value)) {
            wind_speed_value = std::to_string(wind_speed.value);
          }
        }

        if (wind_dir.level.size()) {
          // wind_sensor_level = wind_dir.level;
          if (!std::isnan(wind_dir.value)) {
            wind_dir_value = std::to_string(wind_dir.value);
          }
        }
        int period = periodstr_to_int(params_wind[0]) / 60;
        wind_period_value = std::to_string(period);
      }

      std::vector<std::string> wind_gust_speed_value = {"MISSING", "MISSING"};
      std::vector<std::string> wind_gust_dir_value = {"MISSING", "MISSING"};
      std::vector<std::string> wind_gust_period = {"MISSING", "MISSING"};

      auto params_wind_gust_s =
          find_parameter_names(*t, "wind_speed_of_gust", "", "point", "");

      std::vector<std::string> params_wind_gust(params_wind_gust_s.begin(),
                                                params_wind_gust_s.end());
      if (params_wind_gust.size()) {

        int wind_guest_count =
            params_wind.size() < 2 ? static_cast<int>(params_wind.size()) : 2;

        for (int i = 0; i < wind_guest_count; ++i) {

          struct val_lev wind_gust_speed = find_standard_value(
              *t, "wind_speed_of_gust", "", "point", params_wind_gust[i]);

          if (wind_gust_speed.level.size()) {
            if (!std::isnan(wind_gust_speed.value)) {
              wind_gust_speed_value[i] = std::to_string(wind_gust_speed.value);
            }
          }

          if (params_wind_gust.size() >= i) {
            struct val_lev wind_gust_dir =
                find_standard_value(*t, "wind_gust_from_direction", "", "point",
                                    params_wind_gust[i]);

            if (wind_gust_dir.level.size()) {
              if (!std::isnan(wind_gust_dir.value)) {
                wind_gust_dir_value[i] = std::to_string(wind_gust_dir.value);
              }
            }
          }
          int period = periodstr_to_int(params_wind_gust[i]) / 60;
          wind_gust_period[i] = std::to_string(period);
        }
      }

      bufr->addValue(wind_sensor_level); // HEIGHT OF SENSOR ABOVE LOCAL GROUND
      bufr->addValue("MISSING"); // TYPE OF INSTRUMENTATION FOR WIND MEASUREMENT
      bufr->addValue("MISSING"); // TIME SIGNIFICANCE
      bufr->addValue(wind_period_value); // TIME PERIOD OR DISPLACEMENT
      bufr->addValue(wind_dir_value);    // WIND DIRECTION
      bufr->addValue(wind_speed_value);  // WIND SPEED
      bufr->addValue("MISSING");         // TIME SIGNIFICANCE

      // repeat
      bufr->addValue(wind_gust_period[0]);      // TIME PERIOD OR DISPLACEMEN
      bufr->addValue(wind_gust_dir_value[0]);   // MAXIMUM WIND GUST DIRECTION
      bufr->addValue(wind_gust_speed_value[0]); // MAXIMUM WIND GUST SPEED

      bufr->addValue(wind_gust_period[1]);      // TIME PERIOD OR DISPLACEMEN
      bufr->addValue(wind_gust_dir_value[1]);   // MAXIMUM WIND GUST DIRECTION
      bufr->addValue(wind_gust_speed_value[1]); // MAXIMUM WIND GUST SPEED

      if (params_rd.size()) {
        if (!subsets) {
          bufr->addDescriptor("105000");
          bufr->addDescriptor("031001");
        }
        bufr->addValue(params_rd.size());

        for (int i = 0; i < params_rd.size(); ++i) {
          std::string ldrad_value = "MISSING";
          std::string sdrad_value = "MISSING";
          std::string nlrad_value = "MISSING";
          std::string nsrad_value = "MISSING";
          std::string rad_sensor_level;

          struct val_lev ldrad_sum = find_standard_value(
              *t,
              "integral_wrt_time_of_surface_downwelling_longwave_flux_in_air",
              "", "sum", params_rd[i]);

          if (ldrad_sum.level.size()) {
            rad_sensor_level = ldrad_sum.level;
            if (!std::isnan(ldrad_sum.value)) {
              ldrad_value = std::to_string(ldrad_sum.value);
            }
          }

          int period = periodstr_to_int(params_rd[i]) / 60;

          struct val_lev sdrad_sum = find_standard_value(
              *t, "",
              "integral_wrt_time_of_surface_downwelling_shortwave_flux_in_air",
              "sum", params_rd[i]);

          if (sdrad_sum.level.size()) {
            if (!std::isnan(sdrad_sum.value)) {
              sdrad_value = std::to_string(sdrad_sum.value);
            }
          }

          struct val_lev nlrad_sum = find_standard_value(
              *t, "", "integral_wrt_time_of_surface_net_downward_longwave_flux",
              "sum", params_rd[i]);

          if (nlrad_sum.level.size()) {
            if (!std::isnan(nlrad_sum.value)) {
              nlrad_value = std::to_string(nlrad_sum.value);
            }
          }

          struct val_lev nsrad_sum = find_standard_value(
              *t, "",
              "integral_wrt_time_of_surface_net_downward_shortwave_flux", "sum",
              params_rd[i]);

          if (nsrad_sum.level.size()) {
            if (!std::isnan(nsrad_sum.value)) {
              nsrad_value = std::to_string(nsrad_sum.value);
            }
          }

          if (!subsets && !i) {
            bufr->addDescriptor("004025");
          }
          bufr->addValue(period); // 0 04 024 Time period or displacement
          if (!subsets && !i) {
            bufr->addDescriptor("014002");
          }
          bufr->addValue(ldrad_value);
          if (!subsets && !i) {
            bufr->addDescriptor("014004");
          }
          bufr->addValue(sdrad_value);
          if (!subsets && !i) {
            bufr->addDescriptor("014012");
          }
          bufr->addValue(nlrad_value);
          if (!subsets && !i) {
            bufr->addDescriptor("014014");
          }
          bufr->addValue(nsrad_value);
        }
      }

      if (params_rd_mm.size()) {
        if (!subsets) {
          bufr->addDescriptor("105000");
          bufr->addDescriptor("031001");
        }
        bufr->addValue(params_rd_mm.size());

        for (int i = 0; i < params_rd_mm.size(); ++i) {

          std::string ldrad_min_value = "MISSING";
          std::string ldrad_max_value = "MISSING";
          std::string sdrad_min_value = "MISSING";
          std::string sdrad_max_value = "MISSING";
          std::string rad_sensor_level = "MISSING";

          int period = periodstr_to_int(params_rd_mm[i]) / 60;

          // Budapestnel behalucinalja a valtozokat
          struct val_lev ldrad_min = find_standard_value(
              *t,
              "integral_wrt_time_of_surface_downwelling_longwave_flux_in_air",
              "", "minimum", params_rd_mm[i]);

          if (ldrad_min.level.size()) {
            // rad_sensor_level = ldrad_min.level;
            if (!std::isnan(ldrad_min.value)) {
              ldrad_min_value = std::to_string(ldrad_min.value);
            }
          }

          struct val_lev ldrad_max = find_standard_value(
              *t,
              "integral_wrt_time_of_surface_downwelling_longwave_flux_in_air",
              "", "maximum", params_rd_mm[i]);

          if (ldrad_max.level.size()) {
            if (!std::isnan(ldrad_max.value)) {
              ldrad_max_value = std::to_string(ldrad_max.value);
            }
          }

          struct val_lev sdrad_min = find_standard_value(
              *t, "",
              "integral_wrt_time_of_surface_downwelling_shortwave_flux_in_air",
              "minimum", params_rd_mm[i]);

          if (sdrad_min.level.size()) {
            if (!std::isnan(sdrad_min.value)) {
              sdrad_min_value = std::to_string(sdrad_min.value);
            }
          }

          struct val_lev sdrad_max = find_standard_value(
              *t, "",
              "integral_wrt_time_of_surface_downwelling_shortwave_flux_in_air",
              "maximum", params_rd_mm[i]);

          if (sdrad_max.level.size()) {
            if (!std::isnan(sdrad_max.value)) {
              sdrad_max_value = std::to_string(sdrad_max.value);
            }
          }

          if (!subsets && !i) {
            bufr->addDescriptor("004025");
          }
          bufr->addValue(period); // 0 04 025 Time period or displacement

          if (!subsets && !i) {
            bufr->addDescriptor("014002");
          }
          bufr->addValue(ldrad_max_value);

          if (!subsets && !i) {
            bufr->addDescriptor("014002");
          }
          bufr->addValue(ldrad_min_value);

          if (!subsets && !i) {
            bufr->addDescriptor("014004");
          }
          bufr->addValue(sdrad_max_value);

          if (!subsets && !i) {
            bufr->addDescriptor("014004");
          }
          bufr->addValue(sdrad_min_value);
        }
      }

      if (params_pa.size()) {
        if (!subsets) {
          bufr->addDescriptor("103000");
          bufr->addDescriptor("031001");
        }
        bufr->addValue(params_pa.size());
        // for (int i = 0; i < params_pa.size(); ++i) {
        for (int i = 0; i < params_pa.size(); ++i) {

          std::string prec_amount_value = "MISSING";

          struct val_lev prec_amount = find_standard_value(
              *t, "precipitation_amount", "", "sum", params_pa[i]);
          if (prec_amount.level.size()) {
            prec24_sensor_level = prec_amount.level;
            if (!std::isnan(prec_amount.value)) {
              prec_amount_value = std::to_string(prec_amount.value);
            }
          }

          int period = periodstr_to_int(params_pa[i]) / 60;

          if (!subsets && !i) {
            bufr->addDescriptor("007032");
          }
          bufr->addValue(prec24_sensor_level); // 0 07 032 Height of sensor
                                               // above local ground
          if (!subsets && !i) {
            bufr->addDescriptor("004025");
          }
          bufr->addValue(period); // 0 04 024 Time period or displacement

          if (!subsets && !i) {
            bufr->addDescriptor("013011");
          }
          bufr->addValue(
              prec_amount_value); // 0 13 011 Total precipitation/total water
                                  // equivalent
        }
      }

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

std::set<std::string>
find_parameter_names(std::pair<std::string, std::map<std::string, double>> t,
                     std::string standard_name, std::string level,
                     std::string method, std::string period) {

  std::set<std::string> ret;

  auto range = t.second.begin();
  // while ( range != t.second.end()) {
  do {
    range = std::find_if(
        range, t.second.end(),
        [standard_name, method,
         period](const std::pair<std::string, double> &tt) -> bool {
          size_t ci = tt.first.rfind(':');
          bool retr =
              (tt.first.substr(0, standard_name.size()) == standard_name &&
               tt.first.substr(ci - method.size() - period.size() - 1,
                               method.size() + 2) == (":" + method + ":"));
          return retr;
        });
    if (range != t.second.end()) {
      std::string param_name = range->first.substr(range->first.rfind(':') + 1);
      ret.insert(param_name);
      range++;
    }
  } while (range != t.second.end());

  return ret;
}

int periodstr_to_int(std::string pstr) {
  int ret = 0;
  if (pstr.size()) {
    if (pstr.substr(0, 2) != "PT") {
      std::cout << "Unknown period: " << pstr << "\n";
    } else {
      int period = stoi(pstr.substr(2, -1));
      switch (pstr.back()) {
      case 'S':
        ret = period;
        break;
      case 'M':
        ret = period * 60;
        break;
      case 'H':
        ret = period * 60 * 60;
        break;
      case 'D':
        ret = period * 60 * 60 * 24;
        break;
      default:
        std::cout << "Unknown period unit:" << pstr << "\n";
      }
    }
  }
  return -ret;
}
