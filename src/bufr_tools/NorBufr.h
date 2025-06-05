/*
 * (C) Copyright 2023, met.no
 *
 * This file is part of the Norbufr BUFR en/decoder
 *
 * Author: istvans@met.no
 *
 */

#ifndef _NORBUF_H_
#define _NORBUF_H_

#include <list>

#include "Descriptor.h"
#include "LogBuffer.h"
#include "Sections.h"
#include "Tables.h"

const LogLevel norbufr_default_loglevel = LogLevel::WARN;

class NorBufr : public Section1,
                public Section2,
                public Section3,
                public Section4 {

public:
  NorBufr();
  ~NorBufr();

  uint64_t fromBuffer(char *ext_buf, uint64_t ext_buf_pos,
                      uint64_t ext_buf_size);
  const uint8_t *toBuffer();
  void setTableDir(std::string s);
  ssize_t extractDescriptors(int ss = 0, ssize_t subsb = 0);
  bool saveBuffer(std::string) const;
  double getValue(const Descriptor &d, double v) const;
  uint64_t getBitValue(const Descriptor &d, uint64_t v) const;
  int getValue(const Descriptor &d, int v) const;
  std::string getValue(const Descriptor &d, std::string s,
                       bool with_unit = true) const;
  void setTableB(TableB *tb) { tabB = tb; }
  void setTableC(TableC *tc) { tabC = tc; }
  void setTableD(TableD *td) { tabD = td; }
  uint64_t length() const;
  void print(const DescriptorId df, const std::string filter,
             const DescriptorId dv) const;
  void printValue(DescriptorId df) const;
  std::ostream &printDetail(std::ostream &os = std::cout);

  // TODO: add local/external tables

  void freeBuffer();

  void logToCsvList(std::list<std::string> &list, char delimiter = ';',
                    LogLevel l = LogLevel::UNKNOWN) const;
  void logToJsonList(std::list<std::string> &list,
                     LogLevel l = LogLevel::UNKNOWN) const;

  void setBufrId(std::string);

  // Encoding
  inline void addDescriptor(DescriptorId d) {
    encbufr_stream << d.toString(false) << "\n";
  };
  template <typename T> void addDescriptor(DescriptorId d, T value) {
    encbufr_stream << d.toString(false) << " " << value << "\n";
  };
  template <typename T> void addValue(T value) {
    encbufr_stream << value << "\n";
  };
  inline std::string getEncStream() const { return encbufr_stream.str(); };
  inline bool encodeBufr() { return fromText(encbufr_stream); };
  uint64_t encodeSubsets(std::istream &is);
  bool encodeDescriptor(DescriptorId d, std::istream &is, int level = 0,
                        DescriptorId *parent = nullptr, int index = 0);
  bool fromText(std::istream &is);
  bool fromCovJson(std::string s);

private:
  bool setSections(int slen);
  void clearTable();
  void clear();
  long checkBuffer();
  std::vector<DescriptorMeta *>::iterator findMeta(DescriptorMeta *dm);
  DescriptorMeta *addMeta(DescriptorMeta *dm);
  uint64_t uncompressDescriptor(std::list<DescriptorId>::iterator &it,
                                ssize_t &sb, ssize_t &subsetsb,
                                uint32_t *repeatnum = 0);

protected:
  ssize_t len;
  uint8_t edition;

  std::string table_dir;
  TableA *tabA;
  TableB *tabB;
  TableC *tabC;
  TableD *tabD;

  uint8_t *buffer;

  std::string bufr_id;

  std::vector<std::list<Descriptor>> desc;
  std::vector<DescriptorMeta *> extraMeta;

  // Subset start bit
  std::vector<ssize_t> subsets;

  // Section4 uncompressed bits
  std::vector<bool> ucbits;

  // Encode string stream
  std::stringstream encbufr_stream;
  ssize_t enc_mod_datawidth = 0;
  ssize_t enc_mod_str_datawidth = 0;
  ssize_t enc_local_datawidth = 0;
  int enc_mod_scale = 0;
  int enc_mod_refvalue_mul = 0;
  std::list<std::pair<DescriptorId, std::vector<int>>> encode_descriptors;
  bool auto_date = true;

  mutable LogBuffer lb;

  friend std::istream &operator>>(std::istream &is, NorBufr &bufr);
  friend std::ostream &operator<<(std::ostream &is, NorBufr &bufr);
};

#endif
