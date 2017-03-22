/*
 * This file is part of the FreeStreamer project,
 * (C)Copyright 2011-2015 Matias Muhonen <mmu@iki.fi>
 * See the file ''LICENSE'' for using the code.
 *
 * https://github.com/muhku/FreeStreamer
 */

#ifndef ASTREAMER_FILE_STREAM_H
#define ASTREAMER_FILE_STREAM_H

#import "input_stream.h"
#import "id3_parser.h"
#import "MACLib.h"
#import <CoreAudio/CoreAudio.h>
#import <dispatch/dispatch.h>
#include <pthread.h>

    using namespace APE_MONKEY;
namespace astreamer {

class File_Stream : public Input_Stream {
private:
    
    File_Stream(const File_Stream&);
    File_Stream& operator=(const File_Stream&);
    
    CFURLRef m_url;
    CFReadStreamRef m_readStream;
    bool m_scheduledInRunLoop;
    bool m_readPending;
    Input_Stream_Position m_position;
    
    UInt8 *m_fileReadBuffer;
    
    ID3_Parser *m_id3Parser;
    
    CFStringRef m_contentType;
    
    static void readCallBack(CFReadStreamRef stream, CFStreamEventType eventType, void *clientCallBackInfo);
    
public:
    File_Stream();
    virtual ~File_Stream();
    
    Input_Stream_Position position();
    
    CFStringRef contentType();
    void setContentType(CFStringRef contentType);
    size_t contentLength();
    
    bool open();
    bool open(const Input_Stream_Position& position);
    void close();
    
    void setScheduledInRunLoop(bool scheduledInRunLoop);
    
    void setUrl(CFURLRef url);
    
    static bool canHandleUrl(CFURLRef url);
    
    /* ID3_Parser_Delegate */
    void id3metaDataAvailable(std::map<CFStringRef,CFStringRef> metaData);
    void id3tagSizeAvailable(UInt32 tagSize);
};
    
    class APEFile_Stream : public APEInput_Stream {
    private:
        
        APEFile_Stream(const APEFile_Stream&);
        APEFile_Stream& operator=(const APEFile_Stream&);
        
        CFURLRef m_url;
        Input_Stream_Position m_position;
        
        UInt8 *m_fileReadBuffer;
        
        APE_MONKEY::IAPEDecompress *m_pDecompress;
        AudioStreamBasicDescription m_dstFormat;
        
        CFStringRef m_contentType;
        
        static void readCallBack(CFReadStreamRef stream, CFStreamEventType eventType, void *clientCallBackInfo);
    
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        dispatch_queue_t m_queue;
        bool m_scheduledInRunLoop;
        float m_durationInSeconds;//播放时长
        float m_sampleRate;
        float m_totalBlocks;
        
        BOOL IsTryLockSucc();
        
    public:

        APEFile_Stream();
        virtual ~APEFile_Stream();
        
        size_t durationInSeconds();
        size_t totalBlocks();
        size_t sampleRate();
        
        Input_Stream_Position position();
        
        CFStringRef contentType();
        void setContentType(CFStringRef contentType);
        size_t contentLength();
        
        int seek(size_t nBlockOffset);
        bool open();
        bool open(const Input_Stream_Position& position);
        void close();
        
        void setScheduledInRunLoop(bool scheduledInRunLoop);
        
        void setUrl(CFURLRef url);
        
        static bool canHandleUrl(CFURLRef url);
    };
    
} // namespace astreamer

#endif // ASTREAMER_FILE_STREAM_H