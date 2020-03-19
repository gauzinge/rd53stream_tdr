# rd53stream
A library for simulating data streams coming from RD53 chips.

The library is split into two parts:

1. encoding of raw pixel ADC data into data streams, formatting into AURORA blocks, etc.

2. decoding of the output of step 1 back into raw pixel data.

In both parts, the actual functionality is implemented in C++. Python bindings are generated using `pybind11` to enable easier handling of the C++ objects, but they can also just be operated from within C++.


## Example usage

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

In both cases, the same underlying C++ code is executed.

Python bindings for modules other than the simple decoder will be added soon.