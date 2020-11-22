#ifndef QCORE_H__
#define QCORE_H__

#include <iostream>
#include <vector>
#include <assert.h>
#include <util/LUT.h>
//#include <encode/interface/Encoder.h>

typedef uint32_t ADC;
//class Encoder;

class QCore
{

  public:
    QCore (int module_in, int chip_in, uint32_t ccol_in, uint32_t qcrow_in, bool isneighbour_in, bool islast_in, std::vector<ADC> adcs_in);
    std::vector<ADC> adcs;
    std::vector<bool> hitmap;
    std::vector<bool> encoded_hitmap;
    bool islast;
    bool isneighbour;
    uint32_t ccol; //range 1 to 54 for 54 core columns
    uint32_t qcrow; //quarter core row address in units of qcrow factor rows
    //uint32_t core_row; //core row
    //uint32_t quarter_core_row;// quarter core row within the core, range 0 to 3
    //uint8_t qcrow; //encoded qcrow value: 6bits core_row + 2 bits quarter core in that core
    int module;
    int chip;

    std::vector<bool> binary_tots () const;
    std::vector<bool> row (int row_index) const;
    void print (std::ostream& stream) const;
    //void set_encoded_hitmap (std::vector<bool> map);

  private:
    std::vector<bool> adcs_to_hitmap (std::vector<ADC> adcs);
    void encode_hitmap();

};


#endif
