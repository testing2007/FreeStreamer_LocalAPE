
#pragma once
#include "All.h"
#include "IO.h"
#include "NoWindows.h"
#include <stdio.h>

namespace APE_MONKEY
{



using namespace APE_MONKEY;
    
class CStdLibFileIO// : public CIO
{
public:
    // construction / destruction
    CStdLibFileIO();
    ~CStdLibFileIO();

    // open / close
    int Open(LPCTSTR pName, BOOL bOpenReadOnly = FALSE);
    int Close();
    
    // read / write
    int Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead);
    int Write(const void * pBuffer, unsigned int nBytesToWrite, unsigned int * pBytesWritten);
    
    // seek
    int Seek(long long nDistance, unsigned int nMoveMode);
    
    // other functions
    int SetEOF();

    // creation / destruction
    int Create(const wchar_t * pName);
    int Delete();

    // attributes
    long long GetPosition();
    long long GetSize();
    int GetName(char * pBuffer);
    int GetHandle();

private:
    
    char m_cFileName[MAX_PATH];
    BOOL m_bReadOnly;
    FILE * m_pFile;
};

}

