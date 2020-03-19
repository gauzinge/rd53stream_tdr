
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "decode/interface/DecoderBase.h"
#include "decode/interface/SimpleStreamDecoder.h"


namespace py = pybind11;

PYBIND11_MODULE(pybindings, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring

    py::class_<SimpleStreamDecoder>(m, "SimpleStreamDecoder")
        .def(py::init())
        .def("load_file", &SimpleStreamDecoder::load_file)
        .def("decode", &SimpleStreamDecoder::decode);

}
