# rd53stream
A library for simulating data streams coming from RD53 chips.

The library is split into two parts:

1. encoding of raw pixel ADC data into data streams, formatting into AURORA blocks, etc.

2. decoding of the output of step 1 back into raw pixel data.

In both parts, the actual functionality is implemented in C++. Python bindings are generated using `pybind11` to enable easier handling of the C++ objects, but they can also just be operated from within C++.

This version is purely TEPX specific and only considers the +z end of TEPX, so only modules on Disks 9 - 12 for sside == 1.

## Encoder example

To run the encoder, we need in input file containing a TTree with the digis from simulation. 

```bash
wget https://aalbert.web.cern.ch/aalbert/public/rd53stream/itdigi.root -O example/itdigi.root

make stream
./bin/makestream example/itdigi.root
```
You should edit the scripts/MakeStream.cc file with the right number of events or add proper command line parsing ;-) .


The output will be written to four binary data files labelled ``dtc_x_data.dat``. The mapping of DTCs is explained in the utils/ChipIdentifier.h file. Basically disks 9 & 10 near are connected to a DTC, disks 11 & 12 near to another and the same for far. In the current implementation all modules of Disk 4 (D12) Ring 1 are connected to a single DTC for BRIL and not to any other DTC.

The binary data files are based on 16 bit word size but this could be changed since all relevant functions are templated. The first word in the file is the header version, the next word contains the number of words compromising the header (headersize). The next headersize words contain a list of modules connected to this DTC with the Disk, Ring, Module ID & # of chips encoded in a singel 16 bit word. 

After the header, the data is present in blocks where each block contains data from an individual chip. The structure is:
 - 1 6 bit word with the block size (number of 16 bit words) which is the number of words to read
 - blocksize words for the chip data. This is a chopped-up version of the 64 bit aurora blocks that the RD53 chip produces, including new-stream bits and orphan bits at the end of each chip block.

 The 64 bit aurora blocks always start with a new-stream (NS) bit followed by 2 bit chip identifier that identiefies the ID of the chip within a 2x2 module. Numbering starts top left and increases clockwise. Then the event data is correctly encoded using the binary tree encoding as described in the RD53 manual using correct quarter core addresses etc. For the time being, it is assumed that events are contained in a single stream so there is a one-to-one mapping between events and streams. Also, the event number from the input file is used as 8 bit RD53 tag.
Orphan bits are also correctly implemented: so if a stream ends with less than 6 orphan bits, a full 64 bit word of 0s is appended.

There is a bunch of printout statements in the MakeStream.cc script that can be enabled in oder to see the quarter core information and binary tree encoding / binary data - no text files are written in this version.

This version also correctly translates the 100x25um sensor pitch digis from CMSSW to the 50x50 pixel matrix size on the RD53 and splits each 2x2 module in the individual chips so it's a fairly realistic representation.

## Decoder example 

As an example, let's run the SimpleStreamDecoder on the `dtc_x_data.dat`. 

```bash
make simple
./bin/runsimple dtc_x_data.dat
```

The decoder reads the file, first the header and then the individual chunks for each chip and prints out the header info (version and module list) followed by the decoded event data for each chip. The decoder has been verified against the encoder and produces the same result. 

Please note that in order to simplify decoding, the decoder in the `SimpleStreamDecoder::decode()` method removes the NS bits as well as the chip ID form all but the first 64bit aurora block to simplify decoding.

Have fun!

