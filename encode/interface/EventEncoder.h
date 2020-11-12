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
    void set_chip_matrices(std::map<ChipIdentifier, IntMatrix> pChip_matrices) { chip_matrices = pChip_matrices; }
    void set_streams(std::map<ChipIdentifier, std::vector<uint16_t>> pStreams) { streams = pStreams; streams_iterator = streams.begin(); }
    std::pair<ChipIdentifier, std::vector<uint16_t>> get_next_chip();
    std::vector<std::pair<uint32_t,uint32_t>> get_chip_hits(ChipIdentifier identifier) { return chip_matrices[identifier].hits(); }
    uint32_t get_chip_nclusters(ChipIdentifier identifier) { return chip_clusters[identifier].size(); }
    void print_chip(ChipIdentifier identifier);
    void print ();

  private:
    bool is_empty_var;
    std::map<ChipIdentifier, std::vector<SimpleCluster>> chip_clusters;
    std::map<ChipIdentifier, IntMatrix> chip_matrices;
    std::map<ChipIdentifier, std::vector<uint16_t>> streams;
    std::map<ChipIdentifier, std::vector<uint16_t>>::iterator streams_iterator;

};
#endif

#ifndef EVENTENCODER_H__
#define EVENTENCODER_H__
class EventEncoder
{

  public:
    EventEncoder (std::string pFilename);
    EncodedEvent get_next_event();

  private:
    TFile* file;
    TTreeReader* reader;
    TTreeReaderArray<bool>* trv_barrel;
    TTreeReaderArray<uint32_t>* trv_module;
    //TTreeReaderArray<uint32_t>* trv_adc;
    //TTreeReaderArray<uint32_t>* trv_col;
    //TTreeReaderArray<uint32_t>* trv_row;
    TTreeReaderArray<uint32_t>* trv_ringlayer;
    TTreeReaderArray<uint32_t>* trv_diskladder;
    TTreeReaderArray<std::vector<int>>* trv_clusters;
    const uint32_t nrows_module = 1344;
    const uint32_t ncols_module = 432;
    uint32_t event_id;

};
#endif
