#include <iostream>
#include <TFile.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <TCanvas.h>
#include <TH1F.h>
//#include <include/cxxopts.hpp>
#include <util/Util.h>
#include <util/IntMatrix.h>
#include <util/ChipIdentifier.h>
#include <util/Serializer.h>
#include <encode/interface/Encoder.h>
#include <encode/interface/QCore.h>

using namespace std;



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
    int nevents = 400;

    uint32_t nevent = 0;

    //create one binary data file per DTC - 5 DTC's per side: TEPX D1 & D2 far: DTC1, TEPX D3 & D4 far: DTC2, TEPX D1 & D2 near: DTC3,...
    //D4R1 near & far: DTC5

    std::map<uint32_t, std::ofstream*> outfile_map;
    std::map<uint32_t, TH1F*> eventSizeDTCbits;
    std::map<uint32_t, size_t> WordCountDTC;
    TH1F* eventSizeChipbits = new TH1F ("streamsizeChipsbits", "Distribution of Chip Event sizes [bits]; bits; counts", 30, 0, 1920);
    TH1F* eventSizeChipwords = new TH1F ("streamsizeChipswords", "Distribution of Chip Event sizes [16 bit words]; words; counts", 128, 0, 512);
    //std::map<uint32_t, TH1F*> eventSizeDTC;

    for (uint32_t dtc = 1; dtc <= 5; dtc++)
    {
        std::stringstream ss;
        ss << "dtc_" << dtc << "_data.dat";
        //std::ofstream stream (ss.str(), std::ios::out | std::ios::binary);
        outfile_map[dtc] = new std::ofstream (ss.str().c_str(), std::ios::out | std::ios::binary);

        if (!outfile_map[dtc] || !*outfile_map[dtc])
            perror (ss.str().c_str() );

        //the perDTC histograms
        std::stringstream namestream;
        std::stringstream titlestream;
        namestream << "EventSizeDTC" << dtc;
        titlestream << "Event Size for DTC " << dtc << ";16 bit words;count";

        eventSizeDTCbits[dtc] = new TH1F (namestream.str().c_str(), titlestream.str().c_str(), 100, 0, 30000);
    }

    // Event loop
    while (reader.Next() )
    {
        if (nevent > nevents)
            break;

        //clear the word counter for the DTCs for each event
        for (uint32_t dtc = 1; dtc <= 5; dtc++)
            WordCountDTC[dtc] = 0;

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

            //convert to module address (different row and column numbers because 50x50)
            module_matrices.at (tmp_id).convertPitch_andFill (row, col, adc);
            ientry++;
        }//end module loop

        std::cout << "Finished reading full data for Event " << nevent << " from the tree; found " << module_matrices.size() << " modules for TEPX" << std::endl;

        //do all of the below only for the first event
        if (nevent == 0)
        {
            //for each DTC, encode the module coordinates (disk,ring,moduleID, # of chips) in a vector<uint16_t>, determine the size of that vector and create a file header for each DTC: 1st word: data format version, 2nd word, size of the module list, then: 1 word per module;
            std::map<uint32_t, std::vector<uint16_t>> dtc_map;

            for (auto module : module_matrices)
                dtc_map[module.first.mdtc].push_back (module.first.encode<uint16_t>() );

            for (auto& dtc : dtc_map)
            {
                std::cout << "DTC " << dtc.first << " has " << dtc.second.size() << " Modules connected" << std::endl;
                uint16_t header_version = 1;
                dtc.second.insert (dtc.second.begin(), dtc.second.size() );
                dtc.second.insert (dtc.second.begin(), header_version );
                std::cout << "Now the size is " << dtc.second.size() << " and the header version is " << dtc.second.at (0) << " and the header (module list) size is " << dtc.second.at (1) << std::endl;
                //now to file
                Serializer::to_file<uint16_t> (outfile_map[dtc.first], dtc.second);
            }
        }


        //now split the tmp_matrix  for each module in 4 chip matrices, construct the Chip identifier object and insert into the matrices map
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

        for (auto chip : chip_matrices )
        {
            //pick apart each chip int matrix in QCores
            std::vector<QCore> qcores = enc.qcores (chip.second, nevent, chip.first.mmodule, chip.first.mchip);

            //create the actual stream for this chip
            std::stringstream ss;
            std::vector<bool> tmp =  Serializer::serializeChip (qcores, nevent, chip.first.mchip, true, true, ss);

            std::vector<uint16_t> binary_vec = Serializer::toVec<uint16_t> (tmp);
            Serializer::to_file<uint16_t> (outfile_map[chip.first.mdtc], binary_vec);
            //increment the wordcount for this DTC
            WordCountDTC[chip.first.mdtc] += binary_vec.size();

            //fill the histograms for the chip data size
            eventSizeChipbits->Fill (binary_vec.at (0) *sizeof (uint16_t) * 8 ); //each binary vector is prepended with the number of words per chip in the typename
            eventSizeChipwords->Fill (binary_vec.at (0) ); //each binary vector is prepended with the number of words per chip in the typename



            //std::cout << "Size in 64 bit words: " <<  tmp.size() / 64 << " in 16 bit words " << tmp.size() / 16 << " of the resulting binary vec (remember: +1 for the number of data words!) " << binary_vec.size() << std::endl;

            //chip.first.print();
            //std::cout << "Stream size in 64-bit words for this chip: " << tmp.size() / 64 << std::endl;
            //std::cout << ss.str() << std::endl;
        }

        for (uint32_t dtc = 1; dtc <= 5; dtc++)
        {
            eventSizeDTCbits[dtc]->Fill (WordCountDTC[dtc]);
            std::cout << "Event: " << nevent << " DTC: " << dtc << " has a word count (16bit) of " << WordCountDTC[dtc] << std::endl;
        }

        nevent++;
    }//end event loop

    TCanvas* canvas = new TCanvas();
    canvas->Divide (2, 4);
    canvas->cd (1);
    eventSizeChipbits->Draw();
    canvas->cd (2);
    eventSizeChipwords->Draw();

    //close the files
    for (auto& file : outfile_map)
    {
        file.second->close();
        delete file.second;
        canvas->cd (file.first + 2);
        eventSizeDTCbits[file.first]->Draw();
    }

    canvas->SaveAs ("Bandwidth.pdf");
    canvas->SaveAs ("Bandwidth.root");
}
