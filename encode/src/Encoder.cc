#include <util/Util.h>
#include <encode/interface/Encoder.h>
#include <util/LUT.h>
#include <cmath>
#include <assert.h>
#include <iostream>

using namespace std;

int Encoder::find_last_qrow(IntMatrix & matrix, uint32_t ccol){
    int last_row = -1;
    for(uint32_t r=0; r<matrix.size_row(); r++) {
        for(uint32_t c = ccol * this->col_factor; c<(ccol+1)*this->col_factor; c++){
            if(matrix.data[r][c] > 0) {
                last_row = r;
                break;
            }
        }
    }

    if(last_row < 0){
        return last_row;
    } else {
        return last_row/this->row_factor;
    }
};



/**
 * Encode the 16-bit hitmap of using tree-based encoding
 *
 * The output will be between 4 and 30 bits:
 * [Row OR (1-2 bits)][Row 1 (3-14 bits)][Row 2 (3-14 bits)]
 */
vector<bool> Encoder::encode_hitmap(vector<bool> hitmap) {
    assert(hitmap.size()==16);

    // Separate rows
    vector<bool> row1, row2;
    row1.insert(row1.end(), hitmap.begin(), hitmap.begin()+8);
    row2.insert(row2.end(), hitmap.begin()+8, hitmap.end());
    assert(row1.size()==8 and row2.size()==8);

    // Row OR information
    std::vector<bool> row_or = {0, 0};
    for( auto const bit : row1) {
        if(bit) {
            row_or[0] = 1;
            break;
        }
    }
    for( auto const bit : row2) {
        if(bit) {
            row_or[1] = 1;
            break;
        }
    }

    // Compress the components separately
    // using lookup tables
    vector<bool> row_or_enc = enc2(row_or);
    vector<bool> row1_enc = enc8(row1);
    vector<bool> row2_enc = enc8(row2);

    // Merge together
    vector<bool> encoded_hitmap;
    encoded_hitmap.reserve(row_or_enc.size() + row1_enc.size() + row2_enc.size());
    encoded_hitmap.insert(encoded_hitmap.end(), row_or_enc.begin(), row_or_enc.end());
    encoded_hitmap.insert(encoded_hitmap.end(), row1_enc.begin(), row1_enc.end());
    encoded_hitmap.insert(encoded_hitmap.end(), row2_enc.begin(), row2_enc.end());

    return encoded_hitmap;
};

/**
 * Build QCore objects from a pixel matrix
 */
vector<QCore> Encoder::qcores(IntMatrix & matrix, int event, int module) {
    vector<QCore> qcores;

    // Loop over core columns (ccol)
    uint32_t nccol = floor(matrix.size_col() / this->col_factor);
    for (uint32_t ccol = 0; ccol < nccol ; ccol++){
        int qcrow_prev = -2;
        int last_qcrow = find_last_qrow(matrix, ccol);
        if(last_qcrow<0) {
            continue;
        }

        // Loop over quarter core rows (qcrow)
        for(uint32_t qcrow=0; qcrow<=last_qcrow; qcrow++ ){
            // The native representation of the QCore is
            // a 16-bit vector of ADC values, which are
            // simply copied from the matrix
            vector<ADC> qcore_adcs;
            qcore_adcs.reserve(16);
            bool any_nonzero = false;
            for(uint32_t j = 0; j<this->row_factor; j++) {
                for(uint32_t i = ccol * this->col_factor; i<(ccol+1)*this->col_factor; i++) {
                    uint32_t value = matrix.data[qcrow*this->row_factor+j][i];
                    any_nonzero |= value > 0;
                    qcore_adcs.push_back(value);
                }
            }
            assert(qcore_adcs.size() == 16);

            // Ignore qcores that do not have any hits
            if(!any_nonzero) {
                continue;
            }

            // Meta information about the qcore
            // necessary for stream building
            bool isneighbour = (qcrow_prev + 1 == qcrow);
            qcrow_prev = qcrow;
            bool islast = (qcrow == last_qcrow);
            QCore qcore(event, module, ccol, qcrow, isneighbour, islast, qcore_adcs, this);
            qcores.push_back(qcore);
        }
    }
    return qcores;
}

QCore::QCore(int event_in, int module_in, uint32_t ccol_in, uint32_t qcrow_in, bool isneighbour_in, bool islast_in,vector<ADC> adcs_in, Encoder * encoder_in) :
    event(event_in),
    module(module_in),
    adcs(adcs_in),
    isneighbour(isneighbour_in),
    islast(islast_in),
    qcrow(qcrow_in),
    ccol(ccol_in),
    hitmap(adcs_to_hitmap(adcs)),
    encoded_hitmap(encoder_in->encode_hitmap(this->hitmap))
{
}
vector<bool> QCore::binary_tots () const{
    vector<bool> tots;
    for(auto const & adc: adcs){
        if(adc==0){
            continue;
        }
        vector<bool> adc_binary = adc_to_binary(adc);
        tots.insert(tots.end(), adc_binary.begin(), adc_binary.end());
    }
    return tots;
};
vector<bool> QCore::adcs_to_hitmap(vector<ADC> adcs) {
    assert(adcs.size()==16);

    std::vector<bool> hitmap;
    // Set up uncompressed hitmap
    hitmap.reserve(16);
    for(auto const adc : adcs){
        hitmap.push_back(adc>0);
    }
    assert(hitmap.size()==16);

    return hitmap;
};

vector<bool> QCore::row(int row_index) const {
    if((row_index < 0) or (row_index > 1)){
        throw;
    }
    vector<bool> row;
    row.insert(row.end(), this->hitmap.begin() + 8 * row_index, this->hitmap.begin() + 8 * (row_index+1));
    assert(row.size() == 8);
    return row;
}
// vector<bool> Encoder::encode_matrix(IntMatrix & matrix) {
//     // Number of quarter core columns
//     assert(matrix.size()>0);
//     vector<bool> stream;
//     // TODO: Tag at beginning of module stream
//     vector<bool> tag = {0, 0, 0, 0, 0, 0, 0, 0};
//     assert(tag.size()==8);
//     stream.insert(stream.end(), tag.begin(), tag.end());
//     vector<QCore> qcores = this->qcores(matrix);

//     bool is_new_ccol = true;
//     for(auto const & qcore : qcores) {

//         // Column address
//         if(is_new_ccol) {
//             vector<bool> ccol_addr = {0,0,0,0,0,0}; // Six bits
//             assert(ccol_addr.size()==6);
//             stream.insert(stream.end(), ccol_addr.begin(), ccol_addr.end());
//         }
//         is_new_ccol = qcore.islast;

//         // Auxilliary bits
//         stream.push_back(qcore.isneighbour);
//         stream.push_back(qcore.islast);

//         // Row address
//         if(not qcore.isneighbour) {
//             vector<bool> qrow_addr = {0,0,0,0,0,0,0,0}; // Eight bits
//             assert(qrow_addr.size()==8);
//             stream.insert(stream.end(), qrow_addr.begin(), qrow_addr.end());
//         }
//         // QCore data
//         stream.insert(stream.end(), qcore.encoded_hitmap.begin(), qcore.encoded_hitmap.end());
//         vector<bool> tots = qcore.binary_tots();
//         stream.insert(stream.end(),tots.begin(), tots.end());
//     }

//     return stream;
// }


