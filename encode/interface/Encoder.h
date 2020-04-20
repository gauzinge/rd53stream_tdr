#ifndef ENCODER_H
#define ENCODER_H
#include <vector>
#include <stdint.h>
#include <assert.h>
#include <util/IntMatrix.h>
using namespace std;

class Encoder;

class QCore
{

  public:
    QCore (int event_in, int module_in, int chip_in, uint32_t ccol_in, uint32_t qcrow_in, bool isneighbour_in, bool islast_in, vector<ADC> adcs_in, Encoder* encoder_in);
    vector<ADC> adcs;
    vector<bool> hitmap;
    vector<bool> encoded_hitmap;
    bool islast;
    bool isneighbour;
    uint32_t ccol; //range 1 to 54 for 54 core columns
    uint32_t qcrow; //quarter core row address in units of qcrow factor rows
    //uint32_t core_row; //core row
    //uint32_t quarter_core_row;// quarter core row within the core, range 0 to 3
    //uint8_t qcrow; //encoded qcrow value: 6bits core_row + 2 bits quarter core in that core
    int event;
    int module;
    int chip;

    vector<bool> binary_tots () const;
    vector<bool> row (int row_index) const;
    void print() const;

  private:
    vector<bool> adcs_to_hitmap (vector<ADC> adcs);

};


// Performs the encoding of module ADC data
// Data comes in as a matrix of ADC values
class Encoder
{
  public:
    Encoder()
    {
        col_factor = 8;
        row_factor = 2;
    };
    vector<bool> encode_matrix (IntMatrix& matrix);
    vector<bool> encode_qcore (vector<uint32_t> qcore);
    vector<bool> encode_hitmap (vector<bool> hitmap);
    vector<QCore> qcores (IntMatrix&, int event, int module, int chip);
    int find_last_qrow (IntMatrix& matrix, uint32_t ccol);

  protected:
    uint32_t col_factor;
    uint32_t row_factor;

};


#endif // ENCODER_H
