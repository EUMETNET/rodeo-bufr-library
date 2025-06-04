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
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

#include "Descriptor.h"

#include "Tables.h"

#include "NorBufr.h"

int main(int argc, char *argv[]) {

  bool stream_print = false;
  if (argc > 1) {
    if (!strcmp(argv[1], "stream_print")) {
      stream_print = true;
    }
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

  // Set Subset
  bufr->setSubset(2);

  /* =========================== SUBSET 1 =========================== */

  // Add first descriptor and value: STATE IDENTIFIER = 643
  bufr->addDescriptor("001101", 643);
  // Equivalent formats
  // Format 1:
  // bufr->addDescriptor("001101", "643");
  // Format 2:
  // bufr->addDescriptor(001101, 643);
  // Format 3 (separately):
  // bufr->addDescriptor("001101");
  // bufr->addValue(643);

  bufr->addDescriptor("001102"); // NATIONAL STATION NUMBER
  bufr->addValue(11007827);

  bufr->addDescriptor("001019", "SANDA D"); // LONG STATION OR SITE NAME

  bufr->addDescriptor("002001"); // TYPE OF STATION
  bufr->addValue(1);

  bufr->addDescriptor("301011"); // (Year, month, day)
  bufr->addValue(2025);
  bufr->addValue(3);
  bufr->addValue(28);

  bufr->addDescriptor("301012"); // (Hour, minute)
  bufr->addValue(6);
  bufr->addValue(0);

  bufr->addDescriptor("005001", 57.4254); // LATITUDE (HIGH ACCURACY)

  bufr->addDescriptor("006001"); // LONGITUDE (HIGH ACCURACY
  bufr->addValue(18.2111);

  bufr->addDescriptor(
      "007030"); // HEIGHT OF STATION GROUND ABOVE MEAN SEA LEVEL
  bufr->addValue(13.8);

  /****** REPEAT 102003 ******/
  // Temperature measurement at 2m, 5m, 10m
  bufr->addDescriptor("102003");

  bufr->addDescriptor("007032"); // HEIGHT OF SENSOR ABOVE LOCAL GROUND (OR DECK
                                 // OF MARINE PLATFORM)
  bufr->addValue(2);

  bufr->addDescriptor("012101"); // TEMPERATURE/DRY-BULB TEMPERATURE
  bufr->addValue(274.4);

  bufr->addValue(5);
  bufr->addValue(272.1);

  bufr->addValue(10);
  bufr->addValue(270.6);

  /****** REPEAT 102003 END ******/

  bufr->addDescriptor("007032"); // HEIGHT OF SENSOR ABOVE LOCAL GROUND (OR DECK
                                 // OF MARINE PLATFORM)
  bufr->addValue("MISSING");

  bufr->addDescriptor("002177"); // METHOD OF SNOW DEPTH MEASUREMENT
  bufr->addValue(0);

  bufr->addDescriptor("020062"); // STATE OF THE GROUND (WITH OR WITHOUT SNOW)
  bufr->addValue(0);

  bufr->addDescriptor("013013"); // TOTAL SNOW DEPTH
  bufr->addValue("MISSING");

  /****** REPEAT 103000 ******/

  // Wind measurement at 10m, 20m, 50m, second subset: 10m, 50m

  bufr->addDescriptor("103000");
  bufr->addDescriptor("031001");
  bufr->addValue(2);             // DELAYED DESCRIPTOR REPLICATION FACTOR
  bufr->addDescriptor("007032"); // HEIGHT OF SENSOR ABOVE LOCAL GROUND (OR DECK
                                 // OF MARINE PLATFORM)
  bufr->addValue(10);
  bufr->addDescriptor("011001"); // WIND DIRECTION
  bufr->addValue(176);
  bufr->addDescriptor("011002"); // WIND SPEED
  bufr->addValue(3.4);

  bufr->addValue(
      20); // HEIGHT OF SENSOR ABOVE LOCAL GROUND (OR DECK OF MARINE PLATFORM)
  bufr->addValue(182); // WIND DIRECTION
  bufr->addValue(6.2); // WIND SPEED
                       /*
                         bufr->addValue(
                             50); // HEIGHT OF SENSOR ABOVE LOCAL GROUND (OR DECK OF MARINE PLATFORM)
                         bufr->addValue(212); // WIND DIRECTION
                         bufr->addValue(8.3); // WIND SPEED
                       */
  /****** REPEAT 103000 END ******/

  /* ======================= END of SUBSET 1 ======================== */

  bufr->addDescriptor(0); // Subset end indicator
  // END of first subset, no more addDescriptor()

  /* =========================== SUBSET 2 =========================== */

  bufr->addValue(643); // STATE IDENTIFIER

  bufr->addValue(11007829); // NATIONAL STATION NUMBER

  bufr->addValue("SANDA B"); // LONG STATION OR SITE NAME
  bufr->addValue(1);         // TYPE OF STATION

  bufr->addValue(2025); // (Year, month, day)
  bufr->addValue(3);
  bufr->addValue(28);

  bufr->addValue(6); // (Hour, minute)
  bufr->addValue(0);

  bufr->addValue(67.4254); // LATITUDE (HIGH ACCURACY)
  bufr->addValue(28.2111); // LONGITUDE (HIGH ACCURACY
  bufr->addValue(13.8);    // HEIGHT OF STATION GROUND ABOVE MEAN SEA LEVEL

  /****** REPEAT 102003 ******/

  // Temperature measurement at 2m, 5m, 10m
  bufr->addValue(2);
  bufr->addValue(284.4);

  bufr->addValue(5);
  bufr->addValue(283.9);

  bufr->addValue(10);
  bufr->addValue(281.2);

  /****** REPEAT 102003 END ******/

  bufr->addValue("MISSING"); // HEIGHT OF SENSOR ABOVE LOCAL GROUND (OR DECK OF
                             // MARINE PLATFORM)

  bufr->addValue(0); // METHOD OF SNOW DEPTH MEASUREMENT

  bufr->addValue(1); // STATE OF THE GROUND (WITH OR WITHOUT SNOW

  bufr->addValue(2); // TOTAL SNOW DEPTH

  /****** REPEAT 103000 ******/

  // Wind measurement at 10m, 50m, first subset: 10m, 20m, 50m
  bufr->addValue(2); //  DELAYED DESCRIPTOR REPLICATION FACTOR

  bufr->addValue(10);
  bufr->addValue(284);
  bufr->addValue(2.3);

  bufr->addValue(50);
  bufr->addValue(312);
  bufr->addValue(15.3);

  /****** REPEAT 103000 END ******/

  /* ======================= END of SUBSET 2 ======================== */

  bufr->encodeBufr();
  const uint8_t *rbe = bufr->toBuffer();

  // print the encoding stream
  if (stream_print) {
    std::cout << "===========> STREAM: \n";
    std::cout << bufr->getEncStream() << "\n<===========\n";
  }

  auto bsize = bufr->length();
  std::ofstream os_test("test_encoded_out.bufr");
  for (size_t p = 0; p < bsize; ++p) {
    os_test.put(rbe[p]);
  }

  return 0;
}
