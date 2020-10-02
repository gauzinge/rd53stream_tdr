#include <encode/interface/EventEncoder.h>

#include <util/Util.h>
#include <util/IntMatrix.h>
#include <util/ChipIdentifier.h>
#include <util/Serializer.h>
#include <encode/interface/Encoder.h>
#include <encode/interface/QCore.h>
#include <sstream>
#include <bitset>

using namespace std;

EncodedEvent::EncodedEvent ()
{
    is_empty_var = false;
}

std::pair<ChipIdentifier, std::vector<uint16_t>> EncodedEvent::get_next_chip()
{
    std::pair<ChipIdentifier, std::vector<uint16_t>> ret;
    if (streams_iterator != streams.end()) {
        ret = *streams_iterator;
        streams_iterator++;
    }
    return ret;
}

void EncodedEvent::print_chip(ChipIdentifier identifier) {
    std::vector<uint16_t> stream = streams[identifier];
    IntMatrix chip_matrix = chip_matrices[identifier];
    std::cout << "Disk: " << identifier.mdisk << ", Ring: " << identifier.mring << ", Module: " << identifier.mmodule << ", Chip: " << identifier.mchip << std::endl;
    std::cout << "\tHits: ";
    for(auto hit : chip_matrix.hits()) std::cout << "(col " << hit.first << ", row " << hit.second << ") ";
    std::cout << std::endl;
    std::cout << "\tQcores: " << std::endl;
    Encoder enc;
    std::vector<QCore> qcores = enc.qcores (chip_matrix, 0, identifier.mmodule, identifier.mchip);
    for(auto qcore : qcores) {
        std::cout << "\t\tCol: " << qcore.ccol << ", Row: " << qcore.qcrow << ", islast: " << qcore.islast << ", isneighbour: " << qcore.isneighbour << std::endl;
        std::cout << "\t\t";
        for(auto hit : qcore.hitmap) std::cout << hit << " ";
        std::cout << std::endl;
    }
    std::cout << "\tStream: ";
    for(auto word : stream) {
        std::bitset<16> word_binary(word);
        std::cout << word_binary.to_string() << " ";
    }
    std::cout << std::endl;
}

void EncodedEvent::print() {
    for(auto streams_item : streams) {
        ChipIdentifier identifier = streams_item.first;
        print_chip(identifier);
    }
}

EventEncoder::EventEncoder (std::string pFilename)
{
    event_id = 0;
    file = new TFile (pFilename.c_str() );
    reader = new TTreeReader ("BRIL_IT_Analysis/Digis", file);
    trv_barrel = new TTreeReaderArray<bool> (*reader, "barrel");
    trv_module = new TTreeReaderArray<uint32_t> (*reader, "module");
    trv_adc = new TTreeReaderArray<uint32_t> (*reader, "adc");
    trv_col = new TTreeReaderArray<uint32_t> (*reader, "column");
    trv_row = new TTreeReaderArray<uint32_t> (*reader, "row");
    trv_ringlayer = new TTreeReaderArray<uint32_t> (*reader, "ringlayer");
    trv_diskladder = new TTreeReaderArray<uint32_t> (*reader, "diskladder");
}

EncodedEvent EventEncoder::get_next_event()
{
    // create empty encoded event
    EncodedEvent encoded_event;

    // check that we have some data
    if (!(reader->Next())) {
        encoded_event.set_empty(true);
        return encoded_event;
    }

    // iteration
    uint32_t ientry = 0;

    // Read all data for this event and construct
    // 2D matrices of pixel ADCs, key == chip identifier
    std::map<ChipIdentifier, IntMatrix> module_matrices;
    std::map<ChipIdentifier, IntMatrix> chip_matrices;
    // QCore objects per module, key == chip identifier
    std::map<ChipIdentifier, std::vector<QCore>> qcores;
    std::map<ChipIdentifier, std::vector<uint16_t>> streams;
    //let's use a seperate instance of the encoder for each event, just to be sure
    Encoder enc;

    // module loop witin event
    // this has one entry for each hit (row, col, adc) touple
    for ( auto imod : *trv_module)
    {
        bool barrel = trv_barrel->At (ientry);
        uint32_t diskladder = trv_diskladder->At (ientry);
        uint32_t ringlayer = trv_ringlayer->At (ientry);
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
        uint32_t row = trv_row->At (ientry);
        uint32_t col = trv_col->At (ientry);
        uint32_t adc = trv_adc->At (ientry);

        //convert to module address (different row and column numbers because 50x50)
        module_matrices.at (tmp_id).convertPitch_andFill (row, col, adc);
        ientry++;
    }//end module loop

    std::cout << "Finished reading full data for event from the tree; found " << module_matrices.size() << " modules for TEPX" << std::endl;

    //do all of the below only for the first event
    if (event_id == 0)
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
    encoded_event.set_chip_matrices(chip_matrices);

    std::cout << "Finished picking apart modules; converted " << module_matrices.size() << " modules for TEPX to " << chip_matrices.size() << " chips" << std::endl;

    for (auto chip : chip_matrices )
    {
        //pick apart each chip int matrix in QCores
        std::vector<QCore> qcores = enc.qcores (chip.second, event_id, chip.first.mmodule, chip.first.mchip);

        //create the actual stream for this chip
        std::stringstream ss;
        std::vector<bool> tmp =  Serializer::serializeChip (qcores, event_id, chip.first.mchip, false, true, ss);
        std::vector<uint16_t> binary_vec = Serializer::toVec<uint16_t> (tmp);

        // store in stream
        ChipIdentifier chipId (chip.first.mdisk, chip.first.mring, chip.first.mmodule, chip.first.mchip);
        streams.emplace (chipId, binary_vec);
    }
    encoded_event.set_streams(streams);

    // increment
    event_id++;

    // return
    return encoded_event;
}