#ifndef CHIPIDENTIFIER_H__
#define CHIPIDENTIFIER_H__

#include<iostream>

class ChipIdentifier{
    public:
        ChipIdentifier(uint32_t disk, uint32_t ring, uint32_t module, uint32_t chip);
        ChipIdentifier operator < (ChipIdentifier const &obj);
        //return this->disk<obj.disk etc;

    private:
        uint32_t disk;
        uint32_t ring;
        uint32_t module;
        uint32_t chip;
        uint32_t quarter;
        uint32_t dtc;
    
};


#endif
