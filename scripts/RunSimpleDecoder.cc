#include <iostream>
#include<include/cxxopts.hpp>
#include<decode/interface/SimpleStreamDecoder.h>
#include<decode/interface/DecoderBase.h>
#include <vector>

using namespace std;

int main(int argc, char *argv[]){
    cxxopts::Options options("RunSimpleDecoder", "Execute the simple decoder implementation using an input stream file.");
    options.add_options()
    ("f,file", "Input file",cxxopts::value<string>())
    ;
    auto opts = options.parse(argc, argv);

    SimpleStreamDecoder decoder;
    decoder.load_file(opts["file"].as<string>());

    decoder.decode();
}