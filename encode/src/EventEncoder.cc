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

/*EncodedEvent::EncodedEvent (const EncodedEvent &ev)
{
    is_empty_var = bool(ev.is_empty_var);
    chip_matrices = std::map<ChipIdentifier, IntMatrix> (ev.chip_matrices);
    streams = std::map<ChipIdentifier, std::vector<uint16_t>> (ev.streams);
    streams_iterator = std::map<ChipIdentifier, std::vector<uint16_t>>::iterator (ev.streams_iterator);
}*/

std::pair<ChipIdentifier, std::vector<uint16_t>> EncodedEvent::get_next_chip()
{
    std::pair<ChipIdentifier, IntMatrix> chip;
    std::pair<ChipIdentifier, std::vector<uint16_t>> ret;
    if (chip_iterator != chip_matrices.end()) {
        chip = *chip_iterator;
        chip_iterator++;
        if (chip.second.hits().size() > 0) {
            ret.first = chip.first;
            ret.second = get_stream(chip);
            return ret;
        }
        else
            return this->get_next_chip();
    } else {
        // return empty
        return ret;
    }
}

std::vector<uint16_t> EncodedEvent::get_stream(std::pair<ChipIdentifier, IntMatrix> chip)
{
    //let's use a seperate instance of the encoder for each event, just to be sure
    Encoder enc;

    //pick apart each chip int matrix in QCores
    std::vector<QCore> qcores = enc.qcores (chip.second, chip.first.mmodule, chip.first.mchip);

    //create the actual stream for this chip
    std::stringstream ss;
    std::vector<bool> tmp =  Serializer::serializeChip (qcores, event_id, chip.first.mchip, false, true, ss);
    std::vector<uint16_t> binary_vec = Serializer::toVec<uint16_t> (tmp);

    // return
    return binary_vec;

}

std::vector<uint16_t> EncodedEvent::get_stream_by_id(ChipIdentifier identifier)
{
    // get mq54is
    IntMatrix chip_matrix = chip_matrices[identifier];
    // do
    return get_stream(std::make_pair(identifier, chip_matrix));

}

std::string EncodedEvent::chip_str(ChipIdentifier identifier) {
    IntMatrix chip_matrix = chip_matrices[identifier];
    std::vector<uint16_t> stream = get_stream(std::make_pair(identifier, chip_matrix));
    std::ostringstream out;
    out << "Side: " << identifier.mside << ", Disk: " << identifier.mdisk << ", Ring: " << identifier.mring << ", Module: " << identifier.mmodule << ", Chip: " << identifier.mchip << std::endl;
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
    std::vector<QCore> qcores = enc.qcores (chip_matrix, identifier.mmodule, identifier.mchip);
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
    for(auto chip_item : chip_matrices) {
        ChipIdentifier identifier = chip_item.first;
        std::cout << chip_str(identifier);
    }
}

EventEncoder::EventEncoder (std::string pFilename)
{
    // reset event id
    event_id = 0;

    // init files
    this->init_file(pFilename);

}

void EventEncoder::init_file (std::string pFilename)
{
    // read file
    file = new TFile (pFilename.c_str() );
    reader= new TTreeReader ("BRIL_IT_Analysis/Digis", file);
    trv_event_id= new TTreeReaderValue<uint32_t> (*reader, "event");
    trv_side = new TTreeReaderValue<uint32_t> (*reader, "side");
    trv_module = new TTreeReaderValue<uint32_t> (*reader, "module");
    trv_ring = new TTreeReaderValue<uint32_t> (*reader, "ring");
    trv_disk = new TTreeReaderValue<uint32_t> (*reader, "disk");
    trv_adc = new TTreeReaderArray<uint32_t> (*reader, "adc");
    trv_col = new TTreeReaderArray<uint32_t> (*reader, "column");
    trv_row= new TTreeReaderArray<uint32_t> (*reader, "row");
    trv_clusters = new TTreeReaderArray<std::vector<int>> (*reader, "clusters");

}

void EventEncoder::skip_events(uint32_t pN) {
    for (uint32_t i = 0; i < pN; i++) reader->Next();
}

EncodedEvent EventEncoder::get_next_event()
{
    // create empty encoded event
    EncodedEvent encoded_event;
    encoded_event.set_event_id(event_id);

    // will be used in either case
    // Read all data for this event and construct
    // 2D matrices of pixel ADCs, key == chip identifier
    std::map<ChipIdentifier, IntMatrix> module_matrices;
    std::map<ChipIdentifier, IntMatrix> chip_matrices;
    std::map<ChipIdentifier, std::vector<SimpleCluster>> module_clusters;
    std::map<ChipIdentifier, std::vector<SimpleCluster>> chip_clusters;

    //
    std::cout << "!!! Starting Raw digis read loop" << std::endl;
    // !!! DATA READ LOOP
    uint32_t module_counter = 0;
    bool isDone = false;
    while (true) {

        // check that we have some data
        bool raw_success = reader->Next();
        if (!(raw_success)) {
            if (module_counter == 0) encoded_event.set_empty(true);
            break;
        }
        // check event id raw
        //std::cout<< "Module counter: " << module_counter << ", Event id raw: " << encoded_event.get_event_id_raw() << ", From file: " << **trv_event_id << std::endl;
        if (module_counter == 0) encoded_event.set_event_id_raw(**trv_event_id);
        else if (encoded_event.get_event_id_raw() != **trv_event_id) { // || module_counter == 100) {
            isDone = true;
            reader->SetEntry(reader->GetCurrentEntry()-1);
        }
        module_counter++;

        // module loop within event
        uint32_t side = *trv_side->Get();
        uint32_t disk = *trv_disk->Get();
        uint32_t ring = *trv_ring->Get();
        uint32_t module = *trv_module->Get();

        /*if(side*disk != 9 || ring != 3 || module != 20) {
            if (isDone) break;
            else continue;
        }
        else std::cout << "Got it" << std::endl;*/

        //generate a temporary chip identifier with fake chip id 99 since for the moment we just care for the module
        ChipIdentifier tmp_id (side, disk, ring, module, 99);
        //check if that module is already in our map
        auto matrices_iterator = module_matrices.find (tmp_id);
        if (matrices_iterator == std::end (module_matrices) ) // not found
        {
            //insert an empty IntMatrix
            IntMatrix tmp_matrix (nrows_module, ncols_module);
            //module_matrices[tmp_id] = tmp_matrix;
            module_matrices.emplace (tmp_id, tmp_matrix);
        } else {
            std::cout << "Warning! Module already exists! " << std::endl;
            tmp_id.print();
        }

        //in all other cases the Intmatrix for that module exists already
        uint32_t ientry = 0;
        while (ientry < trv_row->GetSize()) {
            //get the row, col and ADC (attention, sensor row and column address) for the current module
            uint32_t row = trv_row->At (ientry);
            uint32_t col = trv_col->At (ientry);
            uint32_t adc = trv_adc->At (ientry);

            //convert to module address (different row and column numbers because 50x50)
            //module_matrices.at (tmp_id).convertPitch_andFill (row, col, adc);
            module_matrices.at (tmp_id).fill (row, col, adc);
            // increment
            ientry++;
        }

        // module clusters
        ientry = 0;
        while(ientry < trv_clusters->GetSize()) {
            std::vector<int> cluster_hit_vec = trv_clusters->At(ientry);
            std::map<ChipIdentifier, std::vector<SimpleCluster>>::iterator it = module_clusters.find(tmp_id);
            if(it != module_clusters.end()) {
                it->second.push_back(SimpleCluster(nrows_module,ncols_module,cluster_hit_vec));
            } else {
                std::vector<SimpleCluster> tmp;
                tmp.push_back(SimpleCluster(nrows_module,ncols_module,cluster_hit_vec));
                module_clusters[tmp_id] = tmp;
            }
            ientry++;
        }

        if (isDone) break;
    }
    std::cout << "\t\tFinished reading data for event from the tree; found " << module_matrices.size() << " modules for TEPX" << std::endl;

    // now process
    {

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

        //
        std::cout << "!!! Starting module splitting" << std::endl;

        //now split the tmp_matrix  for each module in 4 chip matrices, construct the Chip identifier object and insert into the matrices map
        for (auto matrix : module_matrices)
        {
            for (uint32_t chip = 0; chip < 4; chip++)
            {
                ChipIdentifier chipId (matrix.first.mside, matrix.first.mdisk, matrix.first.mring, matrix.first.mmodule, chip);
                IntMatrix tmp_matrix = matrix.second.submatrix (chip);
                if ( tmp_matrix.hits().size() > 0 ) chip_matrices.emplace (chipId, tmp_matrix);
            }
        }
        encoded_event.set_chip_matrices(chip_matrices);

        // now chip clusters
        for (auto current_module : module_clusters)
        {
            std::vector<SimpleCluster> current_module_clusters = current_module.second;
            for (auto module_cluster : current_module_clusters) {
                std::map<int, SimpleCluster> clusters_per_chip = module_cluster.GetClustersPerChip();
                for(auto cluster : clusters_per_chip) {
                    ChipIdentifier chipId (current_module.first.mside, current_module.first.mdisk, current_module.first.mring, current_module.first.mmodule, cluster.first);
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

        std::cout << "\tFinished picking apart modules; converted " << module_matrices.size() << " modules for TEPX to " << chip_matrices.size() << " chips" << std::endl;

    }

    // increment
    event_id++;

    // return
    return encoded_event;
}