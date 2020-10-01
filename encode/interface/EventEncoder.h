#include <TFile.h>
#include <TTreeReader.h>
#include <TTreeReaderArray.h>
#include <util/IntMatrix.h>
#include <util/ChipIdentifier.h>

#ifndef ENCODEDEVENT_H__
#define ENCODEDEVENT_H__
class EncodedEvent
{

  public:
    EncodedEvent ();
    void set_empty(bool pValue) { is_empty_var = pValue; }
    bool is_empty() { return is_empty_var; }
    void set_chip_matrices(std::map<ChipIdentifier, IntMatrix> pChip_matrices) { chip_matrices = pChip_matrices; }
    void set_streams(std::map<ChipIdentifier, std::vector<uint16_t>> pStreams) { streams = pStreams; }
    void print ();

  private:
    bool is_empty_var;
    std::map<ChipIdentifier, IntMatrix> chip_matrices;
    std::map<ChipIdentifier, std::vector<uint16_t>> streams;

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
    TTreeReaderArray<uint32_t>* trv_adc;
    TTreeReaderArray<uint32_t>* trv_col;
    TTreeReaderArray<uint32_t>* trv_row;
    TTreeReaderArray<uint32_t>* trv_ringlayer;
    TTreeReaderArray<uint32_t>* trv_diskladder;
    const uint32_t nrows_module = 672;
    const uint32_t ncols_module = 864;
    uint32_t event_id;

};
#endif
