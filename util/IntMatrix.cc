#include <util/IntMatrix.h>

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

IntMatrix::IntMatrix()
{
    //this->rows = 0;
    //this->cols = 0;
    //std::vector<std::vector<ADC>> vec;
    //this->data = vec;
}

IntMatrix::IntMatrix(const IntMatrgitix &matrix)
{
    rows = matrix.rows;
    cols = matrix.cols;
    data = matrix.data;
}

size_t IntMatrix::size_row()
{
    return this->rows;
}

size_t IntMatrix::size_col()
{
    return this->cols;
}

std::pair<size_t, size_t> IntMatrix::size()
{
    return std::make_pair(this->rows, this->cols);
}

void IntMatrix::clear()
{
    for(auto row_vec : this->data)
    {
        for(auto val : row_vec)
            val = 0;
    }
}

void IntMatrix::fill(uint32_t row, uint32_t col, uint32_t adc)
{
    //assert(row < this->rows);
    //assert(col < this->cols);
    //if(row > this->rows)
    //std::cout << "Warning, row " << row << " larger than matrix: " <<rows << std::endl;
    //if(col > this->cols)
    //std::cout << "Warning, col " << col << " larger than matrix: " <<cols << std::endl;
    if(row < this->rows && col < this->cols)
        this->data[row][col]=adc;
    //else {
    //    std::cout << "Warning row or col are largen than the matrix " << row << ", " << col << std::endl;
    //}
}

//for some weird reason the digis have larger row and col numbers than the actual chip
void IntMatrix::convertPitch_andFill(uint32_t row, uint32_t col, uint32_t adc)
{
    uint32_t arow=0;
    uint32_t acol = 0;
    if(row%2 == 0)//this is a simple check if row is an even number, alternative (row%2 == 0)
    {
        arow=row/2;
        acol =2*col+1;
    }
    else// odd row number
    {
        arow=(row-1)/2;
        acol=2*col;
    }

    //assert(row < this->rows);
    //assert(col < this->cols);
    //if(arow > this->rows)
    //std::cout << "Warning, row " << arow << " larger than matrix: " <<rows << std::endl;
    //if(acol > this->cols)
    //std::cout << "Warning, col " << acol << " larger than matrix: " <<cols << std::endl;
    if(arow < this->rows && acol < this->cols)
        this->data[arow][acol]=adc;
}

uint32_t IntMatrix::value(uint32_t row, uint32_t col)
{
    assert(row < this->rows);
    assert(col < this->cols);
    return this->data.at(row).at(col);
}

std::vector<std::pair<uint32_t,uint32_t>> IntMatrix::hits()
{
    std::vector<std::pair<uint32_t,uint32_t>> result;
    for(int col = 0; col < this->cols; col++) {
        for(int row = 0; row < this->rows; row++) {
            ADC adc_val = this->data.at(row).at(col);
            if (adc_val) result.push_back(std::make_pair(col,row));
        }
    }
    return result;
}

void IntMatrix::calculate_size()
{
    this->rows = this->data.size();
    this->cols = this->data.at(0).size();
}


IntMatrix IntMatrix::submatrix(uint32_t chip)
{
    size_t nrow = this->rows/2;
    size_t ncol = this->cols/2;

    IntMatrix aMatrix;
    assert(chip < 4);
    if(chip ==0)
    {
        auto first = this->data.begin();
        auto last = this->data.begin()+nrow;
        aMatrix.data=std::vector<std::vector<ADC>>(first, last);
        for(auto& row_vec : aMatrix.data)
            row_vec.resize(ncol);
    }
    if(chip ==1)
    {
        auto first = this->data.begin();
        auto last = this->data.begin()+nrow;
        aMatrix.data=std::vector<std::vector<ADC>>(first, last);
        for(auto& row_vec : aMatrix.data)
            row_vec.erase(row_vec.begin(), row_vec.begin()+ncol);
    }
    if(chip ==2)
    {
        auto first = this->data.begin()+nrow;
        auto last = this->data.end();
        aMatrix.data=std::vector<std::vector<ADC>>(first, last);
        for(auto& row_vec : aMatrix.data)
            row_vec.resize(ncol);
    }
    if(chip ==3)
    {
        auto first = this->data.begin()+nrow;
        auto last = this->data.end();
        aMatrix.data=std::vector<std::vector<ADC>>(first, last);
        for(auto& row_vec : aMatrix.data)
            row_vec.erase(row_vec.begin(), row_vec.begin()+ncol);
    }
    aMatrix.calculate_size();
    //std::cout << "Chip matrix for chip: " << chip << " has size " <<  "Rows: " << aMatrix.size_row() << " Cols: " << aMatrix.size_col() << std::endl;

    return aMatrix;
}

