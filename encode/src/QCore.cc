#include <encode/interface/QCore.h>

QCore::QCore (int event_in, int module_in, int chip_in, uint32_t ccol_in, uint32_t qcrow_in, bool isneighbour_in, bool islast_in, std::vector<ADC> adcs_in) :
    event (event_in),
    module (module_in),
    chip (chip_in),
    adcs (adcs_in),
    isneighbour (isneighbour_in),
    islast (islast_in),
    qcrow (qcrow_in),
    ccol (ccol_in),
    hitmap (adcs_to_hitmap (adcs) )
    //encoded_hitmap (encoder_in->encode_hitmap (this->hitmap) )
{
    this->encoded_hitmap.clear();
    this->encode_hitmap();
}

std::vector<bool > QCore::binary_tots () const
{
    std::vector<bool > tots;

    for (auto const& adc : adcs)
    {
        if (adc == 0)
            continue;

        for (size_t bit = 4; bit > 0; bit--)
            tots.push_back ( (adc >> (bit - 1) ) & 0x1);
    }

    assert (tots.size() <= 4 * adcs.size() );
    return tots;
};

std::vector<bool > QCore::adcs_to_hitmap (std::vector<ADC > adcs)
{
    assert (adcs.size() == 16);

    std::vector<bool > hitmap;
    // Set up uncompressed hitmap
    hitmap.reserve (16);

    for (auto const adc : adcs)
        hitmap.push_back (adc > 0);

    assert (hitmap.size() == 16);

    return hitmap;
};

std::vector<bool > QCore::row (int row_index) const
{
    if ( (row_index < 0) or (row_index > 1) )
        throw;

    std::vector<bool > row;
    row.insert (row.end(), this->hitmap.begin() + 8 * row_index, this->hitmap.begin() + 8 * (row_index + 1) );
    assert (row.size() == 8);
    return row;
}

void QCore::print (std::ostream& stream = std::cout) const
{
    stream << " Module " << this->module << " Chip " << this->chip << " | Quarter core: CoreCol: " << this->ccol << " QRow: " << this->qcrow << " Hitmap: " <<  std::endl;
    size_t index = 0;

    for (auto hit : this->hitmap)
    {
        stream << hit << " ";

        if (index == 7) stream << std::endl;

        index++;
    }

    stream << std::endl;
}


/**
 * Encode the 16-bit hitmap of using tree-based encoding
 *
 * The output will be between 4 and 30 bits:
 * [Row OR (1-2 bits)][Row 1 (3-14 bits)][Row 2 (3-14 bits)]
 */
void QCore::encode_hitmap ()
{
    assert (this->hitmap.size() == 16);

    // Separate rows
    std::vector<bool> row1, row2;
    row1.insert (row1.end(), this->hitmap.begin(), this->hitmap.begin() + 8);
    row2.insert (row2.end(), this->hitmap.begin() + 8, this->hitmap.end() );
    assert (row1.size() == 8 and row2.size() == 8);

    // Row OR information
    std::vector<bool> row_or = {0, 0};

    for ( auto const bit : row1)
    {
        if (bit)
        {
            row_or[0] = 1;
            break;
        }
    }

    for ( auto const bit : row2)
    {
        if (bit)
        {
            row_or[1] = 1;
            break;
        }
    }

    // Compress the components separately
    // using lookup tables
    // this already replaces 01->0
    std::vector<bool> row_or_enc = enc2 (row_or);

    //this is too easy, I actually need to do the whole separation
    //std::vector<bool> row1_enc = enc8 (row1);
    //std::vector<bool> row2_enc = enc8 (row2);
    std::vector<bool>s2_row1 = {0, 0};
    std::vector<bool>s3_row1 = {0, 0, 0, 0};
    std::vector<bool>s2_row2 = {0, 0};
    std::vector<bool>s3_row2 = {0, 0, 0, 0};

    //now get the binary tree bits for step2 and step3 as per RD53B manual (p70)
    if (row_or[0])
    {
        for (int index = 0; index < row1.size(); index++)
        {
            //if (index < row1.size() / 2 && row1.at (index) ) s2_row1[0] = 1;
            //else if (index >= row1.size() / 2 && row1.at (index) ) s2_row1[1] = 1;

            if (row1.at (index) )
            {
                if (index < 2)
                {
                    s2_row1[0] = 1;
                    s3_row1[0] = 1;
                }
                else if (index >= 2 && index < 4)
                {
                    s2_row1[0] = 1;
                    s3_row1[1] = 1;
                }
                else if (index >= 4 && index < 6)
                {
                    s2_row1[1] = 1;
                    s3_row1[2] = 1;
                }
                else
                {
                    s2_row1[1] = 1;
                    s3_row1[3] = 1;
                }
            }
        }
    }

    if (row_or[1])
    {
        for (int index = 0; index < row2.size(); index++)
        {
            //if (index < row2.size() / 2 && row2.at (index) ) s2_row2[0] = 1;
            //else if (index >= row2.size() && row2.at (index) ) s2_row2[1] = 1;

            if (row2.at (index) )
            {
                if (index < 2)
                {
                    s2_row2[0] = 1;
                    s3_row2[0] = 1;
                }
                else if (index >= 2 && index < 4)
                {
                    s2_row2[0] = 1;
                    s3_row2[1] = 1;
                }
                else if (index >= 4 && index < 6)
                {
                    s2_row2[1] = 1;
                    s3_row2[2] = 1;
                }
                else
                {
                    s2_row2[1] = 1;
                    s3_row2[3] = 1;
                }
            }
        }
    }

    // Merge together
    //vector<bool> encoded_hitmap;
    //this->encoded_hitmap.reserve (row_or_enc.size() + row1_enc.size() + row2_enc.size() );

    //insert first the binary code substituted row_or (step1)
    this->encoded_hitmap.insert (this->encoded_hitmap.end(), row_or_enc.begin(), row_or_enc.end() );

    if (row_or[0])
    {
        //a hit in the first row, so I need the step 2 infor for this row
        std::vector<bool> enc_s2_row1 = enc2 (s2_row1);
        this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_s2_row1.begin(), enc_s2_row1.end() );

        //now check which branch of the tree is there
        if (s2_row1[0]) // the left half of row 1 is hit
        {
            std::vector<bool> s3_left = {s3_row1[0], s3_row1[1]};
            std::vector<bool> enc_s3_left = enc2 (s3_left);
            this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_s3_left.begin(), enc_s3_left.end() );
        }

        if (s2_row1[1]) // the right part of row 1 is hit
        {
            std::vector<bool> s3_right = {s3_row1[2], s3_row1[3]};
            std::vector<bool> enc_s3_right = enc2 (s3_right);
            this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_s3_right.begin(), enc_s3_right.end() );

        }

        //now take care to append the hitmaps
        if (s3_row1[0]) // the left quarter of row 1 is hit
        {
            std::vector<bool> map = {row1[0], row1[1]};
            std::vector<bool> enc_map = enc2 (map);
            this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_map.begin(), enc_map.end() );
        }

        if (s3_row1[1]) // the second quarter of row1 is hit
        {
            std::vector<bool> map = {row1[2], row1[3]};
            std::vector<bool> enc_map = enc2 (map);
            this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_map.begin(), enc_map.end() );
        }

        if (s3_row1[2]) // the second quarter of row1 is hit
        {
            std::vector<bool> map = {row1[4], row1[5]};
            std::vector<bool> enc_map = enc2 (map);
            this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_map.begin(), enc_map.end() );
        }

        if (s3_row1[3]) // the second quarter of row1 is hit
        {
            std::vector<bool> map = {row1[6], row1[7]};
            std::vector<bool> enc_map = enc2 (map);
            this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_map.begin(), enc_map.end() );
        }
    }

    if (row_or[1])
    {
        //a hit in the second row, so I need the step 2 infor for this row
        std::vector<bool> enc_s2_row2 = enc2 (s2_row2);
        this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_s2_row2.begin(), enc_s2_row2.end() );

        //now check which branch of the tree is there
        if (s2_row2[0]) // the left half of row 2 is hit
        {
            std::vector<bool> s3_left = {s3_row2[0], s3_row2[1]};
            std::vector<bool> enc_s3_left = enc2 (s3_left);
            this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_s3_left.begin(), enc_s3_left.end() );
        }

        if (s2_row2[1]) // the right part of row 2 is also hit
        {
            std::vector<bool> s3_right = {s3_row2[2], s3_row2[3]};
            std::vector<bool> enc_s3_right = enc2 (s3_right);
            this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_s3_right.begin(), enc_s3_right.end() );

        }

        //now take care to append the hitmaps
        if (s3_row2[0]) // the left quarter of row 1 is hit
        {
            std::vector<bool> map = {row2[0], row2[1]};
            std::vector<bool> enc_map = enc2 (map);
            this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_map.begin(), enc_map.end() );
        }

        if (s3_row2[1]) // the second quarter of row1 is hit
        {
            std::vector<bool> map = {row2[2], row2[3]};
            std::vector<bool> enc_map = enc2 (map);
            this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_map.begin(), enc_map.end() );
        }

        if (s3_row2[2]) // the second quarter of row1 is hit
        {
            std::vector<bool> map = {row2[4], row2[5]};
            std::vector<bool> enc_map = enc2 (map);
            this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_map.begin(), enc_map.end() );
        }

        if (s3_row2[3]) // the second quarter of row1 is hit
        {
            std::vector<bool> map = {row2[6], row2[7]};
            std::vector<bool> enc_map = enc2 (map);
            this->encoded_hitmap.insert (this->encoded_hitmap.end(), enc_map.begin(), enc_map.end() );
        }

    }

    //this->encoded_hitmap.insert (this->encoded_hitmap.end(), row1_enc.begin(), row1_enc.end() );
    //this->encoded_hitmap.insert (this->encoded_hitmap.end(), row2_enc.begin(), row2_enc.end() );

    //return encoded_hitmap;
}
