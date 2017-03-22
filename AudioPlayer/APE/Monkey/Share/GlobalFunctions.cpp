#include "All.h"
#include "GlobalFunctions.h"
#include "StdLibFileIO.h"
#include "CharacterHelper.h"

namespace APE_MONKEY
{

int ReadSafe(CStdLibFileIO * pIO, void * pBuffer, int nBytes)
{
    unsigned int nBytesRead = 0;
    int nRetVal = pIO->Read(pBuffer, nBytes, &nBytesRead);
    if (nRetVal == ERROR_SUCCESS)
    {
        if (nBytes != int(nBytesRead))
            nRetVal = ERROR_IO_READ;
    }

    return nRetVal;
}

int WriteSafe(CStdLibFileIO * pIO, void * pBuffer, int nBytes)
{
    unsigned int nBytesWritten = 0;
    int nRetVal = pIO->Write(pBuffer, nBytes, &nBytesWritten);
    if (nRetVal == ERROR_SUCCESS)
    {
        if (nBytes != int(nBytesWritten))
            nRetVal = ERROR_IO_WRITE;
    }

    return nRetVal;
}

bool FileExists(wchar_t * pFilename)
{    
    if (0 == wcscmp(pFilename, L"-")  ||  0 == wcscmp(pFilename, L"/dev/stdin"))
        return true;

    CSmartPtr<char> spANSI(CAPECharacterHelper::GetANSIFromUTF16(pFilename), TRUE);

    struct stat b;

    if (stat(spANSI, &b) != 0)
        return false;

    if (!S_ISREG(b.st_mode))
        return false;

    return true;

}

void * AllocateAligned(int nBytes, int nAlignment)
{
#ifdef _MSC_VER
    return _aligned_malloc(nBytes, nAlignment);
#else
    void * pMemory = NULL;
    posix_memalign(&pMemory, nAlignment, nBytes);
    return pMemory;
#endif
}

void FreeAligned(void * pMemory)
{
#ifdef _MSC_VER
    _aligned_free(pMemory);
#else
    free(pMemory);
#endif
}

bool GetSSEAvailable()
{
    bool bSSE = false;
#ifdef _MSC_VER
    #define CPU_SSE2 (1 << 26)

    int cpuInfo[4] = { 0 };
    __cpuid(cpuInfo, 0);

    int nIds = cpuInfo[0];
    if (nIds >= 1)
    {
        __cpuid(cpuInfo, 1);
        bSSE = !!(cpuInfo[3] & CPU_SSE2);
    }
#else
    // TODO: fill this in
#endif

    return bSSE;
}

}