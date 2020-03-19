#ifndef SIMPLESTREAMDECODER_H
#define SIMPLESTREAMDECODER_H
#include <vector>
#include <decode/interface/DecoderBase.h>
using namespace std;

class SimpleStreamDecoder : public DecoderBase {
    public:
        SimpleStreamDecoder(){};
        ~SimpleStreamDecoder(){};
        void decode() override;
        virtual std::pair<int, int> decode_row() override;
};
#endif /* SIMPLESTREAMDECODER_H */
