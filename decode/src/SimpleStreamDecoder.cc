#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<decode/interface/SimpleStreamDecoder.h>
#include<decode/interface/DecoderBase.h>
#include<util/Util.h>
using namespace std;


std::pair<int, int> SimpleStreamDecoder::decode_row(){
    vector<bool> row;
    row.reserve(14);
    row.insert(row.end(), position, position+14);

    for(auto bit : row) {
        cout << int(bit);
    }
    cout <<endl;
    return hits_in_row(row);
}

void SimpleStreamDecoder::decode(){
    // using namespace SimpleStreamDecoder;
    state = START;
    position = buffer.begin();
    bool islast;
    int hit_count = 0;
    int iterations = 0;
    int nrows = -1;
    bool row1_hit=false;
    bool row2_hit=false;
    bool do_tot = false;
    int n1=0;
    int n2=0;
    int nqcore=0;
    while(true) {
        iterations++;
        if(state == END){
            break;
        }
        // cout << state << " " << bitsread << endl;
        switch(state){
            case START: {
                if(not *position) {
                    throw "Did not find NS=1 at start!";
                }
                move_ahead(1);
                state = TAG;
                continue;
            }
            case TAGORCCOL: {
                if( *position and *(position+1) and *(position+2) ){
                    move_ahead(3);
                    state = TAG;
                } else {
                    bool orphan = true;
                    for(int i = 0; i<6; i++){
                        // cout << "orphan " << i << " " << *(position+i) << endl;
                        orphan &= not *(position + i);
                    }
                    // cout << "ORPHAN " << orphan <<endl;
                    if(orphan) {
                        state = END;
                    } else {
                        state = CCOL;
                    }
                }
                continue;
            }
            case TAG:{
                move_ahead(8);
                state = TAGORCCOL;
                continue;
            }
            case CCOL: {
                move_ahead(6);
                state = ISLAST;
                continue;
            }
            case ISLAST: {
                islast = *position;
                move_ahead(1);
                state = ISNEIGHBOR;
                continue;
            }
            case ISNEIGHBOR: {
                if(*position) {
                    state = ROWOR;
                } else {
                    state = QROW;
                }
                move_ahead(1);
                continue;
            }
            case QROW: {
                state = ROWOR;
                move_ahead(8);
                continue;
            }
            case ROWOR: {
                if(not *position) {
                    nrows = 1;
                    row1_hit = false;
                    row2_hit = true;
                    move_ahead(1);
                } else if (*position and *(position+1)){
                    nrows = 2;
                    move_ahead(2);
                    row1_hit = true;
                    row2_hit = true;
                } else {
                    nrows = 1;
                    move_ahead(2);
                    row1_hit = true;
                    row2_hit = false;
                }
                state=ROW;
                continue;
            }
            case ROW: {
                if(nrows == 0) {
                    throw "Reached state ROW with ill-defined number of rows.";
                }
                pair<int, int> size_and_count = decode_row();
                int row_size = size_and_count.first;
                hit_count += size_and_count.second;
                if( row1_hit and row2_hit) {
                    if(nrows==2) {
                        n1+=size_and_count.second;
                    } else {
                        n2+=size_and_count.second;
                    }
                } else if (row1_hit) {
                    n1+=size_and_count.second;
                } else if (row2_hit) {
                    n2+=size_and_count.second;
                }
                move_ahead(row_size);
                nrows--;
                if(nrows > 0){
                    state = ROW;
                } else {
                    state = TOT;
                }
                continue;
            }
            case TOT:{
                cout <<  nqcore++ << " " << "QCORE "<< n1 << " " << n2 << " " <<n1+n2<<endl;
                n1 = 0;
                n2=0;
                if(do_tot){
                    if(hit_count < 0){
                        cout << "Reached state TOT with ill-defined hit count " << hit_count << "!" << endl;
                        throw "Reached state TOT with ill-defined hit count";
                    }
                    move_ahead(4*hit_count);
                }
                hit_count = 0;
                if(islast) {
                    state = TAGORCCOL;
                } else {
                    state = ISLAST;
                }
                continue;
            }
            case END:{
                break;
            }
        }
    }

}
enum Align {
    ccol=0,
    hitmap,
};