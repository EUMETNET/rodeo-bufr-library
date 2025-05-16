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

all: bufrprint

bufrprint: bufrprint.o $(objs)
	$(CXX) $(objs) bufrprint.o -o bufrprint

$(objs): %.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f *.o *.so *.cpp~ *.h~ bufrprint
