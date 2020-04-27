#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <util/ChipIdentifier.h>
#include <assert.h>

using namespace std;


#ifndef DECODERBASE_H
#define DECODERBASE_H

#define TAGBITS 8
#define CCOLBITS 6
#define QCROWBITS 8
#define TOTBITS 4

class DecoderBase
{
  public:
    DecoderBase() : bitsread (0) {};
    virtual ~DecoderBase() {};
    void load_file (string path_to_file);
    virtual void decode (bool do_tot);
    void shift_buffer (int nbits);
    vector<bool> buffer;
    enum State
    {
        START = 0,
        CHIP = 1,
        TAG = 2,
        TAGORCCOL = 3,
        CCOL = 4,
        ISLAST = 5,
        ISNEIGHBOR = 6,
        QROW = 7,
        ROWOR = 8,
        ROW = 9,
        TOT = 10,
        END = 11
    };
    void move_ahead (int nbits);
    virtual std::pair<int, int> decode_row();

    void print_buffer (std::ostream& stream = std::cout);

  protected:
    vector<bool>::iterator position;
    size_t pos;
    int clk;
    int state;
    int bitsread;

    uint32_t headerversion;
    size_t headersize;
    std::vector<uint32_t> headervec;
    size_t chips_read;

    std::ifstream file;
    int filepos;

    void print_header (std::ostream& stream = std::cout);

  public:
    size_t getNchips()
    {
        return headersize;
    }

    //method to read binary file
    template<typename T>
    void read_binaryheader (std::string path_to_file)
    {
        this->file.open (path_to_file.c_str(), std::ios::in | std::ios::binary);

        if (file.is_open() )
        {
            T version;
            this->file.read ( (char*) &version, sizeof (T) );
            std::cout << "Version Header read version: " << version << std::endl;
            this->headerversion = static_cast<uint32_t> (version);

            T headersizeT;
            this->file.read ( (char*) &headersizeT, sizeof (T) );
            std::cout << "The header size for this file is: " << headersizeT << " which equates to " << headersizeT << " modules connected to this DTC" << std::endl;
            this->headersize = static_cast<size_t> (headersizeT);

            std::vector<T> headerbuffer (this->headersize);
            //headerbuffer.reserve (this->headersize);
            //std::cout << headerbuffer.size() << std::endl;
            this->file.read ( (char*) &headerbuffer[0], sizeof (T) * this->headersize );
            this->filepos = this->file.tellg();

            for (auto word : headerbuffer)
                this->headervec.push_back (static_cast<uint32_t> (word) );

            this->print_header (std::cout);
            this->chips_read = 0;

        }
    }

    template<typename T>
    void read_chip (std::ostream& stream = std::cout)
    {
        if (file.is_open() )
        {
            this->file.seekg (this->filepos);
            T nwords;
            this->file.read ( (char*) &nwords, sizeof (T) );
            stream << "###############################################################" << std::endl;
            stream << "This chip has " << nwords << " words of " << sizeof (T) * 8 << " bits which corresponds to " << nwords * 8 * sizeof (T) / 64 << " 64 bit words" << std::endl;

            std::vector<T> buf (nwords);
            this->file.read ( (char*) &buf[0], nwords * sizeof (T) );

            this->filepos = this->file.tellg();

            size_t wordsize_bit = sizeof (T) * 8;

            if (buf.size() )
            {
                this->buffer.clear();

                //now iterate the bits in the buf vector and push back into the chip buffer
                int counter = 0;

                for (auto& word : buf)
                {
                    for (int shift = wordsize_bit - 1 ; shift >= 0 ; shift--)
                    {
                        bool bit = (word >> shift) & 1;
                        this->buffer.push_back (bit);
                    }

                    counter++;
                }

                assert (this->buffer.size() == wordsize_bit * buf.size() );
                assert (*this->buffer.begin() == 1); //stream for each chip needs to start with a new stream bit;
                //now read the chip ID
                bool h = this->buffer.at (1);
                bool l = this->buffer.at (2);
                uint32_t chipid = (h << 1) | l;

                size_t moduleindex = this->chips_read / 4;
                uint32_t moduleword = this->headervec.at (moduleindex);

                //uint32_t nchips = moduleword & 0x7;
                uint32_t module = (moduleword >> 3) & 0x3F;
                uint32_t ring = (moduleword >> 9) & 0x7;
                uint32_t disk = (moduleword >> 12) & 0xF;

                ChipIdentifier chip (disk, ring, module, chipid);
                chip.print();

                this->chips_read++;
            }
        }
    }
};

#endif /* DECODERBASE_H */
