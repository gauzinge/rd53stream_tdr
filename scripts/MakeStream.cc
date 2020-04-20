#include <iostream>
#include <TFile.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
//#include <include/cxxopts.hpp>
#include <encode/interface/Encoder.h>
#include <util/Util.h>
#include <util/IntMatrix.h>
#include <util/ChipIdentifier.h>

using namespace std;


/**
 * Write a vector of bits to the stream output file.
 *
 * The stream is formatted into AURORA blocks of 64 bits length.
 * Each block is written into a new line of the output file.
 */
void write_bits (std::ofstream& f, std::vector<bool> bits, int& bits_written)
{
    for (auto bit : bits)
    {
        if ( (bits_written % 64 == 0) )
        {
            // Start new stream
            if (bits_written > 0)
            {
                f << endl;
                f << "0";
            }
            else
                f << "1";

            bits_written++;
        }

        f << bit ? "1" : "0";
        bits_written++;
    }
}
/**
 * Convenience overload to write a single bit in the same manner.
 */
void write_bits (std::ofstream& f, bool bit, int& bits_written)
{
    std::vector<bool> vec = {bit};
    write_bits (f, vec, bits_written);
}


/**
 * Given a set of QCores, write the formatted data stream to file.
 */
void stream_to_file (std::vector<QCore>& qcores, bool tot)
{
    std::ofstream f;
    f.open ("stream.txt");
    bool new_ccol = true;
    int bits_written = 0;
    std::vector<bool> event = {1, 0, 1, 0, 1, 0, 0, 1};
    write_bits (f, event, bits_written);
    f << "|";
    int nqcore = 0;

    for (auto q : qcores)
    {
        int n1 = 0;
        int n2 = 0;

        if (new_ccol)
        {
            std::vector<bool> ccol_address = int_to_binary (53, 6);
            write_bits (f, ccol_address, bits_written);
            // assert(ccol_address.size()==6);
        }

        f << ":";
        write_bits (f, q.islast, bits_written);
        write_bits (f, q.isneighbour, bits_written);
        f << ":";

        if (not q.isneighbour)
        {
            std::vector<bool> qrow_address = int_to_binary (q.qcrow, 8);
            write_bits (f, qrow_address, bits_written);
        }

        f << ":";
        write_bits (f, q.encoded_hitmap, bits_written);

        if (tot)
        {
            f << ":";
            write_bits (f, q.binary_tots(), bits_written);
        }

        f << "|";
        new_ccol = q.islast;
    }

    // Fill up frame with orphan bits
    while (bits_written % 64 != 0 )
        write_bits (f, false, bits_written);

    f << endl;
    f.close();
}

int main (int argc, char* argv[])
{

    if (argc == 1)
        std::cout << "please call like so: bin/makestream filepath nevents" << std::endl;

    if (argc != 3)
        std::cout << "please call like so: bin/makestream filepath nevents" << std::endl;

    std::string filename = argv[1];

    TFile* file = new TFile (filename.c_str() );
    TTreeReader reader ("BRIL_IT_Analysis/Digis", file);
    TTreeReaderArray<bool> trv_barrel (reader, "barrel");
    TTreeReaderArray<uint32_t> trv_module (reader, "module");
    TTreeReaderArray<uint32_t> trv_adc (reader, "adc");
    TTreeReaderArray<uint32_t> trv_col (reader, "column");
    TTreeReaderArray<uint32_t> trv_row (reader, "row");
    TTreeReaderArray<uint32_t> trv_ringlayer (reader, "ringlayer");
    TTreeReaderArray<uint32_t> trv_diskladder (reader, "diskladder");

    uint32_t nrows_module = 672;
    uint32_t ncols_module = 864;
    int nevents = 0;

    uint32_t nevent = 0;



    // Event loop
    while (reader.Next() )
    {
        if (nevent > nevents)
            break;

        uint32_t ientry = 0;

        // Read all data for this event and construct
        // 2D matrices of pixel ADCs, key == chip identifier
        std::map<ChipIdentifier, IntMatrix> module_matrices;
        std::map<ChipIdentifier, IntMatrix> chip_matrices;
        // QCore objects per module, key == chip identifier
        std::map<ChipIdentifier, std::vector<QCore>> qcores;
        //let's use a seperate instance of the encoder for each event, just to be sure
        Encoder enc;

        // module loop witin event
        // this has one entry for each hit (row, col, adc) touple
        for ( auto imod : trv_module)
        {
            bool barrel = trv_barrel.At (ientry);
            uint32_t diskladder = trv_diskladder.At (ientry);
            uint32_t ringlayer = trv_ringlayer.At (ientry);
            uint32_t module = imod;

            // if it's barrel or not barrel but disk < 9 (TFPX) increment ientry (consider next digi) and continue so only consider TEPX
            if (barrel || (!barrel && diskladder < 9) )
            {
                ientry++;
                continue;
            }

            //generate a temporary chip identifier with fake chip id 99 since for the moment we just care for the module
            ChipIdentifier tmp_id (diskladder, ringlayer, module, 99);
            //check if that module is already in our map
            auto matrices_iterator = module_matrices.find (tmp_id);

            if (matrices_iterator == std::end (module_matrices) ) // not found
            {
                //insert an empty IntMatrix
                IntMatrix tmp_matrix (nrows_module, ncols_module);
                //module_matrices[tmp_id] = tmp_matrix;
                module_matrices.emplace (tmp_id, tmp_matrix);
            }

            //in all other cases the Intmatrix for that module exists already
            //get the row, col and ADC (attention, sensor row and column address) for the current module
            uint32_t row = trv_row.At (ientry);
            uint32_t col = trv_col.At (ientry);
            uint32_t adc = trv_adc.At (ientry);
            //some sanity checks
            //convert to module address
            module_matrices.at (tmp_id).convertPitch_andFill (row, col, adc);
            ientry++;
        }//end module loop

        // Loop over modules and create qcores for each module
        std::cout << "Finished reading full data for Event " << nevent << " from the tree; found " << module_matrices.size() << " modules for TEPX" << std::endl;

        //now split the tmp_matrix in 4 chip matrices, construct the Chip identifier object and insert into the matrices map
        for (auto matrix : module_matrices)
        {
            for (uint32_t chip = 0; chip < 4; chip++)
            {
                ChipIdentifier chipId (matrix.first.mdisk, matrix.first.mring, matrix.first.mmodule, chip);
                IntMatrix tmp_matrix = matrix.second.submatrix (chip);
                chip_matrices.emplace (chipId, tmp_matrix);
            }
        }

        std::cout << "Finished picking apart modules for Event " << nevent << " ; converted " << module_matrices.size() << " modules for TEPX to " << chip_matrices.size() << " chips" << std::endl;




        //int nqcore = 0;
        for (auto chip : chip_matrices )
            chip.first.print();

        //vector<QCore> tmp = enc.qcores(pair.second, nevent, pair.first);
        //qcores[pair.first].insert(qcores[pair.first].end(), tmp.begin(), tmp.end());
        //}

        //// Write the stream for qcores of the **first module only**
        //stream_to_file(qcores.at(1),true);
        nevent++;
    }//end event loop
}
