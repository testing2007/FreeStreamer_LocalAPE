#pragma once

namespace APE_MONKEY
{

#include "RollBuffer.h"
#include "NoWindows.h"
#define NN_WINDOW_ELEMENTS    512

class CNNFilter
{
public:
    CNNFilter(int nOrder, int nShift, int nVersion);
    ~CNNFilter();

    int Compress(int nInput);
    int Decompress(int nInput);
    void Flush();

private:
    int m_nOrder;
    int m_nShift;
    int m_nVersion;
    BOOL m_bSSEAvailable;
    int m_nRunningAverage;

    APE_MONKEY::CRollBuffer<short> m_rbInput;
    APE_MONKEY::CRollBuffer<short> m_rbDeltaM;

    short * m_paryM;

    __forceinline short GetSaturatedShortFromInt(int nValue) const
    {
        return short((nValue == short(nValue)) ? nValue : (nValue >> 31) ^ 0x7FFF);
    }

    __forceinline void Adapt(short * pM, short * pAdapt, int nDirection, int nOrder);
    __forceinline int CalculateDotProduct(short * pA, short * pB, int nOrder);
    
    __forceinline void AdaptSSE(short * pM, short * pAdapt, int nDirection, int nOrder);
    __forceinline int CalculateDotProductSSE(short * pA, short * pB, int nOrder);
};

}
