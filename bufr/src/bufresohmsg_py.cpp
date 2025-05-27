/*
 * (C) Copyright 2023, Eumetnet
 *
 * This file is part of the E-SOH Norbufr BUFR en/decoder interface
 *
 * Author: istvans@met.no
 *
 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <sstream>
#include <string>

#include "Descriptor.h"
#include "ESOHBufr.h"
#include "Tables.h"
#include "bufresohmsg_py.h"

struct vc_struct {
  int version;
  int centre;
};

// Radar table files, for example: localtabb_85_10.csv
vc_struct get_versioncentre_from_table_filename(std::string filename) {
  struct vc_struct vc;
  auto vers_s = filename.find(std::string("_"));
  auto vers_e = filename.find(std::string("_"), vers_s + 1);
  if (vers_e == std::string::npos) {
    vers_e = filename.find(std::string("."), vers_s + 1);
    vc.version = std::stoi(filename.substr(vers_s + 1, vers_e - vers_s - 1));
  } else {
    auto centre_e = filename.find(std::string("."), vers_e);
    vc.centre = std::stoi(filename.substr(vers_s + 1, vers_e - vers_s - 1));
    vc.version = std::stoi(filename.substr(vers_e + 1, centre_e - vers_e - 1));
  }
  return vc;
}

bool norbufr_init_bufrtables(std::string tables_dir) {

  if (tb.size() || tc.size() || td.size())
    return false;
  std::string tbl_dir;
  if (tables_dir.size()) {
    tbl_dir = tables_dir;
  } else {
    tbl_dir = "/usr/share/eccodes/definitions/bufr/tables/0/wmo";
  }

  for (const auto &entry : std::filesystem::directory_iterator(tbl_dir)) {
    int vers = 0;
    // ECCodes table B files
    if (std::filesystem::is_directory(entry.path())) {
      vers = std::stoi(entry.path().filename().string());
      if (vers > 0) {
        std::string ecc_tableB_path = entry.path().string() + "/element.table";
        if (std::filesystem::is_regular_file(ecc_tableB_path)) {
          TableB tb_e(ecc_tableB_path);
          tb[vers] = tb_e;
        }
        // ECCodes table C files
        std::string ecc_tableC_dir = entry.path().string() + "/codetables";
        if (std::filesystem::is_directory(ecc_tableC_dir)) {
          TableC tc_e(entry.path().string() + "/codetables");
          tc[vers] = tc_e;
        }
      }
    } else {
      // WMO table B files
      if (entry.path().filename() == "BUFRCREX_TableB_en.txt") {
        TableB tb_e(entry.path().string());
        tb[0] = tb_e;
        std::cerr << "WMO tableB: " << entry.path().string() << "\n";
      } else {
        // WMO table C file
        if (entry.path().filename() == "BUFRCREX_CodeFlag_en.txt") {
          TableC tc_e(entry.path().string());
          std::cerr << "Load WMO C table" << entry.path().string() << "\n";
          if (tc.size()) {
            tc[0] += tc_e;
          } else
            tc[0] = tc_e;
        } else {
          // OPERA table B files
          if (entry.path().filename().string().substr(0, 9) == "localtabb" ||
              entry.path().filename().string().substr(0, 8) == "bufrtabb") {
            TableB tb_e(entry.path().string());
            struct vc_struct vc =
                get_versioncentre_from_table_filename(entry.path().string());
            if (entry.path().filename().string().substr(0, 9) != "localtabb") {
              tb[vc.version] = tb_e;
            } else {
              tbl[vc.version][vc.centre] = tb_e;
            }
          }
          // Meteo-France code table: btc085.019
          else {
            std::string mfr_c_path(entry.path().filename().string());
            if (mfr_c_path == "btc085.019") {
              TableC tc_e(entry.path().string());
              if (tc.size()) {
                tc[0] += tc_e;
              } else
                tc[0] = tc_e;
            }
          }
        }
      }
    }
  }

  for (const auto &entry : std::filesystem::directory_iterator(tbl_dir)) {
    int vers = 0;
    // ECCodes table D files
    if (std::filesystem::is_directory(entry.path())) {
      vers = std::stoi(entry.path().filename().string());
      if (vers > 0) {
        std::string ecc_tableD_path = entry.path().string() + "/sequence.def";
        if (std::filesystem::is_regular_file(ecc_tableD_path)) {
          TableD tb_d(ecc_tableD_path);
          td[vers] = tb_d;
        }
      }
    } else {
      // WMO table D files
      if (entry.path().filename() == "BUFR_TableD_en.txt") {
        TableD td_e(entry.path().string());
        td[0] = td_e;
        std::cerr << "WMO tableD: " << entry.path().string() << "\n";
      } else {
        // OPERA table D files
        if (entry.path().filename().string().substr(0, 9) == "localtabd" ||
            entry.path().filename().string().substr(0, 8) == "bufrtabd") {
          TableD td_e(entry.path().string());
          struct vc_struct vc =
              get_versioncentre_from_table_filename(entry.path().string());
          if (entry.path().filename().string().substr(0, 9) != "localtabd") {
            td[vc.version] = td_e;
          } else {
            tdl[vc.version][vc.centre] = td_e;
          }
        }
      }
    }
  }

  if (!tb.size() || !tc.size() || !td.size()) {
    return false;
  }

  return true;
}

bool norbufr_update_bufrtables(std::string tables_dir) {
  tb.clear();
  tc.clear();
  td.clear();
  return norbufr_init_bufrtables(tables_dir);
}

bool norbufr_init_oscar(std::string oscardb_dir) {
  bool ret = oscar.addStation(oscardb_dir.c_str());
  return ret;
}

bool norbufr_init_schema_template(std::string schema_path) {

  if (schema_path.size()) {
    std::string def_msg;
    std::ifstream msgTemplate(schema_path.c_str(), std::ios_base::in);
    char c;
    while (msgTemplate.get(c)) {
      def_msg += c;
    }
    bufr_input_schema = def_msg;
    if (!def_msg.size()) {
      return false;
    }
  }

  return true;
}

std::list<std::string> norbufr_bufresohmsg(std::string fname) {

  std::list<std::string> ret;

  std::ifstream bufrFile(fname.c_str(),
                         std::ios_base::in | std::ios_base::binary);

  std::filesystem::path file_path(fname);
  std::streamsize bufr_size = std::filesystem::file_size(file_path);
  char *fbuf = new char[bufr_size];
  bufrFile.read(fbuf, bufr_size);
  ret = norbufr_bufresohmsgmem(fbuf, bufr_size);
  delete[] fbuf;
  return ret;
}

std::list<std::string> norbufr_bufresohmsgmem(char *api_buf, int api_size) {

  std::list<std::string> ret;
  uint64_t position = 0;
  TableB ltb;
  TableD ltd;

  while (position < static_cast<uint64_t>(api_size)) {

    ESOHBufr *bufr = new ESOHBufr;
    // TODO:
    // bufr->setBufrId(file_path.filename());
    bufr->setOscar(&oscar);
    bufr->setMsgTemplate(bufr_input_schema);
    bufr->setShadowWigos(default_shadow_wigos_py);
    bufr->setRadarCFMap(radar_cf_st);

    uint64_t n = bufr->fromBuffer(api_buf, position, api_size);
    if (n == ULONG_MAX)
      position = ULONG_MAX;
    if (n > position) {
      position = n;

      int tb_index = -1;
      if (tb.size()) {
        tb_index = tb.rbegin()->first;
        if (tb.find(bufr->getVersionMaster()) != tb.end())
          tb_index = bufr->getVersionMaster();
        bufr->setTableB(&tb.at(tb_index));

        int tbl_index_loc = -1;
        int tbl_index_cen = -1;
        if (tbl.size()) {
          tbl_index_loc = bufr->getVersionLocal();
          tbl_index_cen = bufr->getCentre();
          auto tbl_it = tbl.find(tbl_index_loc);
          if (tbl_it != tbl.end()) {
            if (tbl[tbl_index_loc].find(tbl_index_cen) !=
                tbl[tbl_index_loc].end()) {
              ltb = tb[tb_index];
              ltb += tbl[tbl_index_loc][tbl_index_cen];
              bufr->setTableB(&ltb);
            }
          }
        }

        int td_index = -1;
        if (td.size()) {
          td_index = td.rbegin()->first;
          if (td.find(bufr->getVersionMaster()) != td.end())
            td_index = bufr->getVersionMaster();
          bufr->setTableD(&td.at(td_index));

          int tdl_index_loc = -1;
          int tdl_index_cen = -1;
          if (tdl.size()) {
            tdl_index_loc = bufr->getVersionLocal();
            tdl_index_cen = bufr->getCentre();
            auto tdl_it = tdl.find(tdl_index_loc);
            if (tdl_it != tdl.end()) {
              if (tdl[tdl_index_loc].find(tdl_index_cen) !=
                  tdl[tdl_index_loc].end()) {
                ltd = td[td_index];
                ltd += tdl[tdl_index_loc][tdl_index_cen];
                bufr->setTableD(&ltd);
              }
            }
          }
        }
      }

      bufr->extractDescriptors();

      std::list<std::string> msg = bufr->msg();
      bufr->logToCsvList(esoh_bufr_log);
      ret.insert(ret.end(), msg.begin(), msg.end());
    }
    delete bufr;
  }

  return ret;
}

pybind11::bytes norbufr_covjson2bufr(std::string covjson_str) {

  pybind11::bytes ret;

  bool stream_print = false;
  std::string bufr_file_name("test_encoded_out.bufr");

  rapidjson::Document covjson;

  if (covjson.Parse(covjson_str.c_str()).HasParseError()) {
    std::cerr << "E-SOH covjson message parsing Error!!!\n";
    return ret;
  }

  TableB *tb = nullptr;
  // TableC *tc = nullptr;
  TableD *td = nullptr;

  // Default tables: eccodes
  std::string Btable_dir("/usr/share/eccodes/definitions/bufr/tables/0/wmo");
  std::string Ctable_dir("/usr/share/eccodes/definitions/bufr/tables/0/wmo");
  std::string Dtable_dir("/usr/share/eccodes/definitions/bufr/tables/0/wmo");
  std::string Btable_file;
  std::string Ctable_file;
  std::string Dtable_file;

  if (const char *table_dir = std::getenv("BUFR_TABLE_DIR")) {
    Btable_dir = std::string(table_dir);
    Ctable_dir = Dtable_dir = Btable_dir;
  }
  if (const char *table_dir = std::getenv("BUFR_BTABLE_FILE")) {
    Btable_file = std::string(table_dir);
  }
  if (const char *table_dir = std::getenv("BUFR_CTABLE_FILE")) {
    Ctable_file = std::string(table_dir);
  }
  if (const char *table_dir = std::getenv("BUFR_DTABLE_FILE")) {
    Dtable_file = std::string(table_dir);
  }

  NorBufr *bufr = new NorBufr;

  int vers_master = 34;
  bufr->setVersionMaster(vers_master);

  bufr->setLocalDataSubCategory(0);
  bufr->setCentre(0);
  bufr->setObserved(true);

  // Set B Table
  if (!Btable_file.size()) {
    Btable_file =
        Btable_dir + "/" + std::to_string(vers_master) + "/element.table";
  }
  if (!(std::filesystem::is_regular_file(Btable_file) ||
        std::filesystem::is_symlink(Btable_file))) {
    std::cerr << "Table file B not exists: " << Btable_file << "\n";
    return ret;
  }
  tb = new TableB(Btable_file);
  bufr->setTableB(tb);

  // Set D Table
  if (!Dtable_file.size()) {
    Dtable_file =
        Dtable_dir + "/" + std::to_string(vers_master) + "/sequence.def";
  }
  if (!(std::filesystem::is_regular_file(Dtable_file) ||
        std::filesystem::is_symlink(Dtable_file))) {
    std::cerr << "Table file D not exists:" << Dtable_file << "\n";
    return ret;
  }
  td = new TableD(Dtable_file);
  bufr->setTableD(td);

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
      if (it->HasMember("rodeo:wigosId")) {
        wigosId = (*it)["rodeo:wigosId"].GetString();
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
        /*
        for( auto time_list : axis_t) {
          std::cerr << "Lat: " << axis_x << " Lon: " << axis_y << " Time: " <<
        time_list << "\n";
        }

        if (!strcmp(cov_it->name.GetString(), "parameters")) {
          std::cerr << "\t\tParameters\n";
        }

        if (!strcmp(cov_it->name.GetString(), "range")) {
          std::cerr << "\t\tRange\n";
        }
        */
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

  int test_max_subset = 3000;
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

      // Pressure
      auto press = t->second.find("air_pressure:0.0:point:PT0S");
      double d_press = 0;
      if (press != t->second.end()) {
        d_press = press->second;
        if (unit[w->first]["air_pressure:0.0:point:PT0S"] == "hPa") {
          d_press *= 100;
        }
        bufr->addValue(d_press);
      } else {
        bufr->addValue("MISSING");
      }

      // Pressure reduced to mean sea level
      auto press_msl =
          t->second.find("air_pressure_at_mean_sea_level:0.0:point:PT0S");
      if (press_msl != t->second.end()) {
        double d_press_msl = press_msl->second;
        if (unit[w->first]["air_pressure_at_mean_sea_level:0.0:point:PT0S"] ==
            "hPa") {
          d_press_msl *= 100;
        }
        bufr->addValue(d_press_msl);
      } else {
        bufr->addValue("MISSING");
      }

      bufr->addValue("MISSING"); // 3-HOUR PRESSURE CHANGE
      bufr->addValue("MISSING"); // CHARACTERISTIC OF PRESSURE TENDENCY
      bufr->addValue("MISSING"); // 24-HOUR PRESSURE CHANGE
      // PRESSURE
      if (d_press > 0) {
        bufr->addValue(d_press);
      } else {
        bufr->addValue("MISSING");
      }
      bufr->addValue("MISSING"); // GEOPOTENTIAL HEIGHT

      // Temperature
      if (!subsets) {
        bufr->addDescriptor("302035");
      }

      std::string temp_value = "MISSING";
      std::string temp_sensor_level = "MISSING";

      auto temp =
          std::find_if(t->second.begin(), t->second.end(),
                       [](const std::pair<std::string, double> &t) -> bool {
                         return t.first.substr(0, 15) == "air_temperature";
                       });
      if (temp != t->second.end()) {
        auto level_str_beg = temp->first.find(':');
        if (level_str_beg != std::string::npos) {
          auto level_str_end = temp->first.find(':', level_str_beg + 1);
          if (level_str_end != std::string::npos) {
            temp_sensor_level = temp->first.substr(
                level_str_beg + 1, level_str_end - level_str_beg - 1);
          }
        }
        // Unit Conversion
        double kelvin_value = unit[w->first][temp->first] == "K"
                                  ? temp->second
                                  : temp->second + 273.16;
        temp_value = std::to_string(kelvin_value);
      }

      bufr->addValue(temp_sensor_level);
      bufr->addValue(temp_value);

      std::string dew_value = "MISSING";
      auto dew = std::find_if(
          t->second.begin(), t->second.end(),
          [](const std::pair<std::string, double> &t) -> bool {
            return t.first.substr(0, 21) == "dew_point_temperature";
          });
      if (dew != t->second.end()) {
        // Unit Conversion
        double kelvin_value = unit[w->first][temp->first] == "K"
                                  ? dew->second
                                  : dew->second + 273.16;
        dew_value = std::to_string(kelvin_value);
      }
      bufr->addValue(dew_value);

      std::string hum_value = "MISSING";
      auto hum =
          std::find_if(t->second.begin(), t->second.end(),
                       [](const std::pair<std::string, double> &t) -> bool {
                         return t.first.substr(0, 17) == "relative_humidity";
                       });
      if (hum != t->second.end()) {
        // test workaround
        hum_value = std::to_string(hum->second / 100);
        // hum_value = std::to_string(hum->second);
      }
      bufr->addValue(hum_value);

      // visibility
      bufr->addValue("MISSING"); // visibility sensor height
      bufr->addValue("MISSING"); // visibility

      // 24-H precipitation
      std::string prec24_value = "MISSING";
      std::string prec24_sensor_level = "MISSING";

      // precipitation_amount:0.0:sum:PT24H
      auto prec24 = std::find_if(
          t->second.begin(), t->second.end(),
          [](const std::pair<std::string, double> &t) -> bool {
            return (t.first.substr(0, 20) == "precipitation_amount" &&
                    t.first.substr(t.first.size() - 10, 10) == ":sum:PT24H");
          });
      if (prec24 != t->second.end()) {
        auto level_str_beg = prec24->first.find(':');
        if (level_str_beg != std::string::npos) {
          auto level_str_end = prec24->first.find(':', level_str_beg + 1);
          if (level_str_end != std::string::npos) {
            prec24_sensor_level = prec24->first.substr(
                level_str_beg + 1, level_str_end - level_str_beg - 1);
          }
        }
        prec24_value = std::to_string(prec24->second);
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

      auto wind_s =
          std::find_if(t->second.begin(), t->second.end(),
                       [](const std::pair<std::string, double> &t) -> bool {
                         return t.first.substr(0, 10) == "wind_speed";
                       });
      if (wind_s != t->second.end()) {
        auto level_str_beg = wind_s->first.find(':');
        if (level_str_beg != std::string::npos) {
          auto level_str_end = wind_s->first.find(':', level_str_beg + 1);
          if (level_str_end != std::string::npos) {
            wind_sensor_level = wind_s->first.substr(
                level_str_beg + 1, level_str_end - level_str_beg - 1);
          }
        }
        wind_speed_value = std::to_string(wind_s->second);
      }

      std::string wind_dir_value = "MISSING";

      auto wind_d =
          std::find_if(t->second.begin(), t->second.end(),
                       [](const std::pair<std::string, double> &t) -> bool {
                         return t.first.substr(0, 19) == "wind_from_direction";
                       });
      if (wind_d != t->second.end()) {
        /*        auto level_str_beg = wind->first.find(':');
                if (level_str_beg != std::string::npos) {
                  auto level_str_end = wind->first.find(':', level_str_beg + 1);
                  if (level_str_end != std::string::npos) {
                    wind_sensor_level = wind->first.substr(
                        level_str_beg + 1, level_str_end - level_str_beg - 1);
                  }
                }
                  */
        wind_dir_value = std::to_string(wind_d->second);
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

      // precipitation_amount:0.0:sum:PT1H
      auto prec1 = std::find_if(
          t->second.begin(), t->second.end(),
          [](const std::pair<std::string, double> &t) -> bool {
            return (t.first.substr(0, 20) == "precipitation_amount" &&
                    t.first.substr(t.first.size() - 9, 9) == ":sum:PT1H");
          });
      if (prec1 != t->second.end()) {
        prec1_value = std::to_string(prec1->second);
      }

      std::string prec12_value = "MISSING";

      // precipitation_amount:0.0:sum:PT12H
      auto prec12 = std::find_if(
          t->second.begin(), t->second.end(),
          [](const std::pair<std::string, double> &t) -> bool {
            return (t.first.substr(0, 20) == "precipitation_amount" &&
                    t.first.substr(t.first.size() - 10, 10) == ":sum:PT12H");
          });
      if (prec12 != t->second.end()) {
        prec12_value = std::to_string(prec12->second);
      }

      bufr->addValue(prec24_sensor_level);

      bufr->addValue(-1);
      bufr->addValue(prec1_value);

      bufr->addValue(-12);
      bufr->addValue(prec12_value);

      if (!subsets) {
        bufr->addDescriptor("101000");
        bufr->addDescriptor("031001");
      }

      bufr->addValue(2); // DELAYED DESCRIPTOR REPLICATION FACTOR

      if (!subsets) {
        bufr->addDescriptor("302045");
      }

      std::string ldrad12_value = "MISSING";

      // LONG-WAVE RADIATION PT12H
      auto ldrad12 = std::find_if(
          t->second.begin(), t->second.end(),
          [](const std::pair<std::string, double> &t) -> bool {
            return (t.first.substr(0, 61) ==
                        "integral_wrt_time_of_surface_downwelling_longwave_"
                        "flux_in_air" &&
                    t.first.substr(t.first.size() - 10, 10) == ":sum:PT12H");
          });
      if (ldrad12 != t->second.end()) {
        ldrad12_value = std::to_string(ldrad12->second);
      }

      std::string ldrad1_value = "MISSING";

      // LONG-WAVE RADIATION PT1H
      auto ldrad1 = std::find_if(
          t->second.begin(), t->second.end(),
          [](const std::pair<std::string, double> &t) -> bool {
            return (t.first.substr(0, 61) ==
                        "integral_wrt_time_of_surface_downwelling_longwave_"
                        "flux_in_air" &&
                    t.first.substr(t.first.size() - 9, 9) == ":sum:PT1H");
          });
      if (ldrad1 != t->second.end()) {
        ldrad1_value = std::to_string(ldrad1->second);
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

      bufr->addValue(-12);           // TIME PERIOD OR DISPLACEMENT
      bufr->addValue(ldrad12_value); // LONG-WAVE RADIATION INTEGRATED OVER 12H
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

  // print the encoding stream
  if (stream_print) {
    // if (1) {
    std::cout << "===========> STREAM: \n";
    std::cout << bufr->getEncStream() << "\n<===========\n";
  }

  bufr->encodeBufr();
  uint8_t *rbe = bufr->toBuffer();

  return pybind11::bytes(reinterpret_cast<char *>(rbe), bufr->length());
}

std::string norbufr_bufrprint(std::string fname) {

  std::stringstream ret;

  std::ifstream bufrFile(fname.c_str(),
                         std::ios_base::in | std::ios_base::binary);

  while (bufrFile.good()) {

    ESOHBufr *bufr = new ESOHBufr;

    if (bufrFile >> *bufr) {

      bufr->setTableB(
          &tb.at(bufr->getVersionMaster() &&
                         tb.find(bufr->getVersionMaster()) != tb.end()
                     ? bufr->getVersionMaster()
                     : tb.rbegin()->first));
      bufr->setTableC(
          &tc.at(bufr->getVersionMaster() &&
                         tc.find(bufr->getVersionMaster()) != tc.end()
                     ? bufr->getVersionMaster()
                     : tc.rbegin()->first));
      bufr->setTableD(
          &td.at(bufr->getVersionMaster() &&
                         td.find(bufr->getVersionMaster()) != td.end()
                     ? bufr->getVersionMaster()
                     : td.rbegin()->first));

      bufr->extractDescriptors();

      ret << *bufr;
    }
  }

  return ret.str();
}

bool norbufr_init_radar_cf(std::map<std::string, std::string> cf_py) {
  radar_cf_st = cf_py;
  return true;
}

std::list<std::string> norbufr_log() { return esoh_bufr_log; }

void norbufr_log_clear() { esoh_bufr_log.clear(); }
void norbufr_set_default_wigos(std::string s) { default_shadow_wigos_py = s; }

PYBIND11_MODULE(bufresohmsg_py, m) {
  m.doc() = "bufresoh E-SOH MQTT message generator plugin";

  m.def("init_bufrtables_py", &norbufr_init_bufrtables, "Init BUFR Tables");
  m.def("update_bufrtables_py", &norbufr_update_bufrtables, "Init BUFR Tables");

  m.def("bufresohmsg_py", &norbufr_bufresohmsg,
        "bufresoh MQTT message generator");
  m.def("bufresohmsgmem_py", &norbufr_bufresohmsgmem,
        "bufresoh MQTT message generator");
  m.def("bufrprint_py", &norbufr_bufrprint, "Print bufr message");
  m.def("covjson2bufr_py", &norbufr_covjson2bufr,
        "Generate BUFR from Coverage Json");
  m.def("bufrlog_py", &norbufr_log, "Get bufr log messages list");
  m.def("bufrlog_clear_py", &norbufr_log_clear, "Clear log messages list");

  m.def("init_oscar_py", &norbufr_init_oscar, "Init OSCAR db");
  m.def("init_bufr_schema_py", &norbufr_init_schema_template,
        "Init BUFR schema");
  m.def("bufr_sdwigos_py", &norbufr_set_default_wigos,
        "Set default shadow WIGOS Id");
  m.def("init_radar_cf_py", &norbufr_init_radar_cf,
        "Get Radar CF names from api/radar_cf.py");
}
