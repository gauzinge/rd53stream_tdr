#include<iostream>
#include <encode/interface/EncoderControl.h>
#include <TFile.h>

using namespace std;

EncoderControl::EncoderControl(){
    this->histograms["compressed_hitmap_size"] = TH1D("compressed_hitmap_size", "compressed_hitmap_size", 31., -0.5, 30.5);
    this->histograms["nhits"] = TH1D("nhits", "nhits", 17., -0.5, 16.5);
}
void EncoderControl::qcore_control_plot(vector<bool> hitmap, vector<bool> compressed_hitmap){
    this->histograms["compressed_hitmap_size"].Fill(compressed_hitmap.size());

    int nhits = 0;
    for( auto const hit : hitmap ){
        if(hit){
            nhits++;
        }
    }
    this->histograms["nhits"].Fill(nhits);
}
void EncoderControl::save_histograms(TString filename){
    TFile * f = new TFile(filename,"RECREATE");
    for(auto pair:this->histograms){
        pair.second.SetDirectory(f);
        pair.second.Write();
    }
    f->Close();
}