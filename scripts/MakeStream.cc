#include <iostream>
#include<TFile.h>
#include<TTreeReader.h>
#include<TTreeReaderArray.h>
#include<include/cxxopts.hpp>
#include<encode/interface/Encoder.h>
#include<util/Util.h>
#include<util/IntMatrix.h>
#include<util/ChipIdentifier.h>

using namespace std;

//IntMatrix empty_matrix(uint32_t nrows, uint32_t ncols){
    //IntMatrix matrix;
    //matrix.reserve(nrows);
    //for (uint32_t r = 0; r < nrows; r++) {
            //vector<ADC> row;
            //row.reserve(ncols);
            //for (uint32_t c = 0; c < ncols; c++) {
                  //row.push_back(0);
            //}
            //assert(row.size()==ncols);
            //matrix.push_back(row);
    //}
    //assert(matrix.size()==nrows);
    //return matrix;
//}

//IntMatrix getChipMatrix(uint32_t chip, IntMatrix theMatrix)
//{

//}

/**
 * Convert a row and column address from sensor (1344x432) to module coordinates (672x864).
 */
//void convert_address(uint32_t& row, uint32_t& col)
//{
    //if(row & 1 == 0)//this is a simple check if row is an even number, alternative (row%2 == 0)
    //{
        //row/=2;
        //col *=2;
        //col+=1;
    //}
    //else// odd row number
    //{
        //row-=1;
        //row/=2;
        //col*=2;
    //}
//}


/**
 * Write a vector of bits to the stream output file.
 * 
 * The stream is formatted into AURORA blocks of 64 bits length.
 * Each block is written into a new line of the output file.
 */
void write_bits(ofstream & f, vector<bool> bits, int & bits_written) {
    for (auto bit: bits){
        if((bits_written % 64 == 0)){
            // Start new stream
            if(bits_written > 0) {
                f << endl;
                f << "0";
            } else {
                f << "1";
            }
            bits_written++;
        }
        f << bit ? "1" : "0";
        bits_written++;
    }
}
/**
 * Convenience overload to write a single bit in the same manner.
 */
void write_bits(ofstream & f, bool bit, int & bits_written) {
    vector<bool> vec = {bit};
    write_bits(f, vec, bits_written);
}


/** 
 * Given a set of QCores, write the formatted data stream to file.
 */
void stream_to_file(vector<QCore> & qcores, bool tot){
    ofstream f;
    f.open("stream.txt");
    bool new_ccol = true;
    int bits_written = 0;
    vector<bool> event = {1,0,1,0,1,0,0,1};
    write_bits(f, event, bits_written);
    f<<"|";
    int nqcore=0;
    for(auto q: qcores){
        int n1=0;
        int n2=0;
        if(new_ccol){
            vector<bool> ccol_address = int_to_binary(53, 6);
            write_bits(f, ccol_address, bits_written);
            // assert(ccol_address.size()==6);
        }
        f<<":";
        write_bits(f, q.islast, bits_written);
        write_bits(f, q.isneighbour, bits_written);
        f<<":";
        if(not q.isneighbour){
            vector<bool> qrow_address = int_to_binary(q.qcrow, 8);
            write_bits(f, qrow_address, bits_written);
        }
        f<<":";
        write_bits(f, q.encoded_hitmap, bits_written);

        if(tot){
            f<<":";
            write_bits(f, q.binary_tots(), bits_written);
        }
        f<<"|";
        new_ccol=q.islast;
    }

    // Fill up frame with orphan bits
    while(bits_written % 64 != 0 ) {
        write_bits(f, false, bits_written);
    }
    f << endl;
    f.close();
}

int main(int argc, char *argv[]){
    cxxopts::Options options("ITRate", "One line description of MyProgram");
    options.add_options()
    ("f,file", "Input file",cxxopts::value<string>())
    //("b,barrel", "Look at barrel",cxxopts::value<bool>())
    //("e,endcap", "Look at endcap",cxxopts::value<bool>())
    //("r,ringlayer", "Ring or layer to consider",cxxopts::value<int>())
    //("d,diskladder", "Disk or ladder to consider",cxxopts::value<int>())
    ("n,nevents", "Number of events to process",cxxopts::value<int>())
    ;
    auto opts = options.parse(argc, argv);

    TFile * file = new TFile(opts["file"].as<string>().c_str());
    TTreeReader reader("BRIL_IT_Analysis/Digis", file);
    TTreeReaderArray<bool> trv_barrel(reader, "barrel");
    TTreeReaderArray<uint32_t> trv_module(reader, "module");
    TTreeReaderArray<uint32_t> trv_adc(reader, "adc");
    TTreeReaderArray<uint32_t> trv_col(reader, "column");
    TTreeReaderArray<uint32_t> trv_row(reader, "row");
    TTreeReaderArray<uint32_t> trv_ringlayer(reader, "ringlayer");
    TTreeReaderArray<uint32_t> trv_diskladder(reader, "diskladder");

    uint32_t nrows_module = 672;
    uint32_t ncols_module = 864;
    int nevents =  opts["nevents"].as<int>();
    //int want_ring = opts["ringlayer"].as<int>();
    //int want_disk = opts["diskladder"].as<int>();

    //bool want_barrel = opts["barrel"].as<bool>();
    //bool want_endcap = opts["endcap"].as<bool>();

    //if(want_barrel and want_endcap){
        //throw "Cannot do barrel and endcap at the same time.";
    //}

    uint32_t nevent = 0;

    // 2D matrices of pixel ADCs, key == chip identifier
    map<ChipIdentifier, IntMatrix> matrices;
    // QCore objects per module, key == chip identifier
    map<ChipIdentifier, vector<QCore>> qcores;

    //IntMatrix matrix;
    Encoder enc;

    // Event loop
    while (reader.Next()) 
    {
        if(nevent > nevents){
            break;
        }
        uint32_t ientry=0;

        // Read all data for this event and construct
        // 2D matrices of pixel ADCs **per module**

        // module loop witin event
        for( auto imod : trv_module) 
        {
            bool barrel = trv_barrel.At(ientry);
            uint32_t diskladder = trv_diskladder.At(ientry);
            uint32_t ringlayer = trv_ringlayer.At(ientry);
            uint32_t module = imod;

            // if it's barrel or not barrel but disk < 9 (TFPX) increment ientry and continue so only consider TEPX
            
            if(barrel ||(!barrel && diskladder < 9)){
                ientry++;
                continue;
            }

            //in this case it is a TEPX module so declare an empty matrix with module dimensions
            IntMatrix tmp_matrix(nrows_module, ncols_module);

            //get the row, col and ADC (attention, sensor row and column address) for the current module
            uint32_t row = trv_row.At(ientry);
            uint32_t col = trv_col.At(ientry);
            uint32_t adc = trv_adc.At(ientry);
            //some sanity checks
            assert(row<nrows);
            assert(col<ncols);
            //convert to module address
            convert_address(row, col);

            tmp_matrix.convertPitch_andFill(row, col, adc);
            ientry++;
        }

        // Loop over modules and create qcores for each module
        cout << "Event " << nevent << endl;
        int nqcore = 0;
        for(auto pair : matrices){
            vector<QCore> tmp = enc.qcores(pair.second, nevent, pair.first);
            //for(auto const q : tmp){
                //nqcore++;

                //int bits = 2;
                //if(not q.isneighbour) bits += 8;
                //if(q.islast) bits += 6;
                //bits+=q.encoded_hitmap.size();

                //int nhits = 0;
                //for(auto bit : q.hitmap) {
                    //if(bit) nhits++;
                //}

                //bool row_1_hit = false;
                //bool row_2_hit = false;
                //for(int ipix=0; ipix<8; ipix++) {
                    //row_1_hit |= q.hitmap.at(ipix);
                    //row_2_hit |= q.hitmap.at(8+ipix);
                //}
                //int n_rows = 0;
                //if(row_1_hit) n_rows++;
                //if(row_2_hit) n_rows++;
            //}
            qcores[pair.first].insert(qcores[pair.first].end(), tmp.begin(), tmp.end());
        }

        // Write the stream for qcores of the **first module only**
        stream_to_file(qcores.at(1),true);
        nevent++;
    }
}
