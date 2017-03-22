#pragma once

namespace APE_MONKEY
{

/*************************************************************************************
Definitions
*************************************************************************************/
class CStdLibFileIO;

/*************************************************************************************
Read / Write from an IO source and return failure if the number of bytes specified
isn't read or written
*************************************************************************************/
int ReadSafe(CStdLibFileIO * pIO, void * pBuffer, int nBytes);
int WriteSafe(CStdLibFileIO * pIO, void * pBuffer, int nBytes);

/*************************************************************************************
Checks for the existence of a file
*************************************************************************************/
bool FileExists(wchar_t * pFilename);

/*************************************************************************************
Allocate aligned memory
*************************************************************************************/
void * AllocateAligned(int nBytes, int nAlignment);
void FreeAligned(void * pMemory);

/*************************************************************************************
Test for CPU features
*************************************************************************************/
bool GetSSEAvailable();

}
