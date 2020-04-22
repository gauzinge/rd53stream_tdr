#include <util/Util.h>
#include <encode/interface/Encoder.h>
#include <util/LUT.h>
#include <cmath>
#include <assert.h>
#include <iostream>

using namespace std;

int Encoder::find_last_qrow (IntMatrix& matrix, uint32_t ccol)
{
    int last_row = -1;

    for (uint32_t r = 0; r < matrix.size_row(); r++)
    {
        for (uint32_t c = ccol * this->col_factor; c < (ccol + 1) *this->col_factor; c++)
        {
            if (matrix.data[r][c] > 0)
            {
                last_row = r;
                break;
            }
        }
    }

    if (last_row < 0)
        return last_row;

    else
        //this is the row in units of quarter core size, so 2
        return last_row / this->row_factor;
};




/**
 * Build QCore objects from a pixel matrix
 */
vector<QCore> Encoder::qcores (IntMatrix& matrix, int event, int module, int chip, std::ostream& stream)
{
    vector<QCore> qcores;

    // Loop over core columns (ccol)
    uint32_t nccol = matrix.size_col() / this->col_factor;

    //assert (nccol < 54);

    for (uint32_t ccol = 0; ccol < nccol ; ccol++)
    {
        int qcrow_prev = -2;
        int last_qcrow = find_last_qrow (matrix, ccol);

        if (last_qcrow < 0)
            continue;

        // Loop over quarter core rows (qcrow)
        for (uint32_t qcrow = 0; qcrow <= last_qcrow; qcrow++ )
        {
            // The native representation of the QCore is
            // a 16-bit vector of ADC values, which are
            // simply copied from the matrix
            vector<ADC> qcore_adcs;
            qcore_adcs.reserve (16);
            bool any_nonzero = false;

            for (uint32_t j = 0; j < this->row_factor; j++)
            {
                for (uint32_t i = ccol * this->col_factor; i < (ccol + 1) *this->col_factor; i++)
                {
                    //stream <<   "  row: " << qcrow* this->row_factor + j << " Col " << i << std::endl;
                    //assert (qcrow * this->row_factor + j < matrix.size_row());
                    uint32_t value = matrix.data[qcrow * this->row_factor + j][i];
                    any_nonzero |= value > 0;
                    qcore_adcs.push_back (value);
                }
            }

            assert (qcore_adcs.size() == 16);

            // Ignore qcores that do not have any hits
            if (!any_nonzero)
                continue;

            // Meta information about the qcore
            // necessary for stream building
            bool isneighbour = (qcrow_prev + 1 == qcrow);
            qcrow_prev = qcrow;
            bool islast = (qcrow == last_qcrow);
            //we increment the col by 1 as the valid address values for core_columns are 1 to 54
            QCore qcore (event, module, chip, ccol + 1, qcrow, isneighbour, islast, qcore_adcs);
            qcores.push_back (qcore);
        }
    }

    return qcores;
}
