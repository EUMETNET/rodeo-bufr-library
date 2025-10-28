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
from bufr_tools import bufr2tx
msg = bufr2txt.bufr2text(bufr_file_name)
print(msg)

```

### Create E-SOH message(s)

Print E-SOH message
```python
import create_mqtt_message_from_bufr
msg = create_mqtt_message_from_bufr.bufr2mqtt(bufr_file_name)
for m in msg:
  print(m)

```

#### Encode BUFR content from Coverage json
TBD
