
#include <algorithm>
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

int main(int argc, char *argv[]) {

  // bool stream_print = false;
  std::string bufr_file_name("test_encoded_out.bufr");
  std::string covjson_str;
  if (argc > 1) {
    std::ifstream covjson_file(argv[1],
                               std::ios_base::in | std::ios_base::binary);
    std::stringstream ss;
    ss << covjson_file.rdbuf();
    covjson_str = ss.str();
    if (argc > 2)
      bufr_file_name = std::string(argv[2]);
  } else {
    std::cerr << "E-SOH covjson to bufr converter\n";
    std::cerr << "Usage: covjson2bufr input.json [output.bufr]\n";
  }

  rapidjson::Document covjson;

  if (covjson.Parse(covjson_str.c_str()).HasParseError()) {
    std::cerr << "E-SOH covjson message parsing Error!!!\n";
    return 20;
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
    return 10;
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
    return 10;
  }
  td = new TableD(Dtable_file);
  bufr->setTableD(td);

  struct ret_bufr ret = covjson2bufr(covjson_str, "default", bufr);

  std::ofstream os_test(bufr_file_name.c_str());
  for (size_t p = 0; p < ret.size; ++p) {
    os_test.put(ret.buffer[p]);
  }

  return 0;
}
