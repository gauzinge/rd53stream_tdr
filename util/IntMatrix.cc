#include <IntMatrix.h>

IntMatrix::IntMatrix(uint32_t nrow, uint32_t ncol)
{
    this->rows = nrow;
    this->cols = ncol;
    this->data.reserve(this->rows);
    for(size_t row = 0; row< this->rows; row++)
    {
        std::vector<ADC> row_vec(this->cols,0);
        assert(row_vec.size() == this->cols);
        this->data.push_back(row_vec);
    }
    assert(this->data.size()==this->rows);
}

size_t IntMatrix::size_row()
{
    return this->rows();
}

size_t IntMatrix::size_col()
{
    return this->cols();
}

std::pair<size_t, size_t> IntMatrix::size()
{
    return std::make_pair(this->rows, this->cols);
}

void IntMatrix::clear()
{
    for(auto row_vec : this->data)
    {
        for(auto val : row:vec)
            val = 0;
    }
}

void IntMatrix::fill(uint32_t row, uint32_t col, uint32_t adc)
{
    assert(row < this->rows);
    assert(col < this->cols);
    this->data[row][col]=adc;
}

void IntMatrix::convertPitch_andFill(uint32_t row, uint32_t col, uint32_t adc)
{
    if(row&1 == 0)//this is a simple check if row is an even number, alternative (row%2 == 0)
    {
        row/=2;
        col *=2;
        col+=1;
    }
    else// odd row number
    {
        row-=1;
        row/=2;
        col*=2;
    }

    assert(row < this->rows);
    assert(col < this->cols);
    this->data[row][col]=adc;
}
IntMatrix IntMatrix::submatrix(uint32_t chip)
{
    IntMatrix aMatrix(336, 432);


    return aMatrix;
}

IntMatrix IntMatrix::submatrix(uint32_t row, uint32_t nrow, uint32_t col, uint32_t ncol)
{
    IntMatrix aMatrix(nrow, ncol);


    return aMatrix;
        
}
