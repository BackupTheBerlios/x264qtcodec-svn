/*
 *  x264Codec.h
 *  x264Codec
 *
 *  Created by Henry Mason on 5/23/05.
 *  Copyright 2005 Henry Mason. All rights reserved.
 *
 */

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>

#include "x264.h"

struct x264CodecComponentContext
{
    ComponentInstance   self;
    
    x264_t          *encoder;
    x264_param_t    encoderParams;
    x264_picture_t  sourcePicture;
    Boolean         sourcePictureIsAllocated;
        
    Handle          settings;
    
    Boolean         needsToRunFirstPass;
    
    CFStringRef     statFilePath;
    
    ICMCompressorSessionRef         session;
    ICMCompressionSessionOptionsRef compressionSessionOptions;
        
    ComponentInstance   delegateComponent;
    
    CFMutableArrayRef   sourceFrameQueue;
    
    TimeValue64     totalDuration;
    TimeScale       timeScale;
    int             totalFrames;
    
};

typedef struct x264CodecComponentContext    x264CodecComponentContext;

ComponentResult	x264CodecComponentDispatch(ComponentParameters          *inParameters,
                                           x264CodecComponentContext    *context);