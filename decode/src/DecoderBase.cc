#include <decode/interface/DecoderBase.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
using namespace std;

void DecoderBase::decode (bool do_tot)
{

}
std::pair<int, int> DecoderBase::decode_row()
{
    return std::make_pair<int, int> (0, 0);
}

void DecoderBase::move_ahead (int nbits)
{
    int target = bitsread % 64 + nbits;

    if (target <= 64)
    {
        position += nbits;
        bitsread += nbits;
        pos += nbits;
    }
    else
    {
        position += nbits + 1;
        bitsread += nbits + 1;
        pos += nbits + 1;
    }

    //if (position == buffer.end() )
    //throw "Reached end.";
}

void DecoderBase::shift_buffer (int nbits)
{
    int bufsize = this->buffer.size();

    if (nbits >= bufsize)
        this->buffer.clear();

    else
        this->buffer.assign (this->buffer.begin() + nbits, this->buffer.end() );
}

void DecoderBase::load_file (string path_to_file)
{
    cout << "Loading file " << path_to_file << endl;
    string line;
    ifstream file (path_to_file);

    if (file.is_open() )
    {
        while ( getline (file, line) )
        {
            cout << line.size() << endl;

            for (unsigned int i = 0; i < line.size(); i++)
            {
                auto const symbol = line.at (i);

                if (symbol == '1')
                    this->buffer.push_back (true);

                else if (symbol == '0')
                    this->buffer.push_back (false);
            }
        }

        file.close();
    }
    else
        cout << "WARNING: File could not be opened!." << endl;
}

void DecoderBase::print_header (std::ostream& stream)
{

    if (this->headervec.size() )
    {
        for (auto& word : this->headervec)
        {
            uint32_t disk = 0;
            uint32_t ring = 0;
            uint32_t module = 0;
            uint32_t nchips = 0;

            nchips = word & 0x7;
            module = (word >> 3) & 0x3F;
            ring = (word >> 9) & 0x7;
            disk = (word >> 12) & 0xF;

            stream << "Disk: " << disk << " Ring: " << ring << " Module: " << module << " NChips: " << nchips << std::endl;
        }
    }
}

void DecoderBase::print_buffer (std::ostream& stream)
{
    stream << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
    stream << "Stream data: " << std::endl;
    size_t index = 0;

    for (auto bit : this->buffer)
    {
        stream << bit;
        index++;

        if (index % 64 == 0)
            stream << std::endl;
    }

    stream  << std::endl;
}
