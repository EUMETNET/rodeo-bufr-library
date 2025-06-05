#!/bin/bash

# Updating WMO BUFR table files

RELEASE_NUM=44
SRC_URL="https://github.com/wmo-im/BUFR4/archive/refs/tags/v${RELEASE_NUM}.zip"

DL_SRC="./src/bufr_tools/data/tables/wmo/WMO_BUFR_tables_v${RELEASE_NUM}.zip"

if [ ! -f $DL_SRC ]; then
  curl $SRC_URL >${DL_SRC}
fi
unzip -j -o ${DL_SRC} "BUFR4-${RELEASE_NUM}/txt/BUFR*.txt" -d ./src/bufr_tools/data/tables/wmo/
