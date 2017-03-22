#pragma once



namespace APE_MONKEY
{

class IAPEDecompress;
class CStdLibFileIO;

struct UNBIT_ARRAY_STATE
{
    uint32 k;
    uint32 nKSum;
};

enum DECODE_VALUE_METHOD
{
    DECODE_VALUE_METHOD_UNSIGNED_INT,
    DECODE_VALUE_METHOD_UNSIGNED_RICE,
    DECODE_VALUE_METHOD_X_BITS
};

class CUnBitArrayBase
{
public:
    // construction / destruction
    CUnBitArrayBase(int nFurthestReadByte);
    virtual ~CUnBitArrayBase();
    
    // functions
    virtual int FillBitArray();
    virtual int FillAndResetBitArray(int nFileLocation = -1, int nNewBitIndex = 0);
        
    virtual void GenerateArray(int * pOutputArray, int nElements, int nBytesRequired = -1) {}
    virtual unsigned int DecodeValue(DECODE_VALUE_METHOD DecodeMethod, int nParam1 = 0, int nParam2 = 0) { return 0; }
    
    virtual void AdvanceToByteBoundary();

    virtual int DecodeValueRange(UNBIT_ARRAY_STATE & BitArrayState) { return 0; }
    virtual void FlushState(UNBIT_ARRAY_STATE & BitArrayState) { }
    virtual void FlushBitArray() { }
    virtual void Finalize() { }
    
protected:
    virtual int CreateHelper(CStdLibFileIO * pIO, int nBytes, int nVersion);
    virtual uint32 DecodeValueXBits(uint32 nBits);
    
    uint32 m_nElements;
    uint32 m_nBytes;
    uint32 m_nBits;
    uint32 m_nGoodBytes;
    
    int m_nVersion;
    CStdLibFileIO * m_pIO;
    int m_nFurthestReadByte;

    uint32 m_nCurrentBitIndex;
    uint32 * m_pBitArray;
};

CUnBitArrayBase * CreateUnBitArray(IAPEDecompress * pAPEDecompress, int nVersion);

}
