# Voro++ makefile
#
# Author : Chris H. Rycroft (LBL / UC Berkeley)
# Email  : chr@alum.mit.edu
# Date   : February 27th 2008

# Load the common configuration file
CC=g++
CFLAGS=-Wall -ansi -pedantic -O3

# List of the common source files
SOURCE=container.cc container.hh config.hh cell.hh cell.cc \
       wall.cc wall.hh worklist.cc worklist.hh v_network.cc v_network.hh

# Lists of the command line utilities
EXECUTABLES=network

# Makefile rules
all: $(EXECUTABLES)

#../bin/voro++: $(SOURCE) cmd_line.cc
#	$(CC) $(CFLAGS) -o ../bin/voro++ cmd_line.cc

network: $(SOURCE) network.cc
	$(CC) $(CFLAGS) -o network network.cc

help: Doxyfile $(SOURCE)
	doxygen Doxyfile
	cd ../latex && make

clean:
	rm -f $(EXECUTABLES)

.PHONY: all help clean
