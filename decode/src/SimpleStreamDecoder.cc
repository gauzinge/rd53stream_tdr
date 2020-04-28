#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include<decode/interface/SimpleStreamDecoder.h>
#include<decode/interface/DecoderBase.h>
#include<util/Util.h>
using namespace std;


std::pair<int, int> SimpleStreamDecoder::decode_row()
{
    vector<bool> row;
    row.reserve (14);
    row.insert (row.end(), position, position + 14);

    //std::cout << "ROW: ";

    //for (auto bit : row)
    //cout << int (bit);

    //cout << endl;

    return hits_in_row (row);
}

void SimpleStreamDecoder::decode (bool do_tot)
{
    uint16_t chip = 0;
    uint16_t tag = 0;
    uint16_t ccol = 0;
    uint16_t qcrow = 0;
    // using namespace SimpleStreamDecoder;
    state = START;
    position = buffer.begin();
    bool islast;
    bool isneighbor;
    int hit_count = 0;
    int iterations = 0;
    int nrows = -1;
    bool row1_hit = false;
    bool row2_hit = false;
    //bool do_tot = false;
    int n1 = 0;
    int n2 = 0;
    int nqcore = 0;

    //before decoding, I should remove all the NS=0 bits from the vector
    size_t buffsize = this->buffer.size() / 64;

    for (size_t nword = buffsize - 1; nword > 0; nword--)
    {
        size_t bit = nword * 64;
        std::cout << "Removing NS & chip bits for easier decoding from word " << nword + 1  << std::endl;

        if (!this->buffer.at (bit) )
            this->buffer.erase (this->buffer.begin() + bit, this->buffer.begin() + bit + 3);
        else std::cout << "ERROR, NS bit 0 expected but 1 detected" << std::endl;
    }

    std::cout << std::endl;

    //this->print_buffer();

    std::cout << "***************************CHIP***************************" << std::endl;


    while (true)
    {
        iterations++;

        if (state == END)
            break;



        switch (state)
        {
            case START:
            {
                if (not * position)
                    throw "Did not find NS=1 at start!";

                move_ahead (1);
                state = CHIP;
                continue;
            }

            case CHIP:
            {
                chip = *position << 1 | * (position + 1);
                //std::cout << "Chip ID " << chip << std::endl;
                move_ahead (2);
                state = TAG;
                continue;
            }

            case TAGORCCOL:
            {
                if ( *position and * (position + 1) and * (position + 2) )
                {
                    move_ahead (3);
                    state = TAG;
                }
                else
                {
                    bool orphan = true;

                    for (int i = 0; i < 6; i++)
                    {
                        //cout << "orphan " << i << " " << * (position + i) << endl;
                        orphan &= not * (position + i);
                        //move_ahead (6);
                    }


                    if (orphan)
                    {

                        cout << "Orphan bits detected! - end of Event for this chip " << endl << endl;
                        state = END;
                        //this->buffer.erase (position, this->buffer.end() );
                    }

                    else
                        state = CCOL;
                }

                continue;
            }

            case TAG:
            {

                for (int bit = 0; bit < TAGBITS; bit++)
                    tag |= (* (position + bit) ) << (TAGBITS - 1 - bit);

                std::cout << "Chip Id: " << chip << " Tag: " << tag << std::endl;
                move_ahead (TAGBITS);

                state = TAGORCCOL;
                continue;
            }

            case CCOL:
            {
                ccol = 0;

                for (int bit = 0; bit < CCOLBITS; bit++)
                    ccol |= (* (position + bit) ) << (CCOLBITS - 1 - bit);

                std::cout << "***************" << std::endl;
                std::cout << "** CCol: " << ccol << " **" << std::endl;
                std::cout << "***************" << std::endl;
                move_ahead (CCOLBITS);
                state = ISLAST;
                continue;
            }

            case ISLAST:
            {
                islast = *position;


                move_ahead (1);
                state = ISNEIGHBOR;
                continue;
            }

            case ISNEIGHBOR:
            {
                isneighbor = *position;


                if (isneighbor)
                {
                    state = ROWOR;
                    qcrow += 2;
                    std::cout <<  "QCrow: " << qcrow << " | IsLast: " << islast << " | IsNeighbor: " << isneighbor << std::endl;
                    std::cout << "***************" << std::endl;
                    //std::cout << "IsLast: " << islast << " IsNeighbor: " << isneighbor << std::endl;
                }

                else
                    state = QROW;

                move_ahead (1);
                continue;
            }

            case QROW:
            {

                //uint16_t qcrow = 0;
                qcrow = 0;

                for (int bit = 0; bit < QCROWBITS; bit++)
                    qcrow |= (* (position + bit) ) << (QCROWBITS - 1 - bit);


                std::cout << "QCrow: " << qcrow << " | IsLast: " << islast << " | IsNeighbor: " << isneighbor << std::endl;
                std::cout << "***************" << std::endl;
                move_ahead (QCROWBITS);
                state = ROWOR;
                continue;
            }

            case ROWOR:
            {
                if (not * position)
                {
                    nrows = 1;
                    row1_hit = false;
                    row2_hit = true;

                    move_ahead (1);
                }
                else if (*position and * (position + 1) )
                {
                    nrows = 2;

                    move_ahead (2);
                    row1_hit = true;
                    row2_hit = true;
                }
                else
                {
                    nrows = 1;

                    move_ahead (2);
                    row1_hit = true;
                    row2_hit = false;
                }



                state = ROW;
                continue;
            }

            case ROW:
            {
                if (nrows == 0)
                    throw "Reached state ROW with ill-defined number of rows.";

                pair<int, int> size_and_count = decode_row();
                int row_size = size_and_count.first;
                hit_count += size_and_count.second;

                if ( row1_hit and row2_hit)
                {
                    if (nrows == 2)
                        n1 += size_and_count.second;

                    else
                        n2 += size_and_count.second;
                }
                else if (row1_hit)
                    n1 += size_and_count.second;

                else if (row2_hit)
                    n2 += size_and_count.second;

                move_ahead (row_size);
                nrows--;

                if (nrows > 0)
                    state = ROW;

                else
                    state = TOT;

                continue;
            }

            case TOT:
            {
                //cout <<  nqcore++ << " " << "QCORE " << n1 << " " << n2 << " " << n1 + n2 << endl;
                std::cout << "Row1 hits: " << n1 <<  std::endl << "Row2 hits: " <<  n2 << std::endl << "-------------" << std::endl << "Total hits: " << hit_count <<  std::endl << "TOT list:" << std::endl;
                //std::cout << "Hit count: " << hit_count << " n1: " << n1 << " n2: " << n2 << " n1+ n2: " << n1 + n2 << std::endl;

                n1 = 0;
                n2 = 0;

                if (do_tot)
                {
                    if (hit_count < 0)
                    {
                        cout << "Reached state TOT with ill-defined hit count " << hit_count << "!" << endl;
                        throw "Reached state TOT with ill-defined hit count";
                    }

                    size_t count = 1;

                    while (hit_count > 0)
                    {
                        uint16_t tot = 0;

                        for (int bit = 0; bit < TOTBITS; bit++)
                            tot |= (* (position + bit) ) << (TOTBITS - 1 - bit);

                        std::cout << "Hit " << count << ": " << tot << std::endl;

                        move_ahead (TOTBITS);
                        hit_count--;
                        count++;
                    }
                }

                hit_count = 0;

                if (islast)
                    state = TAGORCCOL;

                else
                {
                    std::cout << "***************" << std::endl;
                    state = ISLAST;
                }

                continue;
            }

            case END:
            {
                break;
            }
        }
    }

}
enum Align
{
    ccol = 0,
    hitmap,
};
