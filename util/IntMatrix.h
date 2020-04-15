#ifndef INTMATRIX_H
#define INTMATRIX_H
#include <vector>
#include <assert.h>

typedef uint32_t ADC;
//typedef vector<vector<ADC>> IntMatrix;

class IntMatrix {
    public:
        IntMatrix(uint32_t nrow, uint32_t ncol);
        size_t size_row();
        size_t size_col();
        std::pair<size_t, size_t> size();
        void clear();
        void fill(uint32_t row, uint32_t col, uint32_t adc);
        void convertPitch_andFill(uint32_t row, uint32_t col, uint32_t adc);
        IntMatrix submatrix(uint32_t chip);
        IntMatrix submatrix(uint32_t row, uint32_t nrow, uint32_t col, uint32_t ncol);

    private:
        std::vector<std::vector<ADC>> data;
        size_t rows;
        size_t cols;
};
#endif
