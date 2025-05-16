# E-SOH

## EURODEO

The RODEO project develops a user interface and Application Programming Interfaces (API) for accessing meteorological datasets declared as High Value Datasets (HVD) by the EU Implementing Regulation (EU) 2023/138 under the EU Open Data Directive (EU) 2019/1024. The project also fosters the engagement between data providers and data users for enhancing the understanding of technical solutions being available for sharing and accessing the HVD datasets.
This project provides a sustainable and standardized system for sharing real-time surface weather observations in line with the HVD regulation and WMO WIS 2.0 strategy. The real-time surface weather observations are made available through open web services, so that they can be accessed by anyone.

## Near real-time observational data

E-SOH is part of the RODEO project. The goal for this project is to make near real-time weather observations from land based station easily available. The data will be published on both a message queue using [MQTT](https://mqtt.org/) and [EDR](https://ogcapi.ogc.org/edr/) compliant APIs. Metadata will also be made available through [OGC Records](https://ogcapi.ogc.org/records/) APIs. The system architecture is portable, scalable and modular for taking into account possible future extensions to existing networks and datasets (e.g. 3rd party surface observations).

## RODEO BUFR Library

This tool handles the BUFR messages:
* ecoding the messages for E-SOH/openradardata ingestion
* providing E-SOH API output in BUFR format

The library suports [ECMWF ecCodes](https://confluence.ecmwf.int/display/ECC), [WMO](https://github.com/wmo-im/BUFR4/) and OPERA BUFR tables.

## Installation
### Clone the repo
```shell 
git clone https://github.com/EUMETNET/rodeo-bufr-library.git
```
### Compiling
```shell
cd rodeo-bufr-library/bufr/src
make
```
### Usage
### Set up BUFR table directory
#### ecCodes tables
To use the ecCodes table definitions you need to install libeccodes-data package on Debian based systems. In this case you don't have to set the BUFR_TABLE_DIR environment variable. 

#### WMO tables
Download WMO tables with the script:
```shell
cd rodeo-bufr-library/bufr/tables/wmo
./get_wmo_tables.sh
export BUFR_TABLE_DIR=path_to_the_repo/rodeo-bufr-library/bufr/tables/wmo/
```
#### OPERA tables
```shell
export BUFR_TABLE_DIR=path_to_the_repo/rodeo-bufr-library/bufr/tables/opera/
```

### Print BUFR content
#### Basic print
```shell
./printbufr path_to_the_bufr_file(s) 
```
#### Detail print
```shell
./printbufr detail path_to_the_bufr_file(s) 
```
#### Log print 
```shell
./printbufr log_print path_to_the_bufr_file(s) 
```
