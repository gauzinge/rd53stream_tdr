#include <util/Serializer.h>

Serializer::Serializer()
{

}

void Serializer::to_binary_stream (std::vector<bool>& vec, uint32_t value, size_t nbits)
{
    assert (nbits > 0);

    while (nbits > 0)
    {
        //if (nbits == 0) break;

        bool bit = (value >> (nbits - 1) ) & 0x1;

        //std::cout << nbits - 1 << " " << bit << std::endl;
        //check if the vector size modolo 64 is 0 which means the next bit needs to be a NS=1 or NS=0 bit
        if (vec.size() % 64 == 0 && vec.size() != 0)
        {
            //std::cout << "CASE 1 - I am at a word boundary -  word # " << vec.size() / 64 << std::endl;
            //in this condition, we have 64 bits full but we are not the first bit in the stream
            vec.push_back (0);
        }

        vec.push_back (bit);

        nbits--;

    }
}

//useful overload for the binary vectors of the QCore object
void Serializer::to_binary_stream (std::vector<bool>& vec, std::vector<bool> to_write)
{
    assert (to_write.size() > 0);

    if (int (64 - (vec.size() % 64) ) - int (to_write.size() )  > 0)
    {
        //the vector to append fits within the same 64 bit word
        //it's insane but If I am at index 0 of the new word and the vec.size() > 0 I need to insert a NS=0 bit

        if (vec.size() % 64 == 0 && vec.size() != 0)
        {
            //std::cout << "Size " << vec.size() << " word " << vec.size() / 64 << " index " << vec.size() % 64 << " left " << 64 - (vec.size() % 64) << " to write " << to_write.size() << " " << 64 - (vec.size() % 64) - to_write.size() <<  std::endl;
            vec.push_back (0);
        }

        vec.insert (vec.end(), to_write.begin(), to_write.end() );
    }
    else
    {
        //I have to wrap to the next 64 bit word and thus need a new stream bit
        for (auto bit : to_write)
        {
            if (vec.size() % 64 == 0 && vec.size() != 0)
            {
                //std::cout << "I am at a word boundary -  word # " << vec.size() / 64 << std::endl;
                //in this condition, we have 64 bits full but we are not the first bit in the stream
                vec.push_back (0);
            }

            //Iknow, not the most efficient but safe and takes care of new stream bit
            vec.push_back (bit);
        }
    }
}

/**
 * Given a vector of QCores per chip, write the formatted data to a vector of bool
 */
std::vector<bool> Serializer::serializeChip (std::vector<QCore>& qcores, uint32_t event, uint32_t chip, bool tot, bool print, std::ostream& f)
{
    std::vector<bool> ret_vec;
    bool new_ccol = true;
    //start a new stream so new stream bit is 1
    ret_vec.push_back (1);
    //we assume multi chip mode, so encode 2 bit chip id
    this->to_binary_stream (ret_vec, chip, 2);
    //8 bits tag, we use event number for this
    this->to_binary_stream (ret_vec, event, 8);

    //now tackle the QCores
    //int nqcore = 0;
    if (print)
    {
        f << "Number of hit Qcores: " << qcores.size() << std::endl;
        f << "Chip " << chip << "  Event " << event << std::endl;
    }


    for (auto qcore : qcores)
    {
        //int n1 = 0;
        //int n2 = 0;

        if (new_ccol)
            this->to_binary_stream (ret_vec, qcore.ccol, 6 );

        this->to_binary_stream (ret_vec, qcore.islast, 1);
        to_binary_stream (ret_vec, qcore.isneighbour, 1);

        if (!qcore.isneighbour)
            this->to_binary_stream (ret_vec, qcore.qcrow, 8 );

        if (print)
        {
            qcore.print (f);
            f << "islast " << qcore.islast << " isneighbor " << qcore.isneighbour <<  std::endl;
            f << "Encoded hitmap: ";

            for (auto bit : qcore.encoded_hitmap)
                f << bit;

            f << std::endl;

        }


        to_binary_stream (ret_vec, qcore.encoded_hitmap);

        if (tot)
            to_binary_stream (ret_vec, qcore.binary_tots() );

        new_ccol = qcore.islast;
    }

    //now the orphan bits
    size_t orphan_bits = 64 - (ret_vec.size() % 64);
    std::vector<bool> orphan_vec (orphan_bits, 0);
    ret_vec.insert (ret_vec.end(), orphan_vec.begin(), orphan_vec.end() );

    if (orphan_bits < 6)
    {
        //fill the 64 bit word, insert a NS bit = 0 and 63 zeros
        std::vector<bool> orphan_vec (64, 0);
        ret_vec.insert (ret_vec.end(), orphan_vec.begin(), orphan_vec.end() );
    }

    if (print)
    {
        f << "Stream data: " << std::endl;
        size_t index = 0;

        for (auto bit : ret_vec)
        {
            f << bit;
            index++;

            if (index % 64 == 0)
                f << std::endl;
        }

        f << std::endl << std::endl;
    }

    return ret_vec;
}
