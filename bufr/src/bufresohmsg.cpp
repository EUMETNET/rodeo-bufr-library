/*
 * (C) Copyright 2023, met.no
 *
 * This file is part of the Norbufr BUFR en/decoder
 *
 * Author: istvans@met.no
 *
 */

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

#include "Descriptor.h"

#include "Tables.h"

#include "ESOHBufr.h"

int main(int argc, char *argv[]) {

  if (argc < 2) {
    std::cout << "Usage: bufresohmsg [log_print] bufr_file(s)\n";
    return 0;
  }
  bool log_print = false;
  // Load All BUFR tables

  const int v_mul = 10000;
  std::map<int, TableB *> tb;
  std::map<int, TableC *> tc;
  std::map<int, TableD *> td;
  // For local tables
  std::map<int, TableB *> tbl;
  std::map<int, TableC *> tcl;
  std::map<int, TableD *> tdl;
  TableB ltb;
  TableC ltc;
  TableD ltd;

  // Init OSCAR DB
  std::string oscar_dir("../oscar/oscar_stations_all.json");
  if (const char *oscar_dump = std::getenv("OSCAR_DUMP")) {
    oscar_dir = std::string(oscar_dump);
  }
  Oscar oscar(oscar_dir.c_str());

  // Init Schema
  std::string esoh_schema;
  if (const char *msg_schema = std::getenv("ESOH_SCHEMA")) {
    std::ifstream schema(msg_schema, std::ios_base::in | std::ios_base::binary);
    std::stringstream ss;
    ss << schema.rdbuf();
    esoh_schema = ss.str();
  }

  // Default tables: eccodes
  std::string Btable_dir("/usr/share/eccodes/definitions/bufr/tables/0/wmo");
  std::string Ctable_dir("/usr/share/eccodes/definitions/bufr/tables/0/wmo");
  std::string Dtable_dir("/usr/share/eccodes/definitions/bufr/tables/0/wmo");

  if (const char *table_dir = std::getenv("BUFR_TABLE_DIR")) {
    Btable_dir = std::string(table_dir);
    Ctable_dir = Dtable_dir = Btable_dir;
  }
  if (const char *table_dir = std::getenv("BUFR_BTABLE_DIR")) {
    Btable_dir = std::string(table_dir);
  }
  if (const char *table_dir = std::getenv("BUFR_CTABLE_DIR")) {
    Ctable_dir = std::string(table_dir);
  }
  if (const char *table_dir = std::getenv("BUFR_DTABLE_DIR")) {
    Dtable_dir = std::string(table_dir);
  }

  for (const auto &entry : std::filesystem::directory_iterator(Btable_dir)) {
    int vers = 0;
    int centre = 0;
    // std::cerr << "File: " << entry.path().filename() << "\n";
    //  Eccodes tables dir
    if (std::filesystem::is_directory(entry.path())) {
      vers = stoi(entry.path().filename().string());
      TableB *tb_e = new TableB(entry.path().string() + "/element.table");
      tb[vers * v_mul + centre] = tb_e;
      TableC *tc_e = new TableC(entry.path().string() + "/codetables");
      tc[vers * v_mul + centre] = tc_e;
    } else {
      // WMO table B
      if (entry.path().filename() == "BUFRCREX_TableB_en.txt") {
        TableB *tb_e = new TableB(entry.path().string());
        tb[0] = tb_e;
      } else {
        // WMO Code Table
        if (entry.path().filename() == "BUFRCREX_CodeFlag_en.txt") {
          TableC *tc_e = new TableC(entry.path().string());
          // std::cerr << "Load WMO C table\n";
          tc[0] = tc_e;
        } else {
          if (entry.path().filename().string().substr(0, 9) == "localtabb" ||
              entry.path().filename().string().substr(0, 8) == "bufrtabb") {
            auto vers_s =
                entry.path().filename().string().find(std::string("_"));
            auto vers_e = entry.path().filename().string().find(
                std::string("_"), vers_s + 1);
            if (vers_e == std::string::npos) {
              vers_e = entry.path().filename().string().find(std::string("."),
                                                             vers_s + 1);
              vers = std::stoi(entry.path().filename().string().substr(
                  vers_s + 1, vers_e - vers_s - 1));
            } else {
              auto centre_e = entry.path().filename().string().find(
                  std::string("."), vers_e);
              //    std::cerr << "CENTRE=" << centre_e << " ";
              centre = std::stoi(entry.path().filename().string().substr(
                  vers_s + 1, vers_e - vers_s - 1));
              vers = std::stoi(entry.path().filename().string().substr(
                  vers_e + 1, centre_e - vers_e - 1));
            }
            // std::cerr << "VERS S: " << vers_s << "=>" << vers_e << " ";
            TableB *tb_e = new TableB(entry.path().string());
            if (entry.path().filename().string().substr(0, 9) != "localtabb") {
              // TableB *tb_e = new TableB();
              tb[vers * v_mul + centre] = tb_e;
            } else {
              // std::cerr << "Localtabb store\n";
              tbl[vers * v_mul + centre] = tb_e;
            }
          }
        }
      }
    }
  }
  if (Ctable_dir.size()) {
    std::string wmo_c_path = Ctable_dir + "/BUFRCREX_CodeFlag_en.txt";
    if (std::filesystem::is_regular_file(wmo_c_path) ||
        std::filesystem::is_symlink(wmo_c_path)) {
      TableC *tc_e = new TableC(wmo_c_path);
      // std::cerr << "Load WMO C table\n";
      tc[0] = tc_e;
    }

    // Meteo-France OPERA Code table
    std::string mfr_c_path = Ctable_dir + "/btc085.019";
    if (std::filesystem::is_regular_file(mfr_c_path) ||
        std::filesystem::is_symlink(mfr_c_path)) {
      TableC *tc_e = new TableC(mfr_c_path);
      // std::cerr << "Load Meteo France Ccode table\n";
      if (tc.size()) {
        ltc = *tc[0];
        ltc += *tc_e;
        tc[0] = &ltc;
      } else
        tc[0] = tc_e;
    }
  }

  for (const auto &entry : std::filesystem::directory_iterator(Dtable_dir)) {
    int vers = 0;
    int centre = 0;

    // Eccodes tables dir
    if (std::filesystem::is_directory(entry.path())) {
      vers = stoi(entry.path().filename().string());
      TableD *tb_d = new TableD(entry.path().string() + "/sequence.def");
      td[vers * v_mul] = tb_d;
    } else {
      // WMO table D
      if (entry.path().filename() == "BUFR_TableD_en.txt") {
        TableD *td_e = new TableD(entry.path().string());
        td[0] = td_e;
      } else {
        if (entry.path().filename().string().substr(0, 9) == "localtabd" ||
            entry.path().filename().string().substr(0, 8) == "bufrtabd") {
          auto vers_s = entry.path().filename().string().find(std::string("_"));
          auto vers_e = entry.path().filename().string().find(std::string("_"),
                                                              vers_s + 1);
          if (vers_e == std::string::npos) {
            vers_e = entry.path().filename().string().find(std::string("."),
                                                           vers_s + 1);
            vers = std::stoi(entry.path().filename().string().substr(
                vers_s + 1, vers_e - vers_s - 1));
          } else {
            auto centre_e =
                entry.path().filename().string().find(std::string("."), vers_e);
            // std::cerr << "CENTRE=" << centre_e << " ";
            centre = std::stoi(entry.path().filename().string().substr(
                vers_s + 1, vers_e - vers_s - 1));
            vers = std::stoi(entry.path().filename().string().substr(
                vers_e + 1, centre_e - vers_e - 1));
          }
          // std::cerr << "VERS S: " << vers_s << "=>" << vers_e << " ";

          TableD *tb_d = new TableD(entry.path().string());
          if (entry.path().filename().string().substr(0, 9) != "localtabd") {
            td[vers * v_mul + centre] = tb_d;
          } else {
            // std::cerr << "Localtab store\n";
            tdl[vers * v_mul + centre] = tb_d;
          }
        }
      }
    }
  }

  std::list<std::string> log;
  bool firstmsg = true;
  std::stringstream ss;
  ss << "[\n";
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "log_print") {
      log_print = true;
      continue;
    }
    std::ifstream bufrFile(argv[i], std::ios_base::in | std::ios_base::binary);

    std::string fname = std::filesystem::path(argv[i]).filename();

    while (bufrFile.good()) {

      ESOHBufr *bufr = new ESOHBufr;
      bufr->setBufrId(fname);
      bufr->setOscar(&oscar);

      if (bufrFile >> *bufr) {

        // std::cerr << "TableB index: ";
        int tb_index = -1;
        if (tb.size()) {
          tb_index = tb.rbegin()->first;
          if (tb.find(bufr->getVersionMaster() * v_mul) != tb.end())
            tb_index = bufr->getVersionMaster() * v_mul;
          if (tb_index > -1)
            bufr->setTableB(tb.at(tb_index));

          int tbl_index = -1;
          if (tbl.size()) {
            tbl_index = bufr->getVersionLocal() * v_mul + bufr->getCentre();
            if (tbl.find(tbl_index) == tbl.end()) {
              tbl_index = bufr->getVersionLocal() * v_mul;
              if (tbl.find(tbl_index) == tbl.end()) {
                std::cerr << "TB No Lolcal:" << tbl_index << "\n";
              }
            }
            if (tbl_index > -1) {

              // std::cerr << "Add local B table: " << tbl_index << "[" <<
              // tb_index << "]\n";
              ltb = *tb.at(tb_index);
              if (tbl.find(tbl_index) != tbl.end())
                ltb += *tbl.at(tbl_index);
              else
                std::cerr << "Local B table missing!!!\n";
              bufr->setTableB(&ltb);
              // std::cerr << "Local table OK\n";
            }
          }
        }
        // std::cerr << "TableD index: ";
        int td_index = -1;
        if (td.size()) {
          td_index = td.rbegin()->first;
          if (td.find(bufr->getVersionMaster() * v_mul) != td.end())
            td_index = bufr->getVersionMaster() * v_mul;
          if (td_index > -1)
            bufr->setTableD(td.at(td_index));

          int tdl_index = -1;
          if (tdl.size()) {
            tdl_index = bufr->getVersionLocal() * v_mul + bufr->getCentre();
            if (tdl.find(tdl_index) == tdl.end()) {
              tdl_index = bufr->getVersionLocal() * v_mul;
              if (tdl.find(tdl_index) == tdl.end()) {
                std::cerr << "TD No Lolcal:" << tdl_index << "\n";
              }
            }
            if (tdl_index > -1) {

              // std::cerr << "Add local D table: " << tdl_index << "[" <<
              // td_index << "]\n";
              ltd = *td.at(td_index);
              if (tdl.find(tdl_index) != tdl.end())
                ltd += *tdl.at(tdl_index);
              else
                std::cerr << "Local D table missing!!!\n";
              bufr->setTableD(&ltd);
            }
          }
        }

        if (tc.size())
          bufr->setTableC(tc.at(
              bufr->getVersionMaster() &&
                      tc.find(bufr->getVersionMaster() * v_mul) != tc.end()
                  ? bufr->getVersionMaster() * v_mul
                  : tc.rbegin()->first));

        if (!tb.size() || !td.size()) {
          std::cerr << "Missing tables\n";
          continue;
        }

        bufr->extractDescriptors();

        if (esoh_schema.size()) {
          bufr->setMsgTemplate(esoh_schema);
        }
        for (auto msg : bufr->msg()) {
          if (firstmsg && msg.size()) {
            firstmsg = false;
          } else {
            ss << ",\n";
          }
          ss << msg << "\n";
        }

        bufr->logToCsvList(log, ';', LogLevel::WARN);
      }
    }
  }
  ss << "]\n";

  rapidjson::Document msg;
  rapidjson::Document::AllocatorType &msg_allocator = msg.GetAllocator();
  msg.Parse(ss.str().c_str());
  rapidjson::StringBuffer sb;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
  writer.SetIndent(' ', 4);
  writer.SetMaxDecimalPlaces(6);
  msg.Accept(writer);
  std::cout << sb.GetString() << "\n";

  // Print log
  if (log_print) {
    if (log.size()) {
      for (auto l : log) {
        std::cout << l << "\n";
      }
    }
  }

  for (auto i : tb)
    if (i.second != &ltb)
      delete i.second;
  for (auto i : tc) {
    if (i.second != &ltc)
      delete i.second;
  }
  for (auto i : td) {
    if (i.second != &ltd)
      delete i.second;
  }

  return 0;
}
