#include <iostream>
#include <TFile.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
//#include <include/cxxopts.hpp>
#include <util/Util.h>
#include <util/IntMatrix.h>
#include <util/ChipIdentifier.h>

using namespace std;

template<typename T>
std::vector<T> toVec (std::vector<bool>& vec)
{
    //convert a vector<bool> that is anyway padded to 64bit boundaries to a vector<T> where the bits are encoded in the words for binary file storage
    //in the vector the various data fields are MSB ... LSB so we keep that notation
    //in the end, prepend the vector<T> with one word containing the number of words containing the data

    //first check that input vec is padded to 64 bit boundaries
    assert (vec.size() % 64 == 0);

    size_t sizeT = sizeof (T) * 8; //in bits
    std::vector<T> ret_vec;
    size_t ret_vec_size_calc = vec.size() / sizeT;
    ret_vec.reserve (ret_vec_size_calc + 1);
    //now insert the number of words to be read after reading this word
    ret_vec.push_back (static_cast<T> (ret_vec_size_calc) );

    //now loop the word-sized chunks of vec
    for (size_t word = 0; word < ret_vec_size_calc; word++)
    {
        //now loop the bits in word
        T tmp_word = 0;

        for (size_t bit_shift = 0; bit_shift < sizeT; bit_shift++)
        {
            //example: I want to shift bit 7 in the vector in the LSB of a uint8_t ->bit_shift =0
            //and shift bit 6 in LSB+1 etc ->bit_shift=1
            size_t bit = word * sizeT + (sizeT - 1) - bit_shift;
            tmp_word |= (vec[bit] << bit_shift);
        }

        ret_vec.push_back (tmp_word);
    }

    return ret_vec;
}

void to_binary_stream (std::vector<bool>& vec, uint32_t value, size_t nbits)
{
    assert (nbits > 0);

    while (nbits > 0)
    {
        //if (nbits == 0) break;

        bool bit = (value >> (nbits - 1) ) & 0x1;

        //std::cout << nbits - 1 << " " << bit << std::endl;
        //check if the vector size modolo 64 is 0 which means the next bit needs to be a NS=1 or NS=0 bit
        if (vec.size() % 64 == 0 && vec.size() != 0)
        {
            //std::cout << "CASE 1 - I am at a word boundary -  word # " << vec.size() / 64 << std::endl;
            //in this condition, we have 64 bits full but we are not the first bit in the stream
            vec.push_back (0);
        }

        vec.push_back (bit);

        nbits--;

    }
}

//useful overload for the binary vectors of the QCore object
void to_binary_stream (std::vector<bool>& vec, std::vector<bool> to_write)
{
    assert (to_write.size() > 0);

    if (int (64 - (vec.size() % 64) ) - int (to_write.size() )  > 0)
    {
        //the vector to append fits within the same 64 bit word
        //it's insane but If I am at index 0 of the new word and the vec.size() > 0 I need to insert a NS=0 bit

        if (vec.size() % 64 == 0 && vec.size() != 0)
        {
            //std::cout << "Size " << vec.size() << " word " << vec.size() / 64 << " index " << vec.size() % 64 << " left " << 64 - (vec.size() % 64) << " to write " << to_write.size() << " " << 64 - (vec.size() % 64) - to_write.size() <<  std::endl;
            vec.push_back (0);
        }

        vec.insert (vec.end(), to_write.begin(), to_write.end() );
    }
    else
    {
        //I have to wrap to the next 64 bit word and thus need a new stream bit
        for (auto bit : to_write)
        {
            if (vec.size() % 64 == 0 && vec.size() != 0)
            {
                //std::cout << "I am at a word boundary -  word # " << vec.size() / 64 << std::endl;
                //in this condition, we have 64 bits full but we are not the first bit in the stream
                vec.push_back (0);
            }

            //Iknow, not the most efficient but safe and takes care of new stream bit
            vec.push_back (bit);
        }
    }
}

/**
 * Given a vector of QCores per chip, write the formatted data to a vector of bool
 */
std::vector<bool> serializeChip (std::vector<QCore>& qcores, uint32_t event, uint32_t chip, bool tot, bool print = false, std::ostream& f = std::cout)
{
    std::vector<bool> ret_vec;
    bool new_ccol = true;
    //start a new stream so new stream bit is 1
    ret_vec.push_back (1);
    //we assume multi chip mode, so encode 2 bit chip id
    to_binary_stream (ret_vec, chip, 2);
    //8 bits tag, we use event number for this
    to_binary_stream (ret_vec, event, 8);

    //now tackle the QCores
    //int nqcore = 0;
    if (print)
    {
        f << "Number of hit Qcores: " << qcores.size() << std::endl;
        f << "Chip " << chip << "  Event " << event << std::endl;
    }


    for (auto qcore : qcores)
    {
        //int n1 = 0;
        //int n2 = 0;

        if (new_ccol)
            to_binary_stream (ret_vec, qcore.ccol, 6 );

        to_binary_stream (ret_vec, qcore.islast, 1);
        to_binary_stream (ret_vec, qcore.isneighbour, 1);

        if (!qcore.isneighbour)
            to_binary_stream (ret_vec, qcore.qcrow, 8 );

        if (print)
        {
            qcore.print (f);
            f << "islast " << qcore.islast << " isneighbor " << qcore.isneighbour <<  std::endl;
            f << "Encoded hitmap: ";

            for (auto bit : qcore.encoded_hitmap)
                f << bit;

            f << std::endl;

        }


        to_binary_stream (ret_vec, qcore.encoded_hitmap);

        if (tot)
            to_binary_stream (ret_vec, qcore.binary_tots() );

        new_ccol = qcore.islast;
    }

    //now the orphan bits
    size_t orphan_bits = 64 - (ret_vec.size() % 64);
    std::vector<bool> orphan_vec (orphan_bits, 0);
    ret_vec.insert (ret_vec.end(), orphan_vec.begin(), orphan_vec.end() );

    if (orphan_bits < 6)
    {
        //fill the 64 bit word, insert a NS bit = 0 and 63 zeros
        std::vector<bool> orphan_vec (64, 0);
        ret_vec.insert (ret_vec.end(), orphan_vec.begin(), orphan_vec.end() );
    }

    if (print)
    {
        f << "Stream data: " << std::endl;
        size_t index = 0;

        for (auto bit : ret_vec)
        {
            f << bit;
            index++;

            if (index % 64 == 0)
                f << std::endl;
        }

        f << std::endl << std::endl;
    }

    return ret_vec;
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
        {
            //Print out chip coordinates
            //chip.first.print();
            //pick apart each chip int matrix in QCores
            std::vector<QCore> qcores = enc.qcores (chip.second, nevent, chip.first.mmodule, chip.first.mchip);

            //create the actual stream for this chip
            std::stringstream ss;
            std::vector<bool> tmp =  serializeChip (qcores, nevent, chip.first.mchip, true, true, ss);

            //TODO:convert in vector<uint16_t> or vector<uint32_t>, insert header word at beginning of every vector and pad to end
            std::vector<uint16_t> binary_vec = toVec<uint16_t> (tmp);
            std::cout << "Size in 64 bit words: " <<  tmp.size() / 64 << " in 16 bit words " << tmp.size() / 16 << " of the resulting binary vec (remember: +1 for the number of data words!) " << binary_vec.size() << std::endl;

            //chip.first.print();
            //std::cout << "Stream size in 64-bit words for this chip: " << tmp.size() / 64 << std::endl;
            //std::cout << ss.str() << std::endl;
        }

        nevent++;
    }//end event loop
}
