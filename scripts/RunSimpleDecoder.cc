#include <iostream>
//#include<include/cxxopts.hpp>
#include<decode/interface/SimpleStreamDecoder.h>
#include<decode/interface/DecoderBase.h>
#include <vector>

using namespace std;

int main (int argc, char* argv[])
{
    //cxxopts::Options options ("RunSimpleDecoder", "Execute the simple decoder implementation using an input stream file.");
    //options.add_options()
    //("f,file", "Input file", cxxopts::value<string>() )
    //;
    //auto opts = options.parse (argc, argv);

    if (argc == 1)
        std::cout << "please call like so: bin/" << argv[0] << " filepath" << std::endl;

    if (argc != 2)
        std::cout << "please call like so: bin/" << argv[0] << " filepath" << std::endl;

    std::string filename = argv[1];
    std::cout << "opening file " << filename << "...." << std::endl;

    SimpleStreamDecoder decoder;
    //decoder.load_file(opts["file"].as<string>());
    decoder.read_binaryheader<uint16_t> (filename );

    for (size_t nchips = 0; nchips < decoder.getNchips(); nchips++)
    {
        decoder.read_chip<uint16_t>();
        decoder.print_buffer();

        //decoder.decode();
    }
}
