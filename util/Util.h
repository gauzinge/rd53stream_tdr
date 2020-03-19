#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <algorithm>    // std::reverse
using namespace std;

extern vector<bool> adc_to_binary(uint32_t adc);

extern vector<bool> int_to_binary(int an_int, int length);

extern int binary_to_int(vector<bool> bin);

extern std::pair<int,int> hits_in_row(vector<bool> encoded_row);

#endif // UTIL_H