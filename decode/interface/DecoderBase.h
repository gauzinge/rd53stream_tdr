#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;


#ifndef DECODERBASE_H
#define DECODERBASE_H

class DecoderBase{
    public:
        DecoderBase() : bitsread(0){};
        virtual ~DecoderBase(){};
        void load_file(string path_to_file);
        virtual void decode();
        void shift_buffer(int nbits);
        vector<bool> buffer;
        enum State{
            START = 0,
            TAG=1,
            TAGORCCOL=2,
            CCOL=3,
            ISLAST=4,
            ISNEIGHBOR=5,
            QROW=6,
            ROWOR=7,
            ROW=8,
            TOT=9,
            END=10
        };
        void move_ahead(int nbits);
        virtual std::pair<int, int> decode_row();
    protected:
        vector<bool>::iterator position;
        int clk;
        int state;
        int bitsread;
};

#endif /* DECODERBASE_H */
