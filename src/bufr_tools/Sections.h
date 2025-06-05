/*
 * (C) Copyright 2023, met.no
 *
 * This file is part of the Norbufr BUFR en/decoder
 *
 * Author: istvans@met.no
 *
 */

#ifndef _SECTIONS_H
#define _SECTIONS_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#include <cstdint>
#include <fstream>
#include <list>
#include <vector>

#include "Descriptor.h"

class SectionBase {
public:
  SectionBase();
  bool fromBuffer(uint8_t *buffer, int size);
  void clear();
  ssize_t length() const;

protected:
  uint8_t *buffer;
  ssize_t len;
  uint8_t zero; // zero, edition num
};

class Section1 : public SectionBase {
public:
  Section1(uint8_t edition = 4);
  bool fromBuffer(uint8_t *buffer, int size, uint8_t edition = 4);
  size_t bufSize() const;
  bool toBuffer(uint8_t *buffer, uint8_t edition = 4) const;

  bool optSection() const;
  int getMasterTable() const;
  int getCentre() const;
  int getSubCentre() const;
  int getUpdateSeqNum() const;
  int getOptionalSelection() const;
  int getDataCategory() const;
  int getDataSubCategory() const;
  int getLocalDataSubCategory() const;
  int getVersionMaster() const;
  int getVersionLocal() const;
  void setYear(int y) { bufr_time.tm_year = y - 1900; };
  void setMonth(int m) { bufr_time.tm_mon = m - 1; };
  void setDay(int d) { bufr_time.tm_mday = d; };
  void setHour(int h) { bufr_time.tm_hour = h; };
  void setMinute(int m) { bufr_time.tm_min = m; };
  void setSecond(int s) { bufr_time.tm_sec = s; };
  void setMasterTable(uint8_t m) { master_table = m; };
  void setCentre(uint16_t c) { centre = c; };
  void setSubCentre(uint16_t s) { subcentre = s; };
  void setUpdateSeqNum(uint8_t u) { upd_seq_num = u; };
  void setOptionalSelection(uint8_t o) { optional_section = o; };
  void setDataCategory(uint8_t d) { data_category = d; };
  void setDataSubCategory(uint8_t i) { int_data_subcategory = i; };
  void setLocalDataSubCategory(uint8_t l) { local_data_subcategory = l; };
  void setVersionMaster(uint8_t v) { version_master = v; };
  void setVersionLocal(uint8_t v) { version_local = v; };

protected:
  void clear();

  uint8_t master_table;
  uint16_t centre;
  uint16_t subcentre;
  uint8_t upd_seq_num;
  uint8_t optional_section;
  uint8_t data_category;
  uint8_t int_data_subcategory;
  uint8_t local_data_subcategory;
  uint8_t version_master;
  uint8_t version_local;
  struct tm bufr_time;
  std::vector<uint8_t> local_data;

  friend std::ostream &operator<<(std::ostream &is, Section1 &sec);
};

class Section2 : public SectionBase {
public:
  Section2();
  bool fromBuffer(uint8_t *buffer, int size);
  size_t bufSize() const;
  bool toBuffer(uint8_t *buffer) const;

protected:
  void clear();
  std::vector<uint8_t> local_data;

  friend std::ostream &operator<<(std::ostream &os, Section2 &sec);
};

class Section3 : public SectionBase {
public:
  Section3();
  bool fromBuffer(uint8_t *buffer, int size);
  bool toBuffer(uint8_t *buffer) const;
  inline void addDescriptor(DescriptorId d) {
    len += 2;
    sec3_desc.push_back(d);
  }

  bool isObserved() const;
  bool isCompressed() const;
  uint16_t subsetNum() const;
  void setSubset(uint16_t s) { subsets = s; }
  void setObsComp(uint8_t o) { obs_comp = o; }
  void setObserved(bool o = true) {
    if (o)
      obs_comp |= 0x80;
    else
      obs_comp &= 0x7f;
  }
  void setComp(bool c = true) {
    if (c)
      obs_comp |= 0x40;
    else
      obs_comp &= 0xbf;
  }

protected:
  void clear();
  uint16_t subsets;
  uint8_t obs_comp;

  std::list<DescriptorId> sec3_desc;

  friend std::ostream &operator<<(std::ostream &os, Section3 &sec);
};

class Section4 : public SectionBase {
public:
  Section4();
  bool fromBuffer(uint8_t *buffer, int size);
  uint64_t bitSize() const;
  bool toBuffer(uint8_t *buffer) const;
  void setValue(uint64_t value, unsigned int datawidth);
  void setMissingValue(unsigned int datawidth);
  uint64_t getValue(unsigned long startbit, int datawidth,
                    bool missingbits = true) const;

protected:
  void clear();

  std::vector<bool> bits;

  friend std::ostream &operator<<(std::ostream &os, Section4 &sec);
};

#endif
