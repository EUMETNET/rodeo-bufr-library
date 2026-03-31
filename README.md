# E-SOH

## EURODEO

The RODEO project develops a user interface and Application Programming Interfaces (API) for accessing meteorological datasets declared as High Value Datasets (HVD) by the EU Implementing Regulation (EU) 2023/138 under the EU Open Data Directive (EU) 2019/1024. The project also fosters the engagement between data providers and data users for enhancing the understanding of technical solutions being available for sharing and accessing the HVD datasets.
This project provides a sustainable and standardized system for sharing real-time surface weather observations in line with the HVD regulation and WMO WIS 2.0 strategy. The real-time surface weather observations are made available through open web services, so that they can be accessed by anyone.

## Near real-time observational data

E-SOH is part of the RODEO project. The goal for this project is to make near real-time weather observations from land based station easily available. The data will be published on both a message queue using [MQTT](https://mqtt.org/) and [EDR](https://ogcapi.ogc.org/edr/) compliant APIs. Metadata will also be made available through [OGC Records](https://ogcapi.ogc.org/records/) APIs. The system architecture is portable, scalable and modular for taking into account possible future extensions to existing networks and datasets (e.g. 3rd party surface observations).

## RODEO BUFR Tools

This tool handles the BUFR messages:
* ecoding the messages for E-SOH/openradardata ingestion
* providing E-SOH API output in BUFR format

The library suports [ECMWF ecCodes](https://confluence.ecmwf.int/display/ECC), [WMO](https://github.com/wmo-im/BUFR4/) and [OPERA](https://www.eumetnet.eu/observations/weather-radar-network/) BUFR tables.

## Installation

Create virtual environment
```shell
python3 -m venv bufr-venv
```
Activate
```shell
source bufr-venv/bin/activate
```
Install rodeo-bufr-tools
```shell
pip install rodeo-bufr-tools
```
Install BUFR table files:
* WMO(for example version 44)
    ```shell
    wget https://github.com/wmo-im/BUFR4/archive/refs/tags/v44.zip
    unzip v44.zip
    export BUFR_TABLE_DIR=./BUFR4-44/txt/
    ```
* ECMWF Eccodes
    ```shell
    sudo apt install libeccodes-data
    export BUFR_TABLE_DIR=/usr/share/eccodes/definitions/bufr/tables/0/wmo
    ```

## Usage

### Dump BUFR content
```python
# Dump BUFR file(s)

import os
import sys

from bufr_tools import bufr2txt

if __name__ == "__main__":

    msg = ""

    if len(sys.argv) > 1:
        for i, file_name in enumerate(sys.argv[1:]):
            if os.path.exists(file_name):
                msg = bufr2txt.bufr2text(file_name)
                print(msg)
                print(file_name)
            else:
                print("File not exists: {0}".format(file_name))
                exit(1)
    else:
        print(f"Usage: python3 {sys.argv[0]} <bufr_file1> <bufr_file2> ...", file=sys.stderr)
        sys.exit(1)

    sys.exit(0)

```
Note: For OPERA RADAR dump
```shell
export OPERA_RADAR_BUFR=YES
```
### Create E-SOH message(s)

Print E-SOH message
```python
# Extract BUFR file(s) to E-SOH ingest json format

import os
import sys

from bufr_tools import create_mqtt_message_from_bufr

if __name__ == "__main__":

    msg = ""

    if len(sys.argv) > 1:
        for i, file_name in enumerate(sys.argv[1:]):
            if os.path.exists(file_name):
                msg = create_mqtt_message_from_bufr.bufr2mqtt(file_name)
                for m in msg:
                    print("======")
                    print(m)
            else:
                print("File not exists: {0}".format(file_name))
                exit(1)
    else:
        print(f"Usage: python3 {sys.argv[0]} <bufr_file1> <bufr_file2> ...", file=sys.stderr)
        sys.exit(1)

    sys.exit(0)

```

### Encode BUFR content from Coverage json
```python
# Convert E-SOH coverage json to BUFR format

import os
import sys

from bufr_tools import covjson2bufr

if __name__ == "__main__":
    bufr_schema = "default"

    if len(sys.argv) > 1:
        for i, file_name in enumerate(sys.argv):
            if i > 0:
                if os.path.exists(file_name):
                    with open(file_name, "rb") as file:
                        coverage_str = file.read()
                        bufr_content = covjson2bufr.covjson2bufr(coverage_str, bufr_schema)
                        with open("test_out.bufr", "wb") as file:
                            file.write(bufr_content)
                else:
                    print("File not exists: {0}".format(file_name))
                    exit(1)
    else:
        print("Usage: python3 covjson2bufr.py coverage.json")

    exit(0)

```
