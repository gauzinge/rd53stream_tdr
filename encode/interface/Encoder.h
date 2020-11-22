#ifndef ENCODER_H
#define ENCODER_H
#include <vector>
#include <stdint.h>
#include <assert.h>
#include <util/IntMatrix.h>
#include <encode/interface/QCore.h>

using namespace std;

// Performs the encoding of module ADC data
// Data comes in as a matrix of ADC values
class QCore;

class Encoder
{
  public:
    Encoder()
    {
        sensor_col_factor = 4;
        sensor_row_factor = 4;
        chip_col_factor = 8;
        chip_row_factor = 2;
    };
    vector<bool> encode_matrix (IntMatrix& matrix);
    vector<bool> encode_qcore (vector<uint32_t> qcore);
    //vector<bool> encode_hitmap (vector<bool> hitmap);
    vector<QCore> qcores (IntMatrix&, int module, int chip, std::ostream& stream = std::cout);
    int find_last_qrow (IntMatrix& matrix, uint32_t ccol);

  protected:
    uint32_t sensor_col_factor;
    uint32_t sensor_row_factor;
    uint32_t chip_col_factor;
    uint32_t chip_row_factor;

};


#endif // ENCODER_H
