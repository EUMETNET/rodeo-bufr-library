/*
 * (C) Copyright 2023, met.no
 *
 * This file is part of the Norbufr BUFR en/decoder
 *
 * Author: istvans@met.no
 *
 */

#include <string.h>

#include <bitset>
#include <climits>
#include <iomanip>
#include <iostream>

#include "NorBufrIO.h"
#include "Sections.h"

/***************************************** SECTION BASE
 * *******************************************/

SectionBase::SectionBase() {
  buffer = 0;
  len = 0;
  zero = 0;
}

bool SectionBase::fromBuffer(uint8_t *buf, int size) {
  clear();
  if (size < 4)
    return false;
  buffer = buf;
  len = NorBufrIO::getBytes(buffer, 3);
  if (len > size)
    return false;
  zero = buffer[3];

  return true;
}

ssize_t SectionBase::length() const { return len; }

void SectionBase::clear() {
  len = 0;
  zero = 0;
}

/***************************************** SECTION 0
 * **********************************************/

/***************************************** SECTION 1
 * **********************************************/

Section1::Section1(uint8_t edition) {
  clear();
  switch (edition) {
  case 2:
    len = 18;
    break;
  case 3:
    len = 18;
    break;
  case 4:
    len = 22;
  }
}

bool Section1::optSection() const { return optional_section; }

int Section1::getMasterTable() const { return master_table; }

int Section1::getCentre() const { return centre; }

int Section1::getSubCentre() const { return subcentre; }

int Section1::getUpdateSeqNum() const { return upd_seq_num; }

int Section1::getOptionalSelection() const { return optional_section; }

int Section1::getDataCategory() const { return data_category; }

int Section1::getDataSubCategory() const { return int_data_subcategory; }

int Section1::getLocalDataSubCategory() const { return local_data_subcategory; }

int Section1::getVersionMaster() const { return version_master; }

int Section1::getVersionLocal() const { return version_local; }

size_t Section1::bufSize() const { return local_data.size(); }

void Section1::clear() {
  master_table = 0;
  centre = 0;
  subcentre = 0;
  upd_seq_num = 0;
  optional_section = 0;
  data_category = 0;
  int_data_subcategory = 0;
  local_data_subcategory = 0;
  version_master = 0;
  version_local = 0;
  memset(&bufr_time, 0, sizeof(struct tm));
  local_data.clear();
  SectionBase::clear();
}

bool Section1::fromBuffer(uint8_t *buf, int size, uint8_t edition) {

  clear();
  // Edition position shift
  int eshift = 0;

  if (!SectionBase::fromBuffer(buf, size))
    return false;

  if (edition >= 4) {
    if (size < 23)
      return false;
  } else {
    if (size < 18)
      return false;
  }

  master_table = buffer[3];

  switch (edition) {
  case 2:
    centre = NorBufrIO::getBytes(buffer + 4, 2);
    subcentre = 0;
    eshift = -2;
    break;
  case 3:
    subcentre = NorBufrIO::getBytes(buffer + 4, 1);
    centre = NorBufrIO::getBytes(buffer + 5, 1);
    eshift = -2;
    break;
  case 4:
    centre = NorBufrIO::getBytes(buffer + 4, 2);
    subcentre = NorBufrIO::getBytes(buffer + 6, 2);
    break;
    // default:
  }

  upd_seq_num = buffer[8 + eshift];
  optional_section = buffer[9 + eshift];
  data_category = buffer[10 + eshift];

  if (edition >= 4)
    int_data_subcategory = buffer[11 + eshift];
  else
    eshift--;

  local_data_subcategory = buffer[12 + eshift];

  version_master = buffer[13 + eshift];
  version_local = buffer[14 + eshift];

  uint16_t year;
  if (edition >= 4) {
    year = NorBufrIO::getBytes(buffer + 15 + eshift, 2);
  } else {
    year = buffer[15 + eshift];
    eshift--;
  }
  if (year < 80)
    year += 100;
  if (year > 1900)
    year -= 1900;

  memset(&bufr_time, 0, sizeof(struct tm));
  bufr_time.tm_year = year;
  bufr_time.tm_mon = buffer[17 + eshift] - 1;
  bufr_time.tm_mday = buffer[18 + eshift];
  bufr_time.tm_hour = buffer[19 + eshift];
  bufr_time.tm_min = buffer[20 + eshift];
  if (edition >= 4)
    bufr_time.tm_sec = buffer[21 + eshift];
  else
    eshift--;

  for (int i = (22 + eshift); i < len; i++) {
    local_data.push_back(buffer[i]);
  }

  return true;
}

bool Section1::toBuffer(uint8_t *buffer, uint8_t edition) const {
  NorBufrIO::setBytes(buffer, len, 3);
  buffer[4] = master_table;
  int eshift = 0;
  switch (edition) {
  case 2:
    NorBufrIO::setBytes(buffer + 4, centre, 2);
    // subcentre = 0;
    eshift = -2;
    break;
  case 3:
    NorBufrIO::setBytes(buffer + 4, subcentre, 1);
    NorBufrIO::setBytes(buffer + 5, centre, 1);
    eshift = -2;
    break;
  case 4:
    NorBufrIO::setBytes(buffer + 4, centre, 2);
    NorBufrIO::setBytes(buffer + 6, subcentre, 2);
    break;
    // default:
  }

  buffer[8 + eshift] = upd_seq_num;
  buffer[9 + eshift] = optional_section;
  buffer[10 + eshift] = data_category;

  if (edition >= 4)
    buffer[11 + eshift] = int_data_subcategory;
  else
    eshift--;

  buffer[12 + eshift] = local_data_subcategory;
  buffer[13 + eshift] = version_master;
  buffer[14 + eshift] = version_local;

  int year = bufr_time.tm_year + 1900;
  if (edition >= 4) {
    NorBufrIO::setBytes(buffer + 15 + eshift, year, 2);
  } else {
    year -= 2000;
    year = buffer[15 + eshift];
    eshift--;
  }

  buffer[17 + eshift] = bufr_time.tm_mon + 1;
  buffer[18 + eshift] = bufr_time.tm_mday;
  buffer[19 + eshift] = bufr_time.tm_hour;
  buffer[20 + eshift] = bufr_time.tm_min;

  if (edition >= 4)
    buffer[21 + eshift] = bufr_time.tm_sec;
  else
    eshift--;

  for (int i = (22 + eshift), j = 0; i < len; i++, j++) {
    buffer[i] = local_data[j];
  }

  return true;
}

std::ostream &operator<<(std::ostream &os, Section1 &sec) {
  os << "=============== Section 1  ===============\n";
  os << "length: " << sec.len << "\n";
  os << "Master Table: " << static_cast<int>(sec.master_table) << "\n";
  os << "Centre: " << sec.centre << "\n";
  os << "Subcentre: " << sec.subcentre << "\n";
  os << "Update Sequence Number: " << static_cast<int>(sec.upd_seq_num) << "\n";
  os << "Optional Section: " << static_cast<int>(sec.optional_section) << "\n";
  os << "Data Category: " << static_cast<int>(sec.data_category) << "\n";
  os << "International Data Subcategory: "
     << static_cast<int>(sec.int_data_subcategory) << "\n";
  os << "Local Data Subcategory: "
     << static_cast<int>(sec.local_data_subcategory) << "\n";
  os << "VersionMaster: " << static_cast<int>(sec.version_master) << "\n";
  os << "VersionLocal: " << static_cast<int>(sec.version_local) << "\n";
  os << "Time: " << asctime(&sec.bufr_time) << "\n";
  os << "Local data[" << sec.local_data.size() << "]:\n";
  for (unsigned int i = 0; i < sec.local_data.size(); ++i)
    os << static_cast<int>(sec.local_data[i]) << " ";
  os << "\n";

  return os;
}

/***************************************** SECTION 2
 * **********************************************/

Section2::Section2() {}

void Section2::clear() {
  SectionBase::clear();
  local_data.clear();
}

bool Section2::fromBuffer(uint8_t *buf, int size) {
  clear();
  if (!SectionBase::fromBuffer(buf, size))
    return false;
  for (int i = 4; i < len; i++) {
    local_data.push_back(buffer[i]);
  }

  return false;
}

bool Section2::toBuffer(uint8_t *buffer) const {

  NorBufrIO::setBytes(buffer, len, 3);
  buffer[3] = 0;
  for (size_t i = 0; i < local_data.size(); ++i) {
    buffer[i + 4] = local_data[i];
  }
  return true;
}
size_t Section2::bufSize() const { return local_data.size(); }

std::ostream &operator<<(std::ostream &os, Section2 &sec) {
  os << "=============== Section 2  ===============\n";
  os << "length: " << sec.len << " zero: " << static_cast<int>(sec.zero)
     << "\n";

  os << "Local data[" << sec.local_data.size() << "]:\n";

  for (unsigned int i = 0; i < sec.local_data.size(); ++i) {
    os << std::dec << static_cast<unsigned int>(sec.local_data[i]) << " ";
  }
  os << "\n";
  for (unsigned int i = 0; i < sec.local_data.size(); ++i) {
    os << std::hex << "0x" << std::setw(2) << std::setfill('0')
       << static_cast<unsigned int>(sec.local_data[i]) << " ";
  }
  os << std::dec << "\n";

  return os;
}

/***************************************** SECTION 3
 * **********************************************/

Section3::Section3() {
  len = 7;
  subsets = 1;
  obs_comp = 0;
}

void Section3::clear() {
  SectionBase::clear();
  subsets = 0;
  obs_comp = 0;
  sec3_desc.clear();
}

bool Section3::fromBuffer(uint8_t *buf, int size) {
  clear();

  if (!SectionBase::fromBuffer(buf, size))
    return false;
  subsets = NorBufrIO::getBytes(buffer + 4, 2);

  obs_comp = buffer[6];

  for (int i = 7; i < len - 1; i += 2) {
    sec3_desc.push_back(DescriptorId(buffer[i], buffer[i + 1]));
  }

  return true;
}
bool Section3::toBuffer(uint8_t *buffer) const {
  NorBufrIO::setBytes(buffer, len, 3);
  buffer[3] = 0;
  NorBufrIO::setBytes(buffer + 4, subsets, 2);
  NorBufrIO::setBytes(buffer + 6, obs_comp, 1);

  int shift = 7;
  for (auto d : sec3_desc) {
    NorBufrIO::setBytes(buffer + shift, d.fxy(), 2);
    shift += 2;
  }
  return true;
}

bool Section3::isObserved() const { return (obs_comp & 0x80); }

bool Section3::isCompressed() const { return (obs_comp & 0x40); }

uint16_t Section3::subsetNum() const { return subsets; }

std::ostream &operator<<(std::ostream &os, Section3 &sec) {
  os << "=============== Section 3  ===============\n";
  os << "length: " << sec.len << " zero: " << static_cast<int>(sec.zero)
     << "\n";
  os << "Subsets: " << sec.subsets << "\n";
  os << "Observed: " << sec.isObserved() << "\n";
  os << "Compressed:" << sec.isCompressed() << " " << "\n";

  os << "Descriptors:\n";

  for (auto v : sec.sec3_desc) {
    os << v << " ";
  }

  os << "\n";

  return os;
}

/***************************************** SECTION 4
 * **********************************************/

Section4::Section4() { len = 4; }

bool Section4::fromBuffer(uint8_t *buf, int size) {
  clear();

  if (!SectionBase::fromBuffer(buf, size) || len <= 4)
    return false;
  bits.resize((len - 4) * 8);
  for (int i = 4; i < len; i++) {
    unsigned char *uc = reinterpret_cast<unsigned char *>(buffer + i);
    std::bitset<8> bs(*uc);
    for (int j = 0; j < 8; ++j) {
      bits[(i - 4) * 8 + 7 - j] = bs[j];
    }
  }

  return true;
}

bool Section4::toBuffer(uint8_t *buffer) const {
  NorBufrIO::setBytes(buffer, len, 3);
  buffer[3] = 0;

  for (unsigned int i = 0; i < bits.size(); i += 8) {
    std::bitset<8> bs;
    for (int j = 0; j < 8; ++j) {
      if (i + j < bits.size())
        bs.set(7 - j, bits[i + j]);
      else {
        bs.set(7 - j, 0);
      }
    }
    unsigned long u = bs.to_ulong();
    unsigned char c = u;
    buffer[4 + i / 8] = c;
  }
  return true;
}

uint64_t Section4::bitSize() const { return bits.size(); }

void Section4::clear() {
  SectionBase::clear();
  bits.clear();
}

void Section4::setValue(uint64_t value, unsigned int datawidth) {
  if (bits.capacity() < bits.size() + datawidth)
    bits.reserve(bits.capacity() * 2);
  const size_t s = sizeof(value) * 8;
  std::bitset<s> bs(value);
  for (int i = datawidth - 1; i >= 0; --i) {
    bits.push_back(bs[i]);
  }
  len = 4 + (bitSize() + 7) / 8;
}

void Section4::setMissingValue(unsigned int datawidth) {
  if (bits.capacity() < bits.size() + datawidth)
    bits.reserve(bits.capacity() * 2);
  for (int i = datawidth - 1; i >= 0; --i) {
    bits.push_back(true);
  }
  len = 4 + (bitSize() + 7) / 8;
}

uint64_t Section4::getValue(unsigned long startbit, int datawidth,
                            bool missingbits) const {
  unsigned long ret = 0;
  bool missing = true;
  for (int i = 0; i < datawidth; ++i) {
    ret *= 2;
    ret += bits[startbit + i];
    if (bits[startbit + i] == 0)
      missing = false;
  }
  if (missing && missingbits && datawidth > 1)
    return ULONG_MAX;
  return ret;
}

std::ostream &operator<<(std::ostream &os, Section4 &sec) {
  os << "=============== Section 4  ===============\n";
  os << "length: " << sec.len << " zero: " << static_cast<int>(sec.zero)
     << "\n";

  return os;
}

/***************************************** SECTION 5
 * **********************************************/
