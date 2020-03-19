#ifndef LUT_H
#define LUT_H

#include <vector>
#include <stdint.h>
#include <util/Util.h>
#include <iostream>
#include <assert.h>
using namespace std;


extern vector<bool> enc8(vector<bool> row);

extern vector<bool> enc2(vector<bool> row_or);

#endif // LUT_H