#ifndef ENCODER_H
#define ENCODER_H
#include <vector>
#include <stdint.h>
#include <assert.h>
#include <util/IntMatrix.h>
using namespace std;

class Encoder;

class QCore{

    public:
        QCore(int event_in, int module_in, uint32_t ccol_in, uint32_t qcrow_in, bool isneighbour_in, bool islast_in,vector<ADC> adcs_in, Encoder * encoder_in);
        vector<ADC> adcs;
        vector<bool> hitmap;
        vector<bool> encoded_hitmap;
        bool islast;
        bool isneighbour;
        uint32_t ccol;
        uint32_t qcrow;
        int event;
        int module;
        vector<bool> binary_tots () const;
        vector<bool> row(int row_index) const;

    private:
        vector<bool> adcs_to_hitmap(vector<ADC> adcs);

};


// Performs the encoding of module ADC data
// Data comes in as a matrix of ADC values
class Encoder{
    public:
        Encoder(){
            col_factor=4;
            row_factor=4;
        };
        vector<bool> encode_matrix(IntMatrix & matrix);
        vector<bool> encode_qcore(vector<uint32_t> qcore);
        vector<bool> encode_hitmap(vector<bool> hitmap);
        vector<QCore> qcores(IntMatrix&, int event, int module);
        int find_last_qrow(IntMatrix & matrix, uint32_t ccol);

    protected:
        uint32_t col_factor;
        uint32_t row_factor;

};


#endif // ENCODER_H
