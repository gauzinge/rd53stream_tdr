#ifndef INTMATRIX_H
#define INTMATRIX_H
#include <vector>
#include <assert.h>
#include<iostream>

typedef uint32_t ADC;
//typedef vector<vector<ADC>> IntMatrix;

class IntMatrix {
    public:
        IntMatrix(uint32_t nrow, uint32_t ncol);
        IntMatrix();
        IntMatrix(const IntMatrix &matrix);
        size_t size_row();
        size_t size_col();
        std::pair<size_t, size_t> size();
        void calculate_size();
        void clear();
        void fill(uint32_t row, uint32_t col, uint32_t adc);
        void convertPitch_andFill(uint32_t row, uint32_t col, uint32_t adc);
        uint32_t value(uint32_t row, uint32_t col);
        IntMatrix submatrix(uint32_t chip);
        //IntMatrix submatrix(uint32_t row, uint32_t nrow, uint32_t col, uint32_t ncol);
        std::vector<std::vector<ADC>> data;

    private:
        size_t rows;
        size_t cols;
};
#endif
