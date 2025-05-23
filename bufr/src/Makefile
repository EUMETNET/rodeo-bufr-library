#
# (C) Copyright 2023, met.no
#
# This file is part of the Norbufr BUFR en/decoder
#
# Author: istvans@met.no
#
#


CXX := g++
#CXX := clang++
CXXFLAGS := -c -Wall -Wextra -O2 -std=c++17 -fPIC
LDFLAGS := -shared

.PHONY: clean all esoh

objs := Sections.o NorBufrIO.o Tables.o Descriptor.o NorBufr.o LogBuffer.o LogEntry.o
eobjs := ESOHBufr.o WSI.o Oscar.o

all: bufrprint bufrenc

esoh: bufresohmsg_py.so
	cp bufresohmsg_py.so ./bufresohmsg_py$$(python3-config --extension-suffix)

bufresohmsg_py.so: $(objs) bufresohmsg_py.o
	$(CXX) $(LDFLAGS) $(objs) $(eobjs) bufresohmsg_py.o -o bufresohmsg_py.so

bufresohmsg_py.o: bufresohmsg_py.cpp bufresohmsg_py.h $(objs) $(eobjs)
	$(CXX) $(CXXFLAGS) $$(python3 -m pybind11 --includes) -o bufresohmsg_py.o bufresohmsg_py.cpp

bufresohmsg: bufresohmsg.o $(objs) $(eobjs)
	$(CXX) $(objs) $(eobjs) bufresohmsg.o -o bufresohmsg

bufrenc: bufrenc.o $(objs)
	$(CXX) $(objs) bufrenc.o -o bufrenc

bufrprint: bufrprint.o $(objs)
	$(CXX) $(objs) bufrprint.o -o bufrprint

$(eobjs): %.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -o $@ $<

$(objs): %.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f *.o *.so *.cpp~ *.h~ bufrprint bufresohmsg bufrenc
