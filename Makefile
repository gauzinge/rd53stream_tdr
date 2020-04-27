IDIR=.
CC=g++
CFLAGS=-I$(IDIR) -std=c++11 `root-config --cflags --glibs` -g -fpermissive

INCDIR=interface
LIBS=-lm

# Python bindings
PYBIND=`python3 -m pybind11 --includes` -fPIC --shared  `python3-config --cflags`  `python3-config --ldflags`
PYBINDIR=pybindir

# Directory for binaries
BINDIR=bin

# Separate src and object directories for 
# encoding, decoding, utilities
ODIR_ENC=encode/obj
ODIR_DEC=decode/obj
ODIR_UTIL=util/obj
SRCDIR_ENC=encode/src
SRCDIR_DEC=decode/src
SRCDIR_UTIL=util

# Set up output directories
$(shell   mkdir -p $(BINDIR))
$(shell   mkdir -p $(ODIR_ENC))
$(shell   mkdir -p $(ODIR_DEC))
$(shell   mkdir -p $(ODIR_UTIL))
$(shell   mkdir -p pyrd53)


SRC_ENC = $(wildcard encode/src/*.cc)
OBJ_ENC = $(addprefix $(ODIR_ENC)/,$(notdir $(SRC_ENC:.cc=.o)))

SRC_DEC = $(wildcard decode/src/*.cc)
OBJ_DEC = $(addprefix $(ODIR_DEC)/,$(notdir $(SRC_DEC:.cc=.o)))

SRC_UTIL = $(wildcard util/*.cc)
OBJ_UTIL = $(addprefix $(ODIR_UTIL)/,$(notdir $(SRC_UTIL:.cc=.o)))

$(ODIR_ENC)/%.o: $(SRCDIR_ENC)/%.cc
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC
$(ODIR_DEC)/%.o: $(SRCDIR_DEC)/%.cc
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC
$(ODIR_UTIL)/%.o: $(SRCDIR_UTIL)/%.cc
	$(CC) -c -o $@ $< $(CFLAGS) -fPIC 

.PHONY: clean

stream: $(OBJ_ENC) $(OBJ_UTIL)
	$(CC) scripts/MakeStream.cc -o $(BINDIR)/makestream $^ $(CFLAGS) $(LIBS)
simple: $(OBJ_DEC) $(OBJ_UTIL)
	$(CC) scripts/RunSimpleDecoder.cc -o $(BINDIR)/runsimple $^ $(CFLAGS) $(LIBS)


py: pybind/PyBindings.cc $(OBJ_DEC) $(OBJ_ENC) $(OBJ_UTIL)
	$(CC) -o pyrd53/pybindings`python3-config --extension-suffix` $(OBJ_DEC) $(OBJ_ENC) $(OBJ_UTIL) $< $(CFLAGS) -fPIC $(PYBIND) -ldl




clean:
	rm -f pyrd53/*.so $(ODIR_ENC)/*.o  $(ODIR_DEC)/*.o $(ODIR_UTIL)/*.o *~ core $(INCDIR)/*~ $(BINDIR)/*
	rmdir pyrd53 $(ODIR_ENC) $(ODIR_DEC) $(ODIR_UTIL) $(BINDIR)

