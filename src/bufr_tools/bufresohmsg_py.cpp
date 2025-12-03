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
#include "covjson2bufr.h"

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
        // std::cerr << "WMO tableB: " << entry.path().string() << "\n";
      } else {
        // WMO table C file
        if (entry.path().filename() == "BUFRCREX_CodeFlag_en.txt") {
          TableC tc_e(entry.path().string());
          // std::cerr << "Load WMO C table" << entry.path().string() << "\n";
          if (tc.size()) {
            tc[0] += tc_e;
          } else
            tc[0] = tc_e;
        } else {
          // OPERA table B files
          if (entry.path().filename().string().substr(0, 9) == "localtabb" ||
              entry.path().filename().string().substr(0, 8) == "bufrtabb") {
            TableB tb_e(entry.path().string());
            struct vc_struct vc = get_versioncentre_from_table_filename(
                entry.path().filename().string());
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
        // std::cerr << "WMO tableD: " << entry.path().string() << "\n";
      } else {
        // OPERA table D files
        if (entry.path().filename().string().substr(0, 9) == "localtabd" ||
            entry.path().filename().string().substr(0, 8) == "bufrtabd") {
          TableD td_e(entry.path().string());
          struct vc_struct vc = get_versioncentre_from_table_filename(
              entry.path().filename().string());
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

pybind11::bytes norbufr_covjson2bufr(std::string covjson_str,
                                     std::string bufr_template) {

  pybind11::bytes ret;

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
  struct ret_bufr ret_b = covjson2bufr(covjson_str, bufr_template, bufr);

  return pybind11::bytes(ret_b.buffer, ret_b.size);
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
