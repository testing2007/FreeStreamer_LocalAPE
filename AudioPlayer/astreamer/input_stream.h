/*
 * This file is part of the FreeStreamer project,
 * (C)Copyright 2011-2015 Matias Muhonen <mmu@iki.fi>
 * See the file ''LICENSE'' for using the code.
 *
 * https://github.com/muhku/FreeStreamer
 */

#ifndef ASTREAMER_INPUT_STREAM_H
#define ASTREAMER_INPUT_STREAM_H

#import "id3_parser.h"
#import <CoreAudio/CoreAudio.h>

namespace astreamer {

class Input_Stream_Delegate;
    
struct Input_Stream_Position {
    UInt64 start;
    UInt64 end;
};
    
class Input_Stream : public ID3_Parser_Delegate {
public:
    Input_Stream();
    virtual ~Input_Stream();
    
    Input_Stream_Delegate* m_delegate;
    
    virtual Input_Stream_Position position() = 0;
    
    virtual CFStringRef contentType() = 0;
    virtual size_t contentLength() = 0;
    
    virtual bool open() = 0;
    virtual bool open(const Input_Stream_Position& position) = 0;
    virtual void close() = 0;
    
    virtual void setScheduledInRunLoop(bool scheduledInRunLoop) = 0;
    
    virtual void setUrl(CFURLRef url) = 0;
};
    
    class APEInput_Stream : public Input_Stream {
    public:
        APEInput_Stream();
        virtual ~APEInput_Stream();
        
//        Input_Stream_Delegate* m_delegate;
        virtual size_t durationInSeconds() = 0;
        virtual size_t totalBlocks() = 0;
        virtual size_t sampleRate() = 0;

        virtual Input_Stream_Position position() = 0;
        
        virtual CFStringRef contentType() = 0;
        virtual size_t contentLength() = 0;
        
        virtual bool open() = 0;
        virtual bool open(const Input_Stream_Position& position) = 0;
        virtual void close() = 0;
        
        virtual void setScheduledInRunLoop(bool scheduledInRunLoop) = 0;
        
        virtual void setUrl(CFURLRef url) = 0;
        
        virtual void id3metaDataAvailable(std::map<CFStringRef,CFStringRef> metaData)
        {
            //do nothing
        }
        virtual void id3tagSizeAvailable(UInt32 tagSize)
        {
            //do nothing
        }
    };
    

class Input_Stream_Delegate {
public:
    virtual void streamIsReadyRead(bool bUnsupportCodec=false, AudioStreamBasicDescription* dstFormat=NULL) = 0;
    virtual void streamHasBytesAvailable(UInt8 *data, UInt32 numBytes) = 0;
    virtual void streamEndEncountered() = 0;
    virtual void streamErrorOccurred(CFStringRef errorDesc) = 0;
    virtual void streamMetaDataAvailable(std::map<CFStringRef,CFStringRef> metaData) = 0;
    virtual void streamMetaDataByteSizeAvailable(UInt32 sizeInBytes) = 0;
};

} // namespace astreamer

#endif // ASTREAMER_INPUT_STREAM_Hs