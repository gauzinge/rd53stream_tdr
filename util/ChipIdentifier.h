#ifndef CHIPIDENTIFIER_H__
#define CHIPIDENTIFIER_H__

#include<iostream>

class ChipIdentifier
{
  public:
    ChipIdentifier (uint32_t disk, uint32_t ring, uint32_t module, uint32_t chip);
    void print() const;

    //protected:
    uint32_t mdisk;
    uint32_t mring;
    uint32_t mmodule;
    uint32_t mchip;
    uint32_t mquarter;
    uint32_t mdtc;
    double mlinkfactor;

};


inline bool operator < (const ChipIdentifier& obj1, const ChipIdentifier& obj2)
{
    if (obj1.mdisk != obj2.mdisk) return obj1.mdisk < obj2.mdisk;
    else if (obj1.mring != obj2.mring) return obj1.mring < obj2.mring;
    else if (obj1.mmodule != obj2.mmodule) return obj1.mmodule < obj2.mmodule;
    else return obj1.mchip < obj2.mchip;
}
#endif
