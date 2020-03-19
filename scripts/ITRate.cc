#include <iostream>
#include<TFile.h>
#include<TTreeReader.h>
#include<TTreeReaderArray.h>
#include<encode/interface/EncoderControl.h>
#include<include/cxxopts.hpp>
#include<encode/interface/Encoder.h>
#include<util/Util.h>

using namespace std;

IntMatrix empty_matrix(uint32_t nrows, uint32_t ncols){
    IntMatrix matrix;
    matrix.reserve(nrows);
    for (uint32_t r = 0; r < nrows; r++) {
            vector<ADC> row;
            row.reserve(ncols);
            for (uint32_t c = 0; c < ncols; c++) {
                  row.push_back(0);
            }
            assert(row.size()==ncols);
            matrix.push_back(row);
    }
    assert(matrix.size()==nrows);
    return matrix;
}
vector<bool> to_stream(vector<QCore> & qcores, bool tot) {
    vector<bool> s;

    bool new_ccol = true;
    for(auto q : qcores) {
        cout << q.ccol << " " << q.qcrow << " " << q.isneighbour << " " << q.islast << endl;
        // cout <<s.size()<<endl;
        if(new_ccol){
            vector<bool> ccol_address = int_to_binary(q.ccol, 6);
            // assert(ccol_address.size()==6);
            s.insert(s.end(), ccol_address.begin(), ccol_address.end());
        }
        // s.push_back(q.islast);
        // s.push_back(q.isneighbour);
        // if(not q.isneighbour){
        //     vector<bool> qrow_address = int_to_binary(q.qcrow, 8);
        //     s.insert(s.end(), qrow_address.begin(), qrow_address.end());
        // }
        // s.insert(s.end(), q.encoded_hitmap.begin(), q.encoded_hitmap.end());
        // if(tot){
        //     auto tots = q.binary_tots();
        //     s.insert(s.end(), tots.begin(), tots.end());
        // }
        // new_ccol = q.islast;
    }

    return s;
}
void write_bits(ofstream & f, vector<bool> bits, int & bits_written) {
    for (auto bit: bits){
        if((bits_written % 64 == 0)){
            // Start new stream
            if(bits_written > 0) {
                f << endl;
                f << "0";
            } else {
                f << "1";
            }
            bits_written++;
        }
        f << bit ? "1" : "0";
        bits_written++;
    }
}
void write_bits(ofstream & f, bool bit, int & bits_written) {
    vector<bool> vec = {bit};
    write_bits(f, vec, bits_written);
}
void qcores_to_file(vector<QCore> & qcores, bool tot){
    ofstream f;
    f.open("stream.txt");
    bool new_ccol = true;
    int bits_written = 0;
    vector<bool> event = {1,0,1,0,1,0,0,1};
    write_bits(f, event, bits_written);
    f<<"|";
    int nqcore=0;
    for(auto q: qcores){
        int n1=0;
        int n2=0;
        if(new_ccol){
            vector<bool> ccol_address = int_to_binary(53, 6);
            write_bits(f, ccol_address, bits_written);
            // assert(ccol_address.size()==6);
        }
        f<<":";
        write_bits(f, q.islast, bits_written);
        write_bits(f, q.isneighbour, bits_written);
        f<<":";
        if(not q.isneighbour){
            vector<bool> qrow_address = int_to_binary(q.qcrow, 8);
            write_bits(f, qrow_address, bits_written);
        }
        f<<":";
        write_bits(f, q.encoded_hitmap, bits_written);

        if(tot){
            f<<":";
            write_bits(f, q.binary_tots(), bits_written);
        }
        f<<"|";
        new_ccol=q.islast;
    }

    // Fill up frame with orphan bits
    while(bits_written % 64 != 0 ) {
        write_bits(f, false, bits_written);
    }
    f << endl;
    f.close();
}
void stream_to_file(vector<bool> & stream, string path) {
    ofstream f;
    f.open (path);
    for(auto bit: stream){
        f << bit ? "1" : "0";
    }
    f.close();
}

int main(int argc, char *argv[]){
    cxxopts::Options options("ITRate", "One line description of MyProgram");
    options.add_options()
    ("b,barrel", "Look at barrel",cxxopts::value<bool>())
    ("e,endcap", "Look at endcap",cxxopts::value<bool>())
    ("r,ringlayer", "Ring or layer to consider",cxxopts::value<int>())
    ("d,diskladder", "Disk or ladder to consider",cxxopts::value<int>())
    ("n,nevents", "Number of events to process",cxxopts::value<int>())
    ;
    auto opts = options.parse(argc, argv);

    TFile * file = new TFile("/home/albert/repos/it_encoding_rates/itdigi.root");
    TTreeReader reader("BRIL_IT_Analysis/Digis", file);
    TTreeReaderArray<bool> trv_barrel(reader, "barrel");
    TTreeReaderArray<uint32_t> trv_module(reader, "module");
    TTreeReaderArray<uint32_t> trv_adc(reader, "adc");
    TTreeReaderArray<uint32_t> trv_col(reader, "column");
    TTreeReaderArray<uint32_t> trv_row(reader, "row");
    TTreeReaderArray<uint32_t> trv_ringlayer(reader, "ringlayer");
    TTreeReaderArray<uint32_t> trv_diskladder(reader, "diskladder");

    uint32_t nrows = 1320 + 40;
    uint32_t ncols = 442;
    int nevents =  opts["nevents"].as<int>();
    int want_ring = opts["ringlayer"].as<int>();
    int want_disk = opts["diskladder"].as<int>();

    bool want_barrel = opts["barrel"].as<bool>();
    bool want_endcap = opts["endcap"].as<bool>();

    if(want_barrel and want_endcap){
        throw "Cannot do barrel and endcap at the same time.";
    }

    TH1D * stream_size = new TH1D("stream_size", "stream_size",1000,0,50000);
    TH1D * h_is_neighbour = new TH1D("isneighbour", "isneighbour_",2,-0.5,1.5);
    TH1D * h_bits_per_qcore = new TH1D("bits_per_qcore", "bits_per_qcore",45,-0.5,44.5);
    TH1D * h_time_per_qcore = new TH1D("time_per_qcore", "time_per_qcore",20,-0.5,19.5);
    TH1D * h_bitrate_per_qcore = new TH1D("bitrate_per_qcore", "bitrate_per_qcore",10,-0.5,9.5);
    TH1D * h_bitrate_per_qcore_with_tot = new TH1D("bitrate_per_qcore_with_tot", "bitrate_per_qcore_with_tot",10,-0.5,9.5);
    TH1D * h_hit_rows = new TH1D("hit_rows", "hit_rows",3,-0.5,2.5);
    TH1D * h_is_last = new TH1D("is_last", "is_last",2,-0.5,1.5);
    TH1D * h_row = new TH1D("row", "row",256,-0.5,255.5);
    TH1D * h_nqcore = new TH1D("nqcore", "nqcore",100,0,10000);

    uint32_t nevent = 0;

    map<uint32_t, IntMatrix> matrices;
    map<uint32_t, vector<QCore>> qcores;
    IntMatrix matrix;
    EncoderControl enc;

    map<int, int> rowtime;
    rowtime[0] = 0;
    rowtime[1] = 4;
    rowtime[2] = 4;
    rowtime[3] = 4;
    rowtime[4] = 4;
    rowtime[5] = 5;
    rowtime[6] = 5;
    rowtime[7] = 5;
    rowtime[8] = 4;
    rowtime[9] = 5;
    rowtime[10] = 5;
    rowtime[11] = 5;
    rowtime[12] = 4;
    rowtime[13] = 5;
    rowtime[14] = 5;
    rowtime[15] = 5;
    rowtime[16] = 4;
    rowtime[17] = 6;
    rowtime[18] = 6;
    rowtime[19] = 6;
    rowtime[20] = 6;
    rowtime[21] = 7;
    rowtime[22] = 7;
    rowtime[23] = 7;
    rowtime[24] = 6;
    rowtime[25] = 7;
    rowtime[26] = 7;
    rowtime[27] = 7;
    rowtime[28] = 6;
    rowtime[29] = 7;
    rowtime[30] = 7;
    rowtime[31] = 7;
    rowtime[32] = 4;
    rowtime[33] = 6;
    rowtime[34] = 6;
    rowtime[35] = 6;
    rowtime[36] = 6;
    rowtime[37] = 7;
    rowtime[38] = 7;
    rowtime[39] = 7;
    rowtime[40] = 6;
    rowtime[41] = 7;
    rowtime[42] = 7;
    rowtime[43] = 7;
    rowtime[44] = 6;
    rowtime[45] = 7;
    rowtime[46] = 7;
    rowtime[47] = 7;
    rowtime[48] = 4;
    rowtime[49] = 6;
    rowtime[50] = 6;
    rowtime[51] = 6;
    rowtime[52] = 6;
    rowtime[53] = 7;
    rowtime[54] = 7;
    rowtime[55] = 7;
    rowtime[56] = 6;
    rowtime[57] = 7;
    rowtime[58] = 7;
    rowtime[59] = 7;
    rowtime[60] = 6;
    rowtime[61] = 7;
    rowtime[62] = 7;
    rowtime[63] = 7;
    rowtime[64] = 4;
    rowtime[65] = 6;
    rowtime[66] = 6;
    rowtime[67] = 6;
    rowtime[68] = 6;
    rowtime[69] = 7;
    rowtime[70] = 7;
    rowtime[71] = 7;
    rowtime[72] = 6;
    rowtime[73] = 7;
    rowtime[74] = 7;
    rowtime[75] = 7;
    rowtime[76] = 6;
    rowtime[77] = 7;
    rowtime[78] = 7;
    rowtime[79] = 7;
    rowtime[80] = 5;
    rowtime[81] = 7;
    rowtime[82] = 7;
    rowtime[83] = 7;
    rowtime[84] = 7;
    rowtime[85] = 8;
    rowtime[86] = 8;
    rowtime[87] = 8;
    rowtime[88] = 7;
    rowtime[89] = 8;
    rowtime[90] = 8;
    rowtime[91] = 8;
    rowtime[92] = 7;
    rowtime[93] = 8;
    rowtime[94] = 8;
    rowtime[95] = 8;
    rowtime[96] = 5;
    rowtime[97] = 7;
    rowtime[98] = 7;
    rowtime[99] = 7;
    rowtime[100] = 7;
    rowtime[101] = 8;
    rowtime[102] = 8;
    rowtime[103] = 8;
    rowtime[104] = 7;
    rowtime[105] = 8;
    rowtime[106] = 8;
    rowtime[107] = 8;
    rowtime[108] = 7;
    rowtime[109] = 8;
    rowtime[110] = 8;
    rowtime[111] = 8;
    rowtime[112] = 5;
    rowtime[113] = 7;
    rowtime[114] = 7;
    rowtime[115] = 7;
    rowtime[116] = 7;
    rowtime[117] = 8;
    rowtime[118] = 8;
    rowtime[119] = 8;
    rowtime[120] = 7;
    rowtime[121] = 8;
    rowtime[122] = 8;
    rowtime[123] = 8;
    rowtime[124] = 7;
    rowtime[125] = 8;
    rowtime[126] = 8;
    rowtime[127] = 8;
    rowtime[128] = 4;
    rowtime[129] = 6;
    rowtime[130] = 6;
    rowtime[131] = 6;
    rowtime[132] = 6;
    rowtime[133] = 7;
    rowtime[134] = 7;
    rowtime[135] = 7;
    rowtime[136] = 6;
    rowtime[137] = 7;
    rowtime[138] = 7;
    rowtime[139] = 7;
    rowtime[140] = 6;
    rowtime[141] = 7;
    rowtime[142] = 7;
    rowtime[143] = 7;
    rowtime[144] = 5;
    rowtime[145] = 7;
    rowtime[146] = 7;
    rowtime[147] = 7;
    rowtime[148] = 7;
    rowtime[149] = 8;
    rowtime[150] = 8;
    rowtime[151] = 8;
    rowtime[152] = 7;
    rowtime[153] = 8;
    rowtime[154] = 8;
    rowtime[155] = 8;
    rowtime[156] = 7;
    rowtime[157] = 8;
    rowtime[158] = 8;
    rowtime[159] = 8;
    rowtime[160] = 5;
    rowtime[161] = 7;
    rowtime[162] = 7;
    rowtime[163] = 7;
    rowtime[164] = 7;
    rowtime[165] = 8;
    rowtime[166] = 8;
    rowtime[167] = 8;
    rowtime[168] = 7;
    rowtime[169] = 8;
    rowtime[170] = 8;
    rowtime[171] = 8;
    rowtime[172] = 7;
    rowtime[173] = 8;
    rowtime[174] = 8;
    rowtime[175] = 8;
    rowtime[176] = 5;
    rowtime[177] = 7;
    rowtime[178] = 7;
    rowtime[179] = 7;
    rowtime[180] = 7;
    rowtime[181] = 8;
    rowtime[182] = 8;
    rowtime[183] = 8;
    rowtime[184] = 7;
    rowtime[185] = 8;
    rowtime[186] = 8;
    rowtime[187] = 8;
    rowtime[188] = 7;
    rowtime[189] = 8;
    rowtime[190] = 8;
    rowtime[191] = 8;
    rowtime[192] = 4;
    rowtime[193] = 6;
    rowtime[194] = 6;
    rowtime[195] = 6;
    rowtime[196] = 6;
    rowtime[197] = 7;
    rowtime[198] = 7;
    rowtime[199] = 7;
    rowtime[200] = 6;
    rowtime[201] = 7;
    rowtime[202] = 7;
    rowtime[203] = 7;
    rowtime[204] = 6;
    rowtime[205] = 7;
    rowtime[206] = 7;
    rowtime[207] = 7;
    rowtime[208] = 5;
    rowtime[209] = 7;
    rowtime[210] = 7;
    rowtime[211] = 7;
    rowtime[212] = 7;
    rowtime[213] = 8;
    rowtime[214] = 8;
    rowtime[215] = 8;
    rowtime[216] = 7;
    rowtime[217] = 8;
    rowtime[218] = 8;
    rowtime[219] = 8;
    rowtime[220] = 7;
    rowtime[221] = 8;
    rowtime[222] = 8;
    rowtime[223] = 8;
    rowtime[224] = 5;
    rowtime[225] = 7;
    rowtime[226] = 7;
    rowtime[227] = 7;
    rowtime[228] = 7;
    rowtime[229] = 8;
    rowtime[230] = 8;
    rowtime[231] = 8;
    rowtime[232] = 7;
    rowtime[233] = 8;
    rowtime[234] = 8;
    rowtime[235] = 8;
    rowtime[236] = 7;
    rowtime[237] = 8;
    rowtime[238] = 8;
    rowtime[239] = 8;
    rowtime[240] = 5;
    rowtime[241] = 7;
    rowtime[242] = 7;
    rowtime[243] = 7;
    rowtime[244] = 7;
    rowtime[245] = 8;
    rowtime[246] = 8;
    rowtime[247] = 8;
    rowtime[248] = 7;
    rowtime[249] = 8;
    rowtime[250] = 8;
    rowtime[251] = 8;
    rowtime[252] = 7;
    rowtime[253] = 8;
    rowtime[254] = 8;
    rowtime[255] = 8;

    while (reader.Next()) {
        if(nevent > nevents){
            break;
        }
        int current_module = -1;
        uint32_t ientry=0;
        for( auto imod : trv_module) {
            bool barrel = trv_barrel.At(ientry);
            uint32_t ringlayer = trv_ringlayer.At(ientry);
            uint32_t diskladder = trv_diskladder.At(ientry);

            if((ringlayer != want_ring) or (diskladder != want_disk) or (want_barrel!=barrel)) {
                ientry++;
                continue;
            }

            if(imod != current_module) {
                current_module = imod;
                matrices[current_module] = empty_matrix(nrows, ncols);
            }
            uint32_t row = trv_row.At(ientry);
            uint32_t col = trv_col.At(ientry);
            uint32_t adc = trv_adc.At(ientry);
            assert(row<nrows);
            assert(col<ncols);
            matrices[current_module][row][col] = adc;
            ientry++;
        }

        cout << "Event " << nevent << endl;
        int nqcore = 0;
        for(auto pair : matrices){
            vector<QCore> tmp = enc.qcores(pair.second, nevent, pair.first);
            for(auto const q : tmp){
                nqcore++;
                h_is_neighbour->Fill(int(q.isneighbour));
                h_is_last->Fill(int(q.islast));

                int bits = 2;
                if(not q.isneighbour) bits += 8;
                if(q.islast) bits += 6;
                bits+=q.encoded_hitmap.size();
                h_bits_per_qcore->Fill(bits);

                // Timing
                int time_per_qcore = 0;
                time_per_qcore += rowtime.at(binary_to_int(q.row(0)));
                time_per_qcore += rowtime.at(binary_to_int(q.row(1)));

                int nhits = 0;
                for(auto bit : q.hitmap) {
                    if(bit) nhits++;
                }
                h_time_per_qcore->Fill(time_per_qcore);
                h_bitrate_per_qcore->Fill(float(bits) / time_per_qcore);
                h_bitrate_per_qcore_with_tot->Fill(float(bits + nhits*4)/ time_per_qcore);

                bool row_1_hit = false;
                bool row_2_hit = false;
                for(int ipix=0; ipix<8; ipix++) {
                    row_1_hit |= q.hitmap.at(ipix);
                    row_2_hit |= q.hitmap.at(8+ipix);
                }
                int n_rows = 0;
                if(row_1_hit) n_rows++;
                if(row_2_hit) n_rows++;
                h_hit_rows->Fill(n_rows);
                h_row->Fill(binary_to_int(q.row(0)));
                h_row->Fill(binary_to_int(q.row(1)));
            }
            qcores[pair.first].insert(qcores[pair.first].end(), tmp.begin(), tmp.end());
        }
        h_nqcore->Fill(nqcore);
        // qcores_to_file(qcores.at(1),true);
        // stream_to_file(s,"stream.txt");
        nevent++;
    }
    TString outfile;
    if(want_barrel){
        outfile.Form("./output/encoder_histos_layer%d_ladder%d.root",want_ring, want_disk);
    } else {
        outfile.Form("./output/encoder_histos_ring%d_disk%d.root",want_ring,want_disk);
    }
    enc.save_histograms(outfile);

    TFile * f = new TFile(outfile,"UPDATE");
    stream_size->SetDirectory(f);
    stream_size->Write();
    h_is_neighbour->SetDirectory(f);
    h_is_neighbour->Write();
    h_is_last->SetDirectory(f);
    h_is_last->Write();
    h_bits_per_qcore->SetDirectory(f);
    h_bits_per_qcore->Write();
    h_time_per_qcore->SetDirectory(f);
    h_time_per_qcore->Write();
    h_bitrate_per_qcore->SetDirectory(f);
    h_bitrate_per_qcore->Write();
    h_bitrate_per_qcore_with_tot->SetDirectory(f);
    h_bitrate_per_qcore_with_tot->Write();

    

    h_hit_rows->SetDirectory(f);
    h_hit_rows->Write();
    h_nqcore->SetDirectory(f);
    h_nqcore->Write();
    h_row->Write();
    f->Close();

}
