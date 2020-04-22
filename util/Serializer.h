#ifndef SERIALIZER_H__
#define SERIALIZER_H__
#include <iostream>
#include <assert.h>
#include <vector>
#include "encode/interface/QCore.h"


class Serializer
{

  public:
    Serializer();
    std::vector<bool> serializeChip (std::vector<QCore>& qcores, uint32_t event, uint32_t chip, bool tot, bool print = false, std::ostream& f = std::cout);
    //template<typename T>
    //std::vector<T> toVec (std::vector<bool>& vec);
    template<typename T>
    std::vector<T> toVec (std::vector<bool>& vec)
    {
        //convert a vector<bool> that is anyway padded to 64bit boundaries to a vector<T> where the bits are encoded in the words for binary file storage
        //in the vector the various data fields are MSB ... LSB so we keep that notation
        //in the end, prepend the vector<T> with one word containing the number of words containing the data

        //first check that input vec is padded to 64 bit boundaries
        assert (vec.size() % 64 == 0);

        size_t sizeT = sizeof (T) * 8; //in bits
        std::vector<T> ret_vec;
        size_t ret_vec_size_calc = vec.size() / sizeT;
        ret_vec.reserve (ret_vec_size_calc + 1);
        //now insert the number of words to be read after reading this word
        ret_vec.push_back (static_cast<T> (ret_vec_size_calc) );

        //now loop the word-sized chunks of vec
        for (size_t word = 0; word < ret_vec_size_calc; word++)
        {
            //now loop the bits in word
            T tmp_word = 0;

            for (size_t bit_shift = 0; bit_shift < sizeT; bit_shift++)
            {
                //example: I want to shift bit 7 in the vector in the LSB of a uint8_t ->bit_shift =0
                //and shift bit 6 in LSB+1 etc ->bit_shift=1
                size_t bit = word * sizeT + (sizeT - 1) - bit_shift;
                tmp_word |= (vec[bit] << bit_shift);
            }

            ret_vec.push_back (tmp_word);
        }

        return ret_vec;
    }


  private:
    void to_binary_stream (std::vector<bool>& vec, uint32_t value, size_t nbits);
    void to_binary_stream (std::vector<bool>& vec, std::vector<bool> to_write);

};


#endif
