#include <TFile.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <util/IntMatrix.h>
#include <util/ChipIdentifier.h>

#ifndef SIMPLECLUSTER_H__
#define SIMPLECLUSTER_H__
class SimpleCluster
{
    public:
        SimpleCluster(uint32_t pNrows, uint32_t pNcols, std::vector<int> pHits) { nrows = pNrows; ncols = pNcols; hits = pHits; }
        std::vector<int> GetHits() { return hits; }
        std::map<int, std::vector<int>> GetHitsPerChip() {
            std::map<int, std::vector<int>> hits_per_chip;
            for (auto hit : hits) {
                uint32_t row = (hit >> 16) & 0xffff;
                uint32_t col = (hit >> 0) & 0xffff;
                if(row < this->nrows && col < this->ncols) {
                    if (row < this->nrows/2) {
                        if(col < this->ncols/2) {
                            uint32_t address = (col << 0) | (row << 16);
                            hits_per_chip[0].push_back(address);
                        } else {
                            uint32_t address = ((col-this->ncols/2) << 0) | (row << 16);
                            hits_per_chip[1].push_back(address);
                        }
                    } else {
                        if(col < this->ncols/2) {
                            uint32_t address = (col << 0) | ((row-this->nrows/2) << 16);
                            hits_per_chip[2].push_back(address);
                        } else {
                            uint32_t address = ((col-this->ncols/2) << 0) | ((row-this->nrows/2) << 16);
                            hits_per_chip[3].push_back(address);
                        }
                    }
                }
            }
            return hits_per_chip;
        }
        std::map<int, SimpleCluster> GetClustersPerChip() {
            std::map<int, std::vector<int>> hits_per_chip = this->GetHitsPerChip();
            std::map<int, SimpleCluster> clusters_per_chip;
            //std::cout<< "Debug:\n";
            //std::cout << "\tAll hits: ";
            //for(auto hit : hits) std::cout << "(col " << ((hit >> 0) & 0xffff) << ", row " << ((hit >> 16) & 0xffff) << ") ";
            //std::cout << std::endl;
            for(auto hits_vec : hits_per_chip) {
                //std::cout<< "\tChip: " << hits_vec.first << ", Hits: ";
                //for(auto hit : hits_vec.second) std::cout << "(col " << ((hit >> 0) & 0xffff) << ", row " << ((hit >> 16) & 0xffff) << ") ";
                //std::cout << std::endl;
                clusters_per_chip.emplace(hits_vec.first, SimpleCluster(this->nrows/2, this->ncols/2, hits_vec.second));
            }
            return clusters_per_chip;
        }

    private:
        uint32_t nrows;
        uint32_t ncols;
        std::vector<int> hits;
};
#endif

#ifndef ENCODEDEVENT_H__
#define ENCODEDEVENT_H__
class EncodedEvent
{

  public:
    EncodedEvent ();
    //EncodedEvent (const EncodedEvent &ev);
    void set_empty(bool pValue) { is_empty_var = pValue; }
    bool is_empty() { return is_empty_var; }
    void set_chip_clusters(std::map<ChipIdentifier, std::vector<SimpleCluster>> pChip_clusters) { chip_clusters = pChip_clusters; }
    void set_chip_matrices(std::map<ChipIdentifier, IntMatrix> pChip_matrices) { chip_matrices = pChip_matrices; chip_iterator = chip_matrices.begin(); }
    std::vector<uint16_t> get_stream(std::pair<ChipIdentifier, IntMatrix> chip);
    std::vector<uint16_t> get_stream_by_id(ChipIdentifier identifier);
    std::pair<ChipIdentifier, std::vector<uint16_t>> get_next_chip();
    std::vector<std::pair<uint32_t,uint32_t>> get_chip_hits(ChipIdentifier identifier) { return chip_matrices[identifier].hits(); }
    uint32_t get_chip_nclusters(ChipIdentifier identifier) { return chip_clusters[identifier].size(); }
    std::vector<SimpleCluster> get_chip_clusters(ChipIdentifier identifier) { return chip_clusters[identifier]; }
    std::string chip_str(ChipIdentifier identifier);
    void print ();

    // setters getters
    void set_event_id(uint32_t value) { event_id = value; }
    uint32_t get_event_id() { return event_id; }
    void set_event_id_raw(uint32_t value) { event_id_raw = value; }
    uint32_t get_event_id_raw() { return event_id_raw; }

  private:
    bool is_empty_var;
    uint32_t event_id;
    uint32_t event_id_raw;
    std::map<ChipIdentifier, std::vector<SimpleCluster>> chip_clusters;
    std::map<ChipIdentifier, IntMatrix> chip_matrices;
    std::map<ChipIdentifier, IntMatrix>::iterator chip_iterator;

};
#endif

#ifndef EVENTENCODER_H__
#define EVENTENCODER_H__
class EventEncoder
{

  public:
    EventEncoder (std::string pFilename);
    void init_file(std::string pFilename);
    void skip_events(uint32_t pN);
    EncodedEvent get_next_event();

  private:

    // common file file
    TFile* file;
    TTreeReader* reader;
    TTreeReaderValue<uint32_t>* trv_event_id;
    TTreeReaderValue<uint32_t>* trv_side;
    TTreeReaderValue<uint32_t>* trv_module;
    TTreeReaderValue<uint32_t>* trv_ring;
    TTreeReaderValue<uint32_t>* trv_disk;
    TTreeReaderArray<uint32_t>* trv_adc;
    TTreeReaderArray<uint32_t>* trv_col;
    TTreeReaderArray<uint32_t>* trv_row;
    TTreeReaderArray<std::vector<int>>* trv_clusters;

    // common
    const uint32_t nrows_module = 1344;
    const uint32_t ncols_module = 432;
    uint32_t event_id;

};
#endif
