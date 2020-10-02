#ifndef CHIPIDENTIFIER_H__
#define CHIPIDENTIFIER_H__

#include<iostream>

class ChipIdentifier
{
  public:
    ChipIdentifier () :
        mdisk (99),
        mring (99),
        mmodule (99),
        mchip (99)
    {;}

    ChipIdentifier (uint32_t disk, uint32_t ring, uint32_t module, uint32_t chip) :
        mdisk (disk),
        mring (ring),
        mmodule (module),
        mchip (chip)
    {
        if (mring == 1)
        {
            if (mmodule <= 5) mquarter = 1;
            else if (mmodule > 5  && mmodule <= 10) mquarter = 2;
            else if (mmodule > 10 && mmodule <= 15) mquarter = 3;
            else mquarter = 4;
        }

        if (mring == 2)
        {
            if (mmodule <= 7) mquarter = 1;
            else if (mmodule > 7  && mmodule <= 14) mquarter = 2;
            else if (mmodule > 14 && mmodule <= 21) mquarter = 3;
            else mquarter = 4;
        }

        if (mring == 3)
        {
            if (mmodule <= 9) mquarter = 1;
            else if (mmodule > 9  && mmodule <= 18) mquarter = 2;
            else if (mmodule > 18 && mmodule <= 27) mquarter = 3;
            else mquarter = 4;
        }

        if (mring == 4)
        {
            if (mmodule <= 11) mquarter = 1;
            else if (mmodule > 11  && mmodule <= 22) mquarter = 2;
            else if (mmodule > 22 && mmodule <= 33) mquarter = 3;
            else mquarter = 4;
        }

        if (mring == 5)
        {
            if (mmodule <= 12) mquarter = 1;
            else if (mmodule > 12  && mmodule <= 24) mquarter = 2;
            else if (mmodule > 24 && mmodule <= 36) mquarter = 3;
            else mquarter = 4;
        }

        //convention: if disk = 9 or disk = 10 and quarter =1 or quarter = 2 -> DTC = 1
        //               disk = 11or disk = 12 and quarter =1 or quarter = 2 -> DTC = 2
        //               disk = 9 or disk = 10 and quarter =3 or quarter = 4 -> DTC = 3
        //               disk = 11or disk = 12 and quarter =3 or quarter = 4 -> DTC = 4
        //side in file is always +z
        if (mdisk < 11)
        {
            if (mquarter == 1 || mquarter == 2) mdtc = 1;
            else mdtc = 3;
        }
        else if (mdisk > 10 && mdisk <= 12)
        {
            if (mquarter == 1 || mquarter == 2) mdtc = 2;
            else mdtc = 4;
        }

        //Override assignment specifically for D4R1 which will be connected to it's own DTC
        if (mdisk == 12 && mring == 1)
            mdtc = 5;

        if (mchip == 99) mlinkfactor = 99;
        else if (ring == 1 && (chip == 0 || chip == 1) ) mlinkfactor = 1.;
        else if (ring == 1 && (chip == 2 || chip == 3) ) mlinkfactor = .5;
        else if (ring == 2) mlinkfactor = .5;
        else if (ring > 2 ) mlinkfactor = .25;

    }

    // copy constructur
    ChipIdentifier (const ChipIdentifier &identifier) {
        mdisk = identifier.mdisk;
        mring = identifier.mring;
        mmodule = identifier.mmodule;
        mchip = identifier.mchip;
        mquarter = identifier.mquarter;
        mdtc = identifier.mdtc;
        mlinkfactor = identifier.mlinkfactor;
    }

    void print() const
    {
        std::cout << "D " << mdisk << " R " << mring << " M " << mmodule <<   " Q " << mquarter << " DTC " << mdtc << " C " << mchip << " ; " << mlinkfactor << std::endl;
    }

    //encode the disk, ring, module, # of chips in a word of type T
    template<typename T>
    T encode() const
    {
        //assert (sizeof (T) * 8 >= 16); //?
        uint16_t word = 0;

        //# of chips (2:0) 3b
        word |= (4 & 0x7);
        //module ID (8:3) 6b
        word |= (mmodule & 0x3F) << 3;
        //ring (11:9) 3b
        word |= (mring & 0x7) << 9;
        //disk (15:12) 4b
        word |= (mdisk & 0xF) << 12;
        //16b total

        return static_cast<T> (word);
        //else
        //{

        //}

    }

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
