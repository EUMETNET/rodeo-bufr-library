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

.PHONY: clean all

objs := Sections.o NorBufrIO.o Tables.o Descriptor.o NorBufr.o LogBuffer.o LogEntry.o
eobjs := ESOHBufr.o WSI.o Oscar.o

all: bufrprint

bufresohmsg: bufresohmsg.o $(objs) $(eobjs)
	$(CXX) $(objs) $(eobjs) bufresohmsg.o -o bufresohmsg

bufrprint: bufrprint.o $(objs)
	$(CXX) $(objs) bufrprint.o -o bufrprint

$(eobjs): %.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -o $@ $<

$(objs): %.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f *.o *.so *.cpp~ *.h~ bufrprint bufresohmsg
