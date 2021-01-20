#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "decode/interface/DecoderBase.h"
#include "decode/interface/SimpleStreamDecoder.h"
#include "encode/interface/EventEncoder.h"
#include "util/ChipIdentifier.h"

namespace py = pybind11;

PYBIND11_MODULE(pybindings, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring

    py::class_<SimpleStreamDecoder>(m, "SimpleStreamDecoder")
        .def(py::init())
        .def("load_file", &SimpleStreamDecoder::load_file)
        .def("decode", &SimpleStreamDecoder::decode);

    py::class_<SimpleCluster>(m, "SimpleCluster")
        .def(py::init<uint32_t, uint32_t, std::vector<int>>())
        .def("get_hits", &SimpleCluster::GetHits);

    py::class_<ChipIdentifier>(m, "ChipIdentifier")
        .def(py::init<uint16_t, uint16_t, uint16_t, uint16_t, uint16_t>());

    py::class_<EncodedEvent>(m, "EncodedEvent")
        .def(py::init())
        .def(py::init<const EncodedEvent &>())
        .def("is_empty", &EncodedEvent::is_empty)
        .def("get_chip_hits", &EncodedEvent::get_chip_hits)
        .def("chip_str", &EncodedEvent::chip_str)
        .def("event_str", &EncodedEvent::event_str)
        .def("print", &EncodedEvent::print)
        .def("get_next_chip", &EncodedEvent::get_next_chip)
        .def("get_stream_by_id", &EncodedEvent::get_stream_by_id)
        .def("get_chip_nclusters", &EncodedEvent::get_chip_nclusters)
        .def("get_chip_clusters", &EncodedEvent::get_chip_clusters)
        .def("get_chip_was_split", &EncodedEvent::get_chip_was_split)
        .def("get_event_id", &EncodedEvent::get_event_id)
        .def("get_event_id_raw", &EncodedEvent::get_event_id_raw);

    py::class_<EventEncoder>(m, "EventEncoder")
        .def(py::init<std::string>())
        .def("get_next_event", &EventEncoder::get_next_event)
        .def("skip_events", &EventEncoder::skip_events);

}
