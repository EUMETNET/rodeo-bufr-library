#
# (C) Copyright 2023, met.no
#
# This file is part of the Norbufr BUFR en/decoder
#
# Author: istvans@met.no
#
#


ifeq ($(CXX),)
	CXX := g++
endif
#CXX := clang++

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CXXFLAGS := -c -Wall -Wextra -O2 -std=c++17 -fPIC
else
	CXXFLAGS := /c /W3 /O2 /std:c++17 -IC:\temp\rapidjson\rapidjson-1.1.0\include
endif

LDFLAGS := -shared

.PHONY: clean all esoh

objs := Sections.o NorBufrIO.o Tables.o Descriptor.o NorBufr.o LogBuffer.o LogEntry.o
eobjs := ESOHBufr.o WSI.o Oscar.o covjson2bufr.o

#all: bufrprint bufrenc esoh covjson2bufr_main
all: esoh

esoh: bufresohmsg_py.so
	cp bufresohmsg_py.so ./bufresohmsg_py$$(python3-config --extension-suffix)

bufresohmsg_py.so: $(objs) bufresohmsg_py.o
	$(CXX) $(LDFLAGS) $(objs) $(eobjs) bufresohmsg_py.o -o bufresohmsg_py.so

bufresohmsg_py.o: bufresohmsg_py.cpp bufresohmsg_py.h $(objs) $(eobjs)
	$(CXX) $(CXXFLAGS) $$(python3 -m pybind11 --includes) -o bufresohmsg_py.o bufresohmsg_py.cpp

$(eobjs): %.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -o $@ $<

$(objs): %.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f *.o *.so *.cpp~ *.h~ bufrprint bufresohmsg bufrenc covjson2bufr_main
