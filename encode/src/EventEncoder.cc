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
    is_hlt_present_var = false;
}

/*EncodedEvent::EncodedEvent (const EncodedEvent &ev)
{
    is_empty_var = bool(ev.is_empty_var);
    chip_matrices = std::map<ChipIdentifier, IntMatrix> (ev.chip_matrices);
    streams = std::map<ChipIdentifier, std::vector<uint16_t>> (ev.streams);
    streams_iterator = std::map<ChipIdentifier, std::vector<uint16_t>>::iterator (ev.streams_iterator);
}*/

std::pair<ChipIdentifier, std::vector<uint16_t>> EncodedEvent::get_next_chip()
{
    std::pair<ChipIdentifier, std::vector<uint16_t>> ret;
    if (streams_iterator != streams.end()) {
        ret = *streams_iterator;
        streams_iterator++;
        IntMatrix chip_matrix = chip_matrices[ret.first];
        if (chip_matrix.hits().size() > 0)
            return ret;
        else
            return this->get_next_chip();
    } else {
        // return empty
        return ret;
    }
}

std::string EncodedEvent::chip_str(ChipIdentifier identifier) {
    std::vector<uint16_t> stream = streams[identifier];
    IntMatrix chip_matrix = chip_matrices[identifier];
    std::ostringstream out;
    out << "Disk: " << identifier.mdisk << ", Ring: " << identifier.mring << ", Module: " << identifier.mmodule << ", Chip: " << identifier.mchip << std::endl;
    out << "\tNumber of clusters: " << chip_clusters[identifier].size() << std::endl;
    int cluster_id = 0;
    for (auto cluster : chip_clusters[identifier]) {
        out << "\t\tCluster " << cluster_id << ", Hits: ";
        std::vector<int> hits = cluster.GetHits();
        for(auto hit: hits) {
            out << "(col " << ((hit >> 0) & 0xffff) << ", row " << ((hit >> 16) & 0xffff) << ") ";
        }
        out << std::endl;
        cluster_id++;
    }
    out << "\tHits: ";
    for(auto hit : chip_matrix.hits()) out << "(col " << hit.first << ", row " << hit.second << ") ";
    out << std::endl;
    out << "\tQcores: " << std::endl;
    Encoder enc;
    std::vector<QCore> qcores = enc.qcores (chip_matrix, 0, identifier.mmodule, identifier.mchip);
    for(auto qcore : qcores) {
        out << "\t\tCol: " << qcore.ccol << ", Row: " << qcore.qcrow << ", islast: " << qcore.islast << ", isneighbour: " << qcore.isneighbour << std::endl;
        out << "\t\t";
        for(auto hit : qcore.hitmap) out << hit << " ";
        out << std::endl;
    }
    out << "\tStream: ";
    for(auto word : stream) {
        std::bitset<16> word_binary(word);
        out << word_binary.to_string() << " ";
    }
    out << std::endl;

    return out.str();
}

void EncodedEvent::print() {
    for(auto streams_item : streams) {
        ChipIdentifier identifier = streams_item.first;
        std::cout << chip_str(identifier);
    }
}

EventEncoder::EventEncoder (std::string pFilenameRaw, std::string pFilenameHlt)
{
    // reset event id
    event_id = 0;

    // init files
    this->init_files(pFilenameRaw, pFilenameHlt);

}

EventEncoder::EventEncoder (std::string pFilename, bool pInjectingRaw)
{
    // reset event id
    event_id = 0;

    // init files
    if (pInjectingRaw) this->init_files(pFilename, "none");
    else this->init_files("none", pFilename);

}

void EventEncoder::init_files (std::string pFilenameRaw, std::string pFilenameHlt)
{
    // read raw file
    if (pFilenameRaw.compare("none") != 0) is_raw_present = true;
    else is_raw_present = false;
    std::cout << "RAW present: " << is_raw_present << std::endl;
    if (is_raw_present) {
        file_raw = new TFile (pFilenameRaw.c_str() );
        reader_raw = new TTreeReader ("BRIL_IT_Analysis/Digis", file_raw);
        trv_barrel_raw = new TTreeReaderArray<bool> (*reader_raw, "barrel");
        trv_module_raw = new TTreeReaderArray<uint32_t> (*reader_raw, "module");
        trv_ringlayer_raw = new TTreeReaderArray<uint32_t> (*reader_raw, "ringlayer");
        trv_diskladder_raw = new TTreeReaderArray<uint32_t> (*reader_raw, "diskladder");
        trv_adc_raw = new TTreeReaderArray<uint32_t> (*reader_raw, "adc");
        trv_col_raw = new TTreeReaderArray<uint32_t> (*reader_raw, "column");
        trv_row_raw = new TTreeReaderArray<uint32_t> (*reader_raw, "row");
    }

    // read hlt file
    if (pFilenameHlt.compare("none") != 0) is_hlt_present = true;
    else is_hlt_present = false;
    std::cout << "HLT present: " << is_hlt_present << std::endl;
    if (is_hlt_present) {
        file_hlt = new TFile (pFilenameHlt.c_str() );
        reader_hlt = new TTreeReader ("BRIL_IT_Analysis/Digis", file_hlt);
        trv_barrel_hlt = new TTreeReaderArray<bool> (*reader_hlt, "barrel");
        trv_module_hlt = new TTreeReaderArray<uint32_t> (*reader_hlt, "module");
        trv_ringlayer_hlt = new TTreeReaderArray<uint32_t> (*reader_hlt, "ringlayer");
        trv_diskladder_hlt = new TTreeReaderArray<uint32_t> (*reader_hlt, "diskladder");
        trv_clusters_hlt = new TTreeReaderArray<std::vector<int>> (*reader_hlt, "clusters");
    }

    // make sure
    if(!is_raw_present && !is_hlt_present) {
        std::cout<<"Error no raw or hlt files specified"<<std::endl;
        exit(1);
    }
}

EncodedEvent EventEncoder::get_next_event()
{
    // create empty encoded event
    EncodedEvent encoded_event;
    encoded_event.set_raw_present(is_raw_present);
    encoded_event.set_hlt_present(is_hlt_present);

    // check that we have some data
    bool raw_success = true;
    if (is_raw_present) raw_success = reader_raw->Next();
    bool hlt_success = true;
    if (is_hlt_present) hlt_success = reader_hlt->Next();

    if (!(raw_success) || !(hlt_success) ) {
        encoded_event.set_empty(true);
        return encoded_event;
    }

    // will be used in either case
    // Read all data for this event and construct
    // 2D matrices of pixel ADCs, key == chip identifier
    std::map<ChipIdentifier, IntMatrix> module_matrices;
    std::map<ChipIdentifier, IntMatrix> chip_matrices;

    // QCore objects per module, key == chip identifier
    std::map<ChipIdentifier, std::vector<QCore>> qcores;
    std::map<ChipIdentifier, std::vector<uint16_t>> streams;
    //let's use a seperate instance of the encoder for each event, just to be sure
    Encoder enc;

    // !!! RAW DIGIS LOOP
    if (is_raw_present) {
        //
        std::cout << "!!! Starting Raw digis read loop" << std::endl;

        // iteration
        uint32_t ientry = 0;

        // module loop within event
        // this has one entry for each hit (row, col, adc) touple
        for ( auto imod : *trv_module_raw)
        {
            bool barrel = trv_barrel_raw->At (ientry);
            uint32_t diskladder = trv_diskladder_raw->At (ientry);
            uint32_t ringlayer = trv_ringlayer_raw->At (ientry);
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
            uint32_t row = trv_row_raw->At (ientry);
            uint32_t col = trv_col_raw->At (ientry);
            uint32_t adc = trv_adc_raw->At (ientry);

            //convert to module address (different row and column numbers because 50x50)
            //module_matrices.at (tmp_id).convertPitch_andFill (row, col, adc);
            module_matrices.at (tmp_id).fill (row, col, adc);
            ientry++;
            //if(module_matrices.size() > 5) break;
        }//end module loop

        std::cout << "\t\tFinished reading raw data for event from the tree; found " << module_matrices.size() << " modules for TEPX" << std::endl;

    }

    // !!! HLT CLUSTER LOOP
    if (is_hlt_present) {
        // defining temp variables
        std::map<ChipIdentifier, std::vector<SimpleCluster>> module_clusters;
        std::map<ChipIdentifier, std::vector<SimpleCluster>> chip_clusters;

        //
        std::cout << "!!! Starting HLT cluster read loop" << std::endl;

        // iteration
        uint32_t ientry = 0;

        // module loop within event
        // this has one entry for each hit (row, col, adc) touple
        for ( auto imod : *trv_module_hlt)
        {
            bool barrel = trv_barrel_hlt->At (ientry);
            uint32_t diskladder = trv_diskladder_hlt->At (ientry);
            uint32_t ringlayer = trv_ringlayer_hlt->At (ientry);
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
            if (!is_raw_present) {
                auto matrices_iterator = module_matrices.find (tmp_id);
                if (matrices_iterator == std::end (module_matrices) ) // not found
                {
                    //insert an empty IntMatrix
                    IntMatrix tmp_matrix (nrows_module, ncols_module);
                    //module_matrices[tmp_id] = tmp_matrix;
                    module_matrices.emplace (tmp_id, tmp_matrix);
                }
                //in all other cases the Intmatrix for that module exists already
            }

            //get the row, col and ADC (attention, sensor row and column address) for the current module
            std::vector<int> hit_vec = trv_clusters_hlt->At(ientry);
            std::map<ChipIdentifier, std::vector<SimpleCluster>>::iterator it = module_clusters.find(tmp_id);
            if(it != module_clusters.end()) {
                it->second.push_back(SimpleCluster(nrows_module,ncols_module,hit_vec));
            } else {
                std::vector<SimpleCluster> tmp;
                tmp.push_back(SimpleCluster(nrows_module,ncols_module,hit_vec));
                module_clusters[tmp_id] = tmp;
            }
            // in case still need to get raw hits
            if (!is_raw_present) {
                for(auto hit : hit_vec) {
                    //get the row, col and ADC (attention, sensor row and column address) for the current module
                    uint32_t row = (hit >> 16) & 0xffff;
                    uint32_t col = (hit >> 0) & 0xffff;
                    uint32_t adc = 1;

                    //store
                    module_matrices.at (tmp_id).fill (row, col, adc);
                }
            }

            ientry++;

        }//end module loop

        // now chip clusters
        for (auto current_module : module_clusters)
        {
            std::vector<SimpleCluster> current_module_clusters = current_module.second;
            for (auto module_cluster : current_module_clusters) {
                std::map<int, SimpleCluster> clusters_per_chip = module_cluster.GetClustersPerChip();
                for(auto cluster : clusters_per_chip) {
                    ChipIdentifier chipId (current_module.first.mdisk, current_module.first.mring, current_module.first.mmodule, cluster.first);
                    std::map<ChipIdentifier, std::vector<SimpleCluster>>::iterator it = chip_clusters.find(chipId);
                    if(it != chip_clusters.end()) {
                        it->second.push_back(cluster.second);
                    } else {
                        std::vector<SimpleCluster> tmp;
                        tmp.push_back(cluster.second);
                        chip_clusters[chipId] = tmp;
                    }
                }
            }
        }
        encoded_event.set_chip_clusters(chip_clusters);

        std::cout << "\tFinished processing clusters" << std::endl;

    }

    // now common (process hits)
    {
        //
        std::cout << "!!! Starting stream encoding" << std::endl;

        //do all of the below only for the first event
        if (event_id == 0)
        {
            //for each DTC, encode the module coordinates (disk,ring,moduleID, # of chips) in a vector<uint16_t>, determine the size of that vector and create a file header for each DTC: 1st word: data format version, 2nd word, size of the module list, then: 1 word per module;
            std::map<uint32_t, std::vector<uint16_t>> dtc_map;

            for (auto module : module_matrices)
                dtc_map[module.first.mdtc].push_back (module.first.encode<uint16_t>() );

            for (auto& dtc : dtc_map)
            {
                std::cout << "\t\tDTC " << dtc.first << " has " << dtc.second.size() << " Modules connected" << std::endl;
                uint16_t header_version = 1;
                dtc.second.insert (dtc.second.begin(), dtc.second.size() );
                dtc.second.insert (dtc.second.begin(), header_version );
                std::cout << "\t\tNow the size is " << dtc.second.size() << " and the header version is " << dtc.second.at (0) << " and the header (module list) size is " << dtc.second.at (1) << std::endl;
            }
        }

        //now split the tmp_matrix  for each module in 4 chip matrices, construct the Chip identifier object and insert into the matrices map
        for (auto matrix : module_matrices)
        {
            for (uint32_t chip = 0; chip < 4; chip++)
            {
                ChipIdentifier chipId (matrix.first.mdisk, matrix.first.mring, matrix.first.mmodule, chip);
                IntMatrix tmp_matrix = matrix.second.submatrix (chip);
                if ( tmp_matrix.hits().size() > 0 ) chip_matrices.emplace (chipId, tmp_matrix);
            }
        }
        encoded_event.set_chip_matrices(chip_matrices);

        std::cout << "\tFinished picking apart modules; converted " << module_matrices.size() << " modules for TEPX to " << chip_matrices.size() << " chips" << std::endl;

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

        std::cout << "\tFinished encoding streams" << std::endl;
    }

    // increment
    event_id++;

    // return
    return encoded_event;
}