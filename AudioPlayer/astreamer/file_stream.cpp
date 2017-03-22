/*
 * This file is part of the FreeStreamer project,
 * (C)Copyright 2011-2015 Matias Muhonen <mmu@iki.fi>
 * See the file ''LICENSE'' for using the code.
 *
 * https://github.com/muhku/FreeStreamer
 */

#include "file_stream.h"
#include <APE/Monkey/Share/CharacterHelper.h>
#include "Stream_Configuration.h"
#include "player_debug.h"
#include "URLDecoder.h"

namespace astreamer {
    
File_Stream::File_Stream() :
    m_url(0),
    m_readStream(0),
    m_scheduledInRunLoop(false),
    m_readPending(false),
    m_fileReadBuffer(0),
    m_id3Parser(new ID3_Parser()),
    m_contentType(0)
{
    m_id3Parser->m_delegate = this;
}
    
File_Stream::~File_Stream()
{
    close();
    
    if (m_fileReadBuffer) {
        delete [] m_fileReadBuffer, m_fileReadBuffer = 0;
    }
    
    if (m_url) {
        CFRelease(m_url), m_url = 0;
    }
    
    delete m_id3Parser, m_id3Parser = 0;
    
    if (m_contentType) {
        CFRelease(m_contentType);
    }
}
    
Input_Stream_Position File_Stream::position()
{
    return m_position;
}
    
CFStringRef File_Stream::contentType()
{
    if (m_contentType) {
        // Use the provided content type
        return m_contentType;
    }
    
    // Try to resolve the content type from the file
    
    CFStringRef contentType = CFSTR("");
    CFStringRef pathComponent = 0;
    CFIndex len = 0;
    CFRange range;
    CFStringRef suffix = 0;
    
    if (!m_url) {
        goto done;
    }
    
    pathComponent = CFURLCopyLastPathComponent(m_url);
    
    if (!pathComponent) {
        goto done;
    }
    
    len = CFStringGetLength(pathComponent);
    
    if (len > 5) {
        range.length = 4;
        range.location = len - 4;
        
        suffix = CFStringCreateWithSubstring(kCFAllocatorDefault,
                                             pathComponent,
                                             range);
        
        if (!suffix) {
            goto done;
        }
        
        // TODO: we should do the content-type resolvation in a better way.
        if (CFStringCompare(suffix, CFSTR(".mp3"), 0) == kCFCompareEqualTo) {
            contentType = CFSTR("audio/mpeg");
        } else if (CFStringCompare(suffix, CFSTR(".m4a"), 0) == kCFCompareEqualTo) {
            contentType = CFSTR("audio/x-m4a");
        } else if (CFStringCompare(suffix, CFSTR(".mp3"), 0) == kCFCompareEqualTo) {
            contentType = CFSTR("audio/mp4");
        } else if (CFStringCompare(suffix, CFSTR(".aac"), 0) == kCFCompareEqualTo) {
            contentType = CFSTR("audio/aac");
        }
    }
    
done:
    if (pathComponent) {
        CFRelease(pathComponent);
    }
    if (suffix) {
        CFRelease(suffix);
    }
    
    return contentType;
}
    
void File_Stream::setContentType(CFStringRef contentType)
{
    if (m_contentType) {
        CFRelease(m_contentType), m_contentType = 0;
    }
    if (contentType) {
        m_contentType = CFStringCreateCopy(kCFAllocatorDefault, contentType);
    }
}
    
size_t File_Stream::contentLength()
{
    CFNumberRef length = NULL;
    CFErrorRef err = NULL;

    if (CFURLCopyResourcePropertyForKey(m_url, kCFURLFileSizeKey, &length, &err)) {
        CFIndex fileLength;
        if (CFNumberGetValue(length, kCFNumberCFIndexType, &fileLength)) {
            CFRelease(length);
            
            return fileLength;
        }
    }
    return 0;
}
    
bool File_Stream::open()
{
    Input_Stream_Position position;
    position.start = 0;
    position.end = 0;
    
    m_id3Parser->reset();
    
    return open(position);
}
    
bool File_Stream::open(const Input_Stream_Position& position)
{
    bool success = false;
    CFStreamClientContext CTX = { 0, this, NULL, NULL, NULL };
    
    /* Already opened a read stream, return */
    if (m_readStream) {
        goto out;
    }
    
    if (!m_url) {
        goto out;
    }
    
    /* Reset state */
    m_position = position;
    
    m_readPending = false;
	
    /* Failed to create a stream */
    if (!(m_readStream = CFReadStreamCreateWithFile(kCFAllocatorDefault, m_url))) {
        goto out;
    }
    
    if (m_position.start > 0) {
        CFNumberRef position = CFNumberCreate(0, kCFNumberLongLongType, &m_position.start);
        CFReadStreamSetProperty(m_readStream, kCFStreamPropertyFileCurrentOffset, position);
        CFRelease(position);
    }
    
    
    if (!CFReadStreamSetClient(m_readStream, kCFStreamEventHasBytesAvailable |
                               kCFStreamEventEndEncountered |
                               kCFStreamEventErrorOccurred, readCallBack, &CTX)) {
        CFRelease(m_readStream), m_readStream = 0;
        goto out;
    }
    
    setScheduledInRunLoop(true);
    
    if (!CFReadStreamOpen(m_readStream)) {
        /* Open failed: clean */
        CFReadStreamSetClient(m_readStream, 0, NULL, NULL);
        setScheduledInRunLoop(false);
        if (m_readStream) {
            CFRelease(m_readStream), m_readStream = 0;
        }
        goto out;
    }
    
    success = true;
    
out:
    
    if (success) {
        if (m_delegate) {
            m_delegate->streamIsReadyRead();
        }
    }
    return success;
}
    
void File_Stream::close()
{
    /* The stream has been already closed */
    if (!m_readStream) {
        return;
    }
    
    CFReadStreamSetClient(m_readStream, 0, NULL, NULL);
    setScheduledInRunLoop(false);
    CFReadStreamClose(m_readStream);
    CFRelease(m_readStream), m_readStream = 0;
}
    
void File_Stream::setScheduledInRunLoop(bool scheduledInRunLoop)
{
    /* The stream has not been opened, or it has been already closed */
    if (!m_readStream) {
        return;
    }
    
    /* The state doesn't change */
    if (m_scheduledInRunLoop == scheduledInRunLoop) {
        return;
    }
    
    if (m_scheduledInRunLoop) {
        CFReadStreamUnscheduleFromRunLoop(m_readStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
    } else {
        if (m_readPending) {
            m_readPending = false;
            
            readCallBack(m_readStream, kCFStreamEventHasBytesAvailable, this);
        }
        
        CFReadStreamScheduleWithRunLoop(m_readStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
    }
    
    m_scheduledInRunLoop = scheduledInRunLoop;
}
    
void File_Stream::setUrl(CFURLRef url)
{
    if (m_url) {
        CFRelease(m_url);
    }
    if (url) {
        m_url = (CFURLRef)CFRetain(url);
    } else {
        m_url = NULL;
    }
}
    
bool File_Stream::canHandleUrl(CFURLRef url)
{
    if (!url) {
        return false;
    }
    
    CFStringRef scheme = CFURLCopyScheme(url);
    
    if (scheme) {
        if (CFStringCompare(scheme, CFSTR("file"), 0) == kCFCompareEqualTo) {
            CFRelease(scheme);
            // The only scheme we claim to handle are the local files
            return true;
        }
        
        CFRelease(scheme);
    }
    
    // We don't handle anything else but local files
    return false;
}
    
/* ID3_Parser_Delegate */
void File_Stream::id3metaDataAvailable(std::map<CFStringRef,CFStringRef> metaData)
{
    if (m_delegate) {
        m_delegate->streamMetaDataAvailable(metaData);
    }
}
    
void File_Stream::id3tagSizeAvailable(UInt32 tagSize)
{
    if (m_delegate) {
        m_delegate->streamMetaDataByteSizeAvailable(tagSize);
    }
}
    
void File_Stream::readCallBack(CFReadStreamRef stream, CFStreamEventType eventType, void *clientCallBackInfo)
{
    File_Stream *THIS = static_cast<File_Stream*>(clientCallBackInfo);
    
    switch (eventType) {
        case kCFStreamEventHasBytesAvailable: {
            if (!THIS->m_fileReadBuffer) {
                THIS->m_fileReadBuffer = new UInt8[1024];
            }
            
            while (CFReadStreamHasBytesAvailable(stream)) {
                if (!THIS->m_scheduledInRunLoop) {
                    /*
                     * This is critical - though the stream has data available,
                     * do not try to feed the audio queue with data, if it has
                     * indicated that it doesn't want more data due to buffers
                     * full.
                     */
                    THIS->m_readPending = true;
                    break;
                }
                
                CFIndex bytesRead = CFReadStreamRead(stream, THIS->m_fileReadBuffer, 1024);
                
                if (CFReadStreamGetStatus(stream) == kCFStreamStatusError ||
                    bytesRead < 0) {
                    
                    if (THIS->m_delegate) {
                        CFStringRef reportedNetworkError = NULL;
                        CFErrorRef streamError = CFReadStreamCopyError(stream);
                        
                        if (streamError) {
                            CFStringRef errorDesc = CFErrorCopyDescription(streamError);
                            
                            if (errorDesc) {
                                reportedNetworkError = CFStringCreateCopy(kCFAllocatorDefault, errorDesc);
                                
                                CFRelease(errorDesc);
                            }
                            
                            CFRelease(streamError);
                        }
                        
                        THIS->m_delegate->streamErrorOccurred(reportedNetworkError);
                        if (reportedNetworkError) {
                            CFRelease(reportedNetworkError);
                        }
                    }
                    break;
                }
                
                if (bytesRead > 0) {
                    if (THIS->m_delegate) {
                        THIS->m_delegate->streamHasBytesAvailable(THIS->m_fileReadBuffer, (UInt32)bytesRead);
                    }
                    
                    if (THIS->m_id3Parser->wantData()) {
                        THIS->m_id3Parser->feedData(THIS->m_fileReadBuffer, (UInt32)bytesRead);
                    }
                }
            }
            
            break;
        }
        case kCFStreamEventEndEncountered: {
            if (THIS->m_delegate) {
                THIS->m_delegate->streamEndEncountered();
            }
            break;
        }
        case kCFStreamEventErrorOccurred: {
            if (THIS->m_delegate) {
                CFStringRef reportedNetworkError = NULL;
                CFErrorRef streamError = CFReadStreamCopyError(stream);
                
                if (streamError) {
                    CFStringRef errorDesc = CFErrorCopyDescription(streamError);
                    
                    if (errorDesc) {
                        reportedNetworkError = CFStringCreateCopy(kCFAllocatorDefault, errorDesc);
                        
                        CFRelease(errorDesc);
                    }
                    
                    CFRelease(streamError);
                }
                
                THIS->m_delegate->streamErrorOccurred(reportedNetworkError);
                if (reportedNetworkError) {
                    CFRelease(reportedNetworkError);
                }
            }
            break;
        }
    }
}
    
    
////////APEStream  begin//////////
    APEFile_Stream::APEFile_Stream() :
    m_url(0),
    m_scheduledInRunLoop(false),
    m_fileReadBuffer(0),
    m_pDecompress(NULL),
    m_contentType(0)
    {
    }
    
    APEFile_Stream::~APEFile_Stream()
    {
        close();
        
        if (m_fileReadBuffer) {
            delete [] m_fileReadBuffer, m_fileReadBuffer = 0;
        }
        
        if(m_pDecompress)
        {
            delete m_pDecompress;
            m_pDecompress = NULL;
        }
        if (m_url) {
            CFRelease(m_url), m_url = 0;
        }
        
        if (m_contentType) {
            CFRelease(m_contentType);
        }
        
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }
    
    Input_Stream_Position APEFile_Stream::position()
    {
        return m_position;
    }
    
    CFStringRef APEFile_Stream::contentType()
    {
        if (m_contentType) {
            // Use the provided content type
            return m_contentType;
        }
        
        // Try to resolve the content type from the file
        
        CFStringRef contentType = CFSTR("");
        CFStringRef pathComponent = 0;
        CFIndex len = 0;
        CFRange range;
        CFStringRef suffix4 = 0;
        CFStringRef suffix6 = 0;
        
        if (!m_url) {
            goto done;
        }
        
        pathComponent = CFURLCopyLastPathComponent(m_url);
        
        if (!pathComponent) {
            goto done;
        }
        
        len = CFStringGetLength(pathComponent);
        
        if (len > 6) {
            range.length = 4;
            range.location = len - 4;
            suffix4 = CFStringCreateWithSubstring(kCFAllocatorDefault,
                                                 pathComponent,
                                                 range);

            range.length = 6;
            range.location = len - 6;
            suffix6 = CFStringCreateWithSubstring(kCFAllocatorDefault,
                                                  pathComponent,
                                                  range);
            if (!suffix4 && !suffix6) {
                goto done;
            }
            
            // TODO: we should do the content-type resolvation in a better way.
            if ((CFStringCompare(suffix4, CFSTR(".mac"), 0) == kCFCompareEqualTo) ||
                (CFStringCompare(suffix4, CFSTR(".ape"), 0) == kCFCompareEqualTo) ||
                (CFStringCompare(suffix4, CFSTR(".APE"), 0) == kCFCompareEqualTo) ||
                (CFStringCompare(suffix6, CFSTR(".41000"), 0) == kCFCompareEqualTo)) {
                contentType = CFSTR("audio/ape");
            }
        }
        
    done:
        if (pathComponent) {
            CFRelease(pathComponent);
        }
        if (suffix4) {
            CFRelease(suffix4);
        }
        if (suffix6) {
            CFRelease(suffix6);
        }
        
        return contentType;
    }
    
    void APEFile_Stream::setContentType(CFStringRef contentType)
    {
        if (m_contentType) {
            CFRelease(m_contentType), m_contentType = 0;
        }
        if (contentType) {
            m_contentType = CFStringCreateCopy(kCFAllocatorDefault, contentType);
        }
    }
    
    size_t APEFile_Stream::contentLength()
    {
        CFNumberRef length = NULL;
        CFErrorRef err = NULL;
        
        if (CFURLCopyResourcePropertyForKey(m_url, kCFURLFileSizeKey, &length, &err)) {
            CFIndex fileLength;
            if (CFNumberGetValue(length, kCFNumberCFIndexType, &fileLength)) {
                CFRelease(length);
                
                return fileLength;
            }
        }
        return 0;
    }
    
    bool APEFile_Stream::open()
    {
        Input_Stream_Position position;
        position.start = 0;
        position.end = 0;
        
        return open(position);
    }
    
    bool APEFile_Stream::open(const Input_Stream_Position& position)
    {
        bool success = false;
        CFStreamClientContext CTX = { 0, this, NULL, NULL, NULL };
        
        if (!m_url) {
            goto out;
        }
        
        /* Reset state */
        m_position = position;
        success = true;
        
    out:
        
        if (success) {
            if(m_url!=NULL)
            {
                //CFStringRef CFURLGetString ( CFURLRef anURL );
                //CFStringRef strUrl = CFURLGetString(url);
                CFStringRef strCompleteUrl = CFURLGetString(m_url);
                CFRange range = CFRangeMake(7, CFStringGetLength(strCompleteUrl)-7);
                CFStringRef strUrl = CFStringCreateWithSubstring(NULL, strCompleteUrl, range);
                const char* cUrl = CFStringGetCStringPtr(strUrl, kCFStringEncodingUTF8);
                URLDecoder urlDec;
                std::string decodeURL = urlDec.decode(string(cUrl));
                CSmartPtr<str_utf16> fileNameUtf16 = APE_MONKEY::CAPECharacterHelper::GetUTF16FromANSI(decodeURL.c_str());
                int error = 0;
                m_pDecompress = CreateIAPEDecompress(fileNameUtf16, &error);
                if(m_pDecompress!=NULL)
                {
                    m_totalBlocks = m_pDecompress->GetInfo(APE_INFO_TOTAL_BLOCKS);
                    m_sampleRate = m_pDecompress->GetInfo(APE_INFO_SAMPLE_RATE);
                    m_durationInSeconds = m_totalBlocks/m_sampleRate;
                    int chanel = m_pDecompress->GetInfo(APE_INFO_CHANNELS);
                    int bps = m_pDecompress->GetInfo(APE_INFO_BITS_PER_SAMPLE);
                    
                    ASSERT(position.start<=position.end);
                    size_t nBlockOffset;
                    if(position.start==position.end && position.start==0)
                    {
                        nBlockOffset = 0;
                    }
                    else
                    {
                        nBlockOffset = ((position.start*1.0)/position.end) * m_totalBlocks;
                    }
                    m_pDecompress->Seek(nBlockOffset);
                    FillOutASBDForLPCM(m_dstFormat, m_sampleRate, chanel, bps, bps, false, false);
                }
                else
                {
                    if(m_delegate)
                    {
                        m_delegate->streamErrorOccurred(CFSTR("fail to open APE file"));
                    }
                }
            }
            
            if (m_delegate) {
                m_delegate->streamIsReadyRead(true, &m_dstFormat);
                
                //创建线程
                m_queue = dispatch_queue_create("com.wenyu.kwplayermac", DISPATCH_QUEUE_SERIAL);
                pthread_mutexattr_t attr;
                pthread_mutexattr_init(&attr);
                pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
                pthread_mutex_init(&mutex, &attr);
                
                pthread_cond_init(&cond, NULL);
                dispatch_async(m_queue, ^{
                    const int BLOCK_SIZE = 7168;
                    int nBlockDecoded = 0;
                    int bufferSize = Stream_Configuration::configuration()->bufferSize;
                    UInt8* pBuffer = new UInt8[bufferSize];
                    memset(pBuffer, 0, bufferSize);
                    int result  = ERROR_SUCCESS;
                    if(m_pDecompress!=NULL)
                    {
                        result = m_pDecompress->GetData((char*)pBuffer, BLOCK_SIZE, &nBlockDecoded);
                    }
                    while ((nBlockDecoded > 0 && result == ERROR_SUCCESS)) {
                        FS_TRACE("ZQ function %s, lock before\n", __PRETTY_FUNCTION__);
                        pthread_mutex_lock(&mutex);
                        FS_TRACE("ZQ function %s, lock after\n", __PRETTY_FUNCTION__);
                        if(!m_scheduledInRunLoop)
                        {
                            FS_TRACE("ZQ function %s, wait before\n", __PRETTY_FUNCTION__);
                            pthread_cond_wait(&cond, &mutex);
                            FS_TRACE("ZQ function %s, wait after\n", __PRETTY_FUNCTION__);
                        }
                        if(m_pDecompress==NULL)
                        {
                            FS_TRACE("ZQ function %s, (m_pDecompress==NULL) unlock before\n", __PRETTY_FUNCTION__);
                            pthread_mutex_unlock(&mutex);
                            FS_TRACE("ZQ function %s, (m_pDecompress==NULL) unlock after\n", __PRETTY_FUNCTION__);
                            break;
                        }
                        FS_TRACE("m_pDecompress=0x%p, if m_scheduledInRunLoop=true\n", m_pDecompress);
                        UInt32 actualBufferSize = nBlockDecoded * m_pDecompress->GetInfo(APE_MONKEY::APE_INFO_BYTES_PER_SAMPLE) * m_pDecompress->GetInfo(APE_MONKEY::APE_INFO_CHANNELS);
                        
                        if(m_delegate!=NULL)
                        {
                            FS_TRACE("ZQ function %s, streamHasBytesAvailable before\n", __PRETTY_FUNCTION__);
                            m_delegate->streamHasBytesAvailable(pBuffer, (UInt32)actualBufferSize);
                            FS_TRACE("ZQ function %s, streamHasBytesAvailable after\n", __PRETTY_FUNCTION__);
                        }
                        memset(pBuffer, 0, bufferSize);
                        result = m_pDecompress->GetData((char*)pBuffer, BLOCK_SIZE, &nBlockDecoded);
                        FS_TRACE("ZQ function %s, unlock before\n", __PRETTY_FUNCTION__);
                        pthread_mutex_unlock(&mutex);
                        FS_TRACE("ZQ function %s, unlock after\n", __PRETTY_FUNCTION__);
                    }
                    if(result!=ERROR_SUCCESS)
                    {
                        CFStringRef errorDesc = CFSTR("fail to decompress file");
                        if(m_delegate!=NULL)
                        {
                            FS_TRACE("ZQ function %s, streamErrorOccurred before\n", __PRETTY_FUNCTION__);
                            m_delegate->streamErrorOccurred(errorDesc);
                            FS_TRACE("ZQ function %s, streamErrorOccurred after\n", __PRETTY_FUNCTION__);
                        }
                    }
                    else
                    {
                        if(m_delegate!=NULL)
                        {
                            FS_TRACE("ZQ function %s, streamEndEncountered before\n", __PRETTY_FUNCTION__);
                            m_delegate->streamEndEncountered();
                            FS_TRACE("ZQ function %s, streamEndEncountered after\n", __PRETTY_FUNCTION__);
                        }
                    }
                    delete[] pBuffer;
                    pBuffer = NULL;
                });//end dispatch_async
                setScheduledInRunLoop(true);
            }
        }
        else
        {
            ASSERT(false);
        }
        return success;
    }
    
    void APEFile_Stream::close()
    {
        FS_TRACE("enter %s\n", __PRETTY_FUNCTION__);
        int nRet = 0;
        FS_TRACE("ZQ function %s, trylock after nRet =%d\n", __PRETTY_FUNCTION__, nRet);
        FS_TRACE("ZQ function %s, lock before\n", __PRETTY_FUNCTION__);
        if(IsTryLockSucc())
        {
            FS_TRACE("ZQ function %s, lock after\n", __PRETTY_FUNCTION__);
            m_scheduledInRunLoop = false;
            
            FS_TRACE("nRet=%d", nRet);
            if(m_pDecompress!=NULL)
            {
                delete m_pDecompress;
                m_pDecompress = NULL;
            }
            
            FS_TRACE("ZQ function %s, unlock before\n", __PRETTY_FUNCTION__);
            pthread_mutex_unlock(&mutex);
            FS_TRACE("ZQ function %s, unlock after\n", __PRETTY_FUNCTION__);
        }
        FS_TRACE("leave %s\n", __PRETTY_FUNCTION__);
    }
    
    BOOL APEFile_Stream::IsTryLockSucc()
    {
        static int maxTryMutexTimes = 5;
        int nCurTimes = 0;
        int nRet = pthread_mutex_trylock(&mutex);
        while (nRet!=0 || nCurTimes>=maxTryMutexTimes) {
            nRet =  pthread_mutex_trylock(&mutex);
            if(nRet==0)
            {
                break;
            }
            nCurTimes++;
        }
        return (nCurTimes<maxTryMutexTimes) ? TRUE : FALSE;
    }
    
    void APEFile_Stream::setScheduledInRunLoop(bool scheduledInRunLoop)
    {
        /* The stream has not been opened, or it has been already closed */
        if (!m_pDecompress) {
            return;
        }
        
        /* The state doesn't change */
        if (m_scheduledInRunLoop == scheduledInRunLoop) {
            return;
        }
        FS_TRACE("enter %s\n", __PRETTY_FUNCTION__);
        
        FS_TRACE("ZQ function %s, trylock before\n", __PRETTY_FUNCTION__);
        if(IsTryLockSucc())
        {
            FS_TRACE("ZQ function %s, trylock after\n", __PRETTY_FUNCTION__);
            
            m_scheduledInRunLoop = scheduledInRunLoop;
            FS_TRACE("ZQ function %s, signal before\n", __PRETTY_FUNCTION__);
            pthread_cond_signal(&cond);
            FS_TRACE("ZQ function %s, signal after\n", __PRETTY_FUNCTION__);
            
            FS_TRACE("ZQ function %s, unlock before\n", __PRETTY_FUNCTION__);
            pthread_mutex_unlock(&mutex);
            FS_TRACE("ZQ function %s, unlock after\n", __PRETTY_FUNCTION__);
        }
        FS_TRACE("leave %s\n", __PRETTY_FUNCTION__);
    }
    
    size_t APEFile_Stream::durationInSeconds()
    {
        return m_durationInSeconds;
    }
    
    size_t APEFile_Stream::totalBlocks()
    {
        return m_totalBlocks;
    }
    
    size_t APEFile_Stream::sampleRate()
    {
        return m_sampleRate;
    }
    
    int APEFile_Stream::seek(size_t nBlockOffset)
    {
        if(IsTryLockSucc())
        {
            if(m_pDecompress==NULL)
            {
                pthread_mutex_unlock(&mutex);
                return 0;
            }
            int nRet = m_pDecompress->Seek(nBlockOffset);
            pthread_mutex_unlock(&mutex);
            return nRet;
        }
        return 0;
    }

    void APEFile_Stream::setUrl(CFURLRef url)
    {
        CFStringRef strCompleteUrl = CFURLGetString(url);
        CFRange range = CFRangeMake(7, CFStringGetLength(strCompleteUrl)-7);
        CFStringRef strUrl = CFStringCreateWithSubstring(NULL, strCompleteUrl, range);
        if (m_url) {
            CFRelease(m_url);
        }
        if (url) {
            m_url = (CFURLRef)CFRetain(url);
        } else {
            m_url = NULL;
        }
    }
    
    bool APEFile_Stream::canHandleUrl(CFURLRef url)
    {
        if (!url) {
            return false;
        }
        
        CFStringRef scheme = CFURLCopyScheme(url);
        CFStringRef strUrl = CFURLGetString(url);
        
        if (scheme) {
            if (CFStringCompare(scheme, CFSTR("file"), 0) == kCFCompareEqualTo &&
                (CFStringHasSuffix(strUrl, CFSTR(".41000")) ||
                 CFStringHasSuffix(strUrl, CFSTR(".ape"))))
            {
                CFRelease(scheme);
                // The only scheme we claim to handle are the local files
                return true;
            }
            
            CFRelease(scheme);
        }
        
        // We don't handle anything else but local files
        return false;
    }
////////APEStream  end////////////
    
    
} // namespace astreamer