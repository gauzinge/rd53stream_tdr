#!/usr/bin/env python
import sys
import os
import pyrd53.pybindings

infile = sys.argv[1]
if not os.path.exists(infile):
    raise FileNotFoundError(infile)

decoder=pyrd53.pybindings.SimpleStreamDecoder()
decoder.load_file(infile)
decoder.decode()
