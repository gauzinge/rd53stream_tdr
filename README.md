# rd53stream
A library for simulating data streams coming from RD53 chips.

The library is split into two parts:

1. encoding of raw pixel ADC data into data streams, formatting into AURORA blocks, etc.

2. decoding of the output of step 1 back into raw pixel data.

In both parts, the actual functionality is implemented in C++. Python bindings are generated using `pybind11` to enable easier handling of the C++ objects, but they can also just be operated from within C++.

## Encoder example

To run the encoder, we need in input file containing a TTree with the digis from simulation. 

```bash
wget https://aalbert.web.cern.ch/aalbert/public/rd53stream/itdigi.root -O example/itdigi.root

make stream
./bin/makestream -file example/itdigi.root \
             -barrel \
             -ringlayer 1 \
             -diskladder 1 \
             -nevents 1
```

The commandline arguments define which part of the detector is analyzed, with `-barrel` referring to the TBPX, while `-endcap` refers to the TFPX and TEPX. The `-ringlayer` and `-diskladder` arguments further narrow down the detector region, which `ringlayer` meaning `ring` for the endcaps and `layer` for the barrel (similarly for diskladder).

The output will be written to `stream.txt`. The first few lines of the file will look something like this:

```
110101001|110101:10:10010001:0110000:11110101|110101:00:10010001:00111110:
0101111110100|:11::1010011:00111011|110101:10:10010010:111110111011111000:
0110100101111111101000010|110101:10:01101011:01000:1010|110101:00:011010
011:011101110010:000101111110|:11::1010011:11001101|110101:10:01101100:1011
010101010:01100100|110101:00:10000011:1101000100:00100010|:11::11111010001
```

Each line represents a 64-bit AURORA block (the 2-bit AURORA header has been omitted here). To make the files more human-readable, the bits are interspersed with the symbols "|" which indicates a new quarter core starting, and ":", which indicates a new field inside a quarter core. These symbols are only meant for human consumption and are ignored by the decoder! The start of the stream can thus be decomposed as follows:

```
110101001   -> new stream bit "1" + 8-bit event identifier "10101001"
|           -> start of first qcore
110101      -> Column address of the qcore (Always 6 bits)
10          -> islast and isneighbour bits
10010001    -> row address (always 8 bits, omitted if isneighbour=True)
0110000     -> encoded hitmap (variable size)
11110101    -> ToTs for hit pixels (2 pixels * 4 bits / pixel)

...(and so on)...
```

It is important to note that the stream building currently has limitations:
* Each module is treated separately, ratther than correctly accounting for the module -> ROC mapping
* event numbers, as well as row and column identifiers are arbitrary placeholder values
* The stream decoder is not yet fully validated.


## Decoder example 

As an example, let's run the SimpleStreamDecoder on the input file `example/stream.txt`. We can do it in two ways: Either entirely in C++:

```bash
make simple
./bin/runsimple -f example/stream.txt
```

or using the python bindings (assuming we are in a virtual environment with python3):

```bash
pip install pybind11
make py
./python/run_simple_decoder.py example/stream.txt
```

In both cases, the same underlying C++ code is executed. Of course, you can also run the decoder on the `stream.txt` file created in the encoding step above.

Python bindings for modules other than the simple decoder will be added soon.
