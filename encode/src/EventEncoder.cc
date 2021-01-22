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

EncodedEvent::~EncodedEvent ()
{
    std::cout << "Destructor called" << std::endl;
    chip_clusters.clear();
    chip_was_split.clear();
    chip_matrices.clear();
    chip_iterator = chip_matrices.end();
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
    std::vector<bool> tmp =  Serializer::serializeChip (qcores, event_id_raw, chip.first.mchip, false, true, ss);
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
    out << "Event id raw: " << get_event_id_raw();
    out << " Side: " << identifier.mside << ", Disk: " << identifier.mdisk << ", Ring: " << identifier.mring << ", Module: " << identifier.mmodule << ", Chip: " << identifier.mchip << std::endl;
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
    for(auto hit : chip_matrix.hits()) out << "(col " << ((hit.first >> 0) & 0xffff) << ", row " << ((hit.first >> 16) & 0xffff) << ", adc " << hit.second << ") ";
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

std::string EncodedEvent::event_str() {
    std::ostringstream out;
    out << "Event ID raw: " << get_event_id_raw() << std::endl;
    for(auto chip_item : chip_matrices) {
        ChipIdentifier identifier = chip_item.first;
        out << chip_str(identifier);
    }
    return out.str();
}

void EncodedEvent::print() {
    std::cout << event_str();
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

EncodedEvent* EventEncoder::get_single_module_event(uint32_t pEventId, ChipIdentifier pModuleId)
{
    // first find needed event
    reader->Restart();
    bool success = reader->Next();
    while(success) {
        if(**trv_event_id == pEventId && **trv_side == pModuleId.mside && **trv_disk == pModuleId.mdisk && **trv_ring == pModuleId.mring && **trv_module == pModuleId.mmodule) break;
        else success = reader->Next();
    }
    reader->SetEntry(reader->GetCurrentEntry()-1);
    // now get it
    return get_next_event(true);
}

EncodedEvent* EventEncoder::get_next_event(bool pDoSingle = false)
{
    // create empty encoded event
    EncodedEvent *encoded_event = new EncodedEvent();

    // will be used in either case
    // Read all data for this event and construct
    // 2D matrices of pixel ADCs, key == chip identifier
    std::map<ChipIdentifier, IntMatrix> module_matrices;
    std::map<ChipIdentifier, IntMatrix> chip_matrices;
    std::map<ChipIdentifier, std::vector<SimpleCluster>> module_clusters;
    std::map<ChipIdentifier, std::vector<SimpleCluster>> chip_clusters;
    std::map<ChipIdentifier, bool> chip_was_split;

    // make sure everything is empty
    if (!(module_matrices.empty() && chip_matrices.empty() && module_clusters.empty() && chip_clusters.empty() && chip_was_split.empty())) {
        std::cout << "Error, some of the just created maps is not empty" << std::endl;
        exit(1);
    }

    //
    std::cout << "!!! Starting Raw digis read loop" << std::endl;
    // !!! DATA READ LOOP
    uint32_t module_counter = 0;
    bool isDone = false;
    while (true) {

        // check that we have some data
        bool raw_success = reader->Next();
        if (!(raw_success)) {
            std::cout << "End of file reached" << std::endl;
            if (module_counter == 0) encoded_event->set_empty(true);
            break;
        }
        // check event id raw
        if (module_counter == 0) encoded_event->set_event_id_raw(**trv_event_id);
        else if (encoded_event->get_event_id_raw() != **trv_event_id) {
            //std::cout << "\ttoo far " << encoded_event.get_event_id_raw() << " whereas got: " << **trv_event_id << std::endl;
            isDone = true;
            reader->SetEntry(reader->GetCurrentEntry()-1);
        }

        if (!isDone) {
            module_counter++;

            // module loop within event
            uint32_t side = *trv_side->Get();
            uint32_t disk = *trv_disk->Get();
            uint32_t ring = *trv_ring->Get();
            uint32_t module = *trv_module->Get();

            //if((**trv_event_id == 48 || **trv_event_id == 54) && (side == 2 && disk == 11 && ring == 4 && module == 32)) {
            //if((**trv_event_id > 40 && **trv_event_id < 60)) {

            //generate a temporary chip identifier with fake chip id 99 since for the moment we just care for the module
            ChipIdentifier tmp_id_module(side, disk, ring, module, 99);
            auto matrices_iterator = module_matrices.find (tmp_id_module);
            if (matrices_iterator == std::end (module_matrices) ) // not found
            {
                //insert an empty IntMatrix
                IntMatrix tmp_matrix (nrows_module, ncols_module);
                //module_matrices[tmp_id] = tmp_matrix;
                module_matrices.emplace (tmp_id_module, tmp_matrix);
            } else {
                std::cout << "Error! Module already exists! Event id: " << **trv_event_id << " " << encoded_event->get_event_id_raw() << std::endl;
                tmp_id_module.print();
                exit(1);
            }

            ChipIdentifier tmp_id_chip[4];
            for(uint32_t chip_id = 0; chip_id < 4; chip_id++) {
                tmp_id_chip[chip_id] = ChipIdentifier(side, disk, ring, module, chip_id);
                //check if that module is already in our map
                auto matrices_iterator = chip_matrices.find (tmp_id_chip[chip_id]);
                if (matrices_iterator == std::end (chip_matrices) ) // not found
                {
                    //insert an empty IntMatrix
                    IntMatrix tmp_matrix (nrows_module/2, ncols_module/2);
                    chip_matrices.emplace (tmp_id_chip[chip_id], tmp_matrix);
                } else {
                    std::cout << "Error! Chip already exists! " << std::endl;
                    tmp_id_chip[chip_id].print();
                    exit(1);
                }
            }

            //in all other cases the Intmatrix for that module exists already
            uint32_t ientry = 0;
            while (ientry < trv_row->GetSize()) {
                //get the row, col and ADC (attention, sensor row and column address) for the current module
                uint32_t row = trv_row->At (ientry);
                uint32_t col = trv_col->At (ientry);
                uint32_t adc = trv_adc->At (ientry);

                uint32_t chip_id = 0;
                if (row < nrows_module/2) {
                    if (col < ncols_module/2) {
                        chip_id = 0;
                    } else if (col >= ncols_module/2 && col < ncols_module) {
                        chip_id = 1;
                    } else chip_id = 4;
                } else if (row >= nrows_module/2 && row < nrows_module) {
                    if (col < ncols_module/2) {
                        chip_id = 2;
                    }
                    else if (col >= ncols_module/2 && col < ncols_module) {
                        chip_id = 3;
                    } else chip_id = 4;
                } else chip_id = 5;

                // append
                module_matrices.at (tmp_id_module).fill (row, col, adc);
                if(chip_id < 4) chip_matrices.at (tmp_id_chip[chip_id]).fill (row % (nrows_module/2), col % (ncols_module/2), adc);

                // increment
                ientry++;
            }

            // module clusters
            ientry = 0;
            while(ientry < trv_clusters->GetSize()) {
                std::vector<int> cluster_hit_vec = trv_clusters->At(ientry);
                std::map<ChipIdentifier, std::vector<SimpleCluster>>::iterator it = module_clusters.find(tmp_id_module);
                if(it != module_clusters.end()) {
                    it->second.push_back(SimpleCluster(nrows_module,ncols_module,cluster_hit_vec));
                } else {
                    std::vector<SimpleCluster> tmp;
                    tmp.push_back(SimpleCluster(nrows_module,ncols_module,cluster_hit_vec));
                    module_clusters[tmp_id_module] = tmp;
                }
                ientry++;
            }
            //}
        } else break;

        if (pDoSingle) break;
    }
    encoded_event->set_chip_matrices(chip_matrices);
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

        // now chip clusters
        for (auto current_module : module_clusters)
        {
            std::vector<SimpleCluster> current_module_clusters = current_module.second;
            for (auto module_cluster : current_module_clusters) {
                std::map<int, std::vector<SimpleCluster>> clusters_per_chip = module_cluster.GetClustersPerChip();
                for(auto clusters : clusters_per_chip) {
                    ChipIdentifier chipId (current_module.first.mside, current_module.first.mdisk, current_module.first.mring, current_module.first.mmodule, clusters.first);
                    std::map<ChipIdentifier, std::vector<SimpleCluster>>::iterator it = chip_clusters.find(chipId);
                    if(it != chip_clusters.end()) {
                        if (clusters.second.size() > 1) chip_was_split[chipId] = true; // if already exists set only true
                        it->second.insert(it->second.end(), clusters.second.begin(), clusters.second.end());
                    } else {
                        chip_was_split[chipId] = (clusters.second.size() > 1);
                        chip_clusters[chipId] = clusters.second;
                    }
                }
            }
        }
        encoded_event->set_chip_clusters(chip_clusters);
        encoded_event->set_chip_split(chip_was_split);

        std::cout << "\tFinished picking apart modules; converted " << module_matrices.size() << " modules for TEPX to " << chip_matrices.size() << " chips" << std::endl;

    }

    // increment
    event_id++;

    // return
    //if ((encoded_event.get_event_id_raw() > 40 && encoded_event.get_event_id_raw() < 60) || encoded_event.is_empty()) return encoded_event;
    //else return get_next_event();
    return encoded_event;
}