#ifndef ENCODER_CTRL_H
#define ENCODER_CTRL_H
#include <vector>
#include <stdint.h>
#include <TH1D.h>
#include <TString.h>
#include <encode/interface/Encoder.h>
#include <map> 
using namespace std;


class EncoderControl : public Encoder {
    public:
        EncoderControl();
        map<TString, TH1D> histograms;
        void save_histograms(TString filename);
    protected:
        void qcore_control_plot(vector<bool> hitmap, std::vector<bool> compressed_hitmap);
};
#endif // ENCODER_CTRL_H