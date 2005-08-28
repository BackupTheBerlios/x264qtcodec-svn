/*
 *  x264Codec.c
 *  x264Codec
 *
 *  Created by Henry Mason on 5/23/05.
 *  Copyright 2005 Henry Mason. All rights reserved.
 *
 */

#include "x264Codec.h"

struct x264CodecGlueHeader
{
    UInt8                flags;
    UInt8                paramSize;
    short                what;
};

#define STAT_FILE_PATH_BUFFER_SIZE  1024

static CFStringRef x264CodecFindTempFile()
{
    FSRef tempFolder;
    
    ComponentResult err = FSFindFolder(kOnAppropriateDisk, kTemporaryFolderType, TRUE, &tempFolder);    
    if (err) {
        fprintf(stderr, "x264Codec: Could not find temp folder!\n");
        return NULL;  
    } 
    
    CFURLRef tempFolderURL = CFURLCreateFromFSRef(NULL, &tempFolder);
    
    CFStringRef tempFolderPath = CFURLCopyFileSystemPath(tempFolderURL, kCFURLPOSIXPathStyle);
    
    CFShow(tempFolderPath);
    
    srandom(time(NULL));
    
    CFStringRef ret = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@/x264Codec-statfile-%ld"), tempFolderPath, random());
    
    CFShow(ret);
    
    //CFRelease(tempFolderPath);
    //CFRelease(tempFolderURL);
    
    CFRetain(ret);
    
    return ret;
}

ComponentResult x264CodecComponentOpen(x264CodecComponentContext* context, ComponentInstance self)
{
    fprintf(stderr, "x264Codec: x264CodecComponentOpen called\n");
    
    ComponentResult err = noErr;
    
    context = malloc(sizeof(x264CodecComponentContext));    
    
    SetComponentInstanceStorage(self, (Handle)context);
    
    context->self = self;
    
    context->settings = NewHandle(sizeof(x264_param_t));
    
    x264_param_default(&context->encoderParams);
    
    context->encoderParams.i_bframe = 2;
    context->encoderParams.b_bframe_adaptive = TRUE;
    context->encoderParams.analyse.b_chroma_me = TRUE;
    
    context->encoderParams.rc.psz_stat_out = malloc(STAT_FILE_PATH_BUFFER_SIZE);
    context->encoderParams.rc.psz_stat_in = malloc(STAT_FILE_PATH_BUFFER_SIZE);
    
    context->encoder = NULL;    
    context->session = NULL;
    context->compressionSessionOptions = NULL;
    context->statFilePath = NULL;    
    
    context->sourcePictureIsAllocated = FALSE;
    
    context->delegateComponent = NULL;
    
    context->sourceFrameQueue = NULL;
    
    err = OpenADefaultComponent(decompressorComponentType, kBaseCodecType, &context->delegateComponent);
    
    
    return err;
}

ComponentResult x264CodecComponentClose(x264CodecComponentContext* context)
{
    fprintf(stderr, "x264Codec: x264CodecComponentClose called\n");
    
    ComponentResult err = noErr;
    
    if (context->encoder) {
        x264_encoder_close(context->encoder);
        context->encoder = NULL;
    }
        
    if (context->compressionSessionOptions) {
        ICMCompressionSessionOptionsRelease(context->compressionSessionOptions);
        context->compressionSessionOptions = NULL;
    }
    
    if (context->statFilePath) {
        
        CFURLRef    statFileURL = CFURLCreateWithFileSystemPath(NULL,
                                                                context->statFilePath,
                                                                kCFURLPOSIXPathStyle,
                                                                FALSE);
        
        CFURLDestroyResource(statFileURL, NULL);
        
        CFRelease(statFileURL);
        
        CFRelease(context->statFilePath);
        context->statFilePath = NULL;
    }
    
    if (context->sourcePictureIsAllocated) {
        x264_picture_clean(&context->sourcePicture);
        context->sourcePictureIsAllocated = FALSE;
    }
    
    if (context->delegateComponent) {
        CloseComponent(context->delegateComponent);
        context->delegateComponent = NULL;
    }
    
    if (context->sourceFrameQueue) {
        CFRelease(context->sourceFrameQueue);
        context->sourceFrameQueue = NULL;
    }
    
    free(context->encoderParams.rc.psz_stat_in);
    context->encoderParams.rc.psz_stat_in = NULL;
    
    free(context->encoderParams.rc.psz_stat_out);
    context->encoderParams.rc.psz_stat_out = NULL;
    
    DisposeHandle(context->settings);
    
    return err;
}

ComponentResult x264CodecComponentGetCodecInfo(x264CodecComponentContext* context, CodecInfo *info)
{
    ComponentResult err = noErr;
    
    fprintf(stderr, "x264Codec: x264CodecComponentGetCodecInfo called\n");
    
    if (info == NULL) {
        err = paramErr;
    } else {
        CodecInfo **tempCodecInfo;
        
        err = GetComponentResource((Component)context->self, codecInfoResourceType, 256, (Handle *)&tempCodecInfo);
        if (err == noErr) {
            *info = **tempCodecInfo;
            
            DisposeHandle((Handle)tempCodecInfo);
        } else {
            fprintf(stderr, "x264Codec: Could not get codec info\n");    
        }
    }
    
    return err;
}

struct x264CodecQTGetComponentPropertyGlue
{
    struct x264CodecGlueHeader  header;
    
    ByteCount *                 outPropValueSizeUsed;
    ComponentValuePtr           outPropValueAddress;
    ByteCount                   inPropValueSize;
    ComponentPropertyID         inPropID;    
    ComponentPropertyClass      inPropClass;    
};

ComponentResult x264CodecQTGetComponentProperty(x264CodecComponentContext   *context,                                                
                                                ComponentPropertyClass      inPropClass,
                                                ComponentPropertyID         inPropID,
                                                ByteCount                   inPropValueSize,
                                                ComponentValuePtr           outPropValueAddress,
                                                ByteCount *                 outPropValueSizeUsed)
{
    ComponentResult err = noErr;
    
    fprintf(stderr, "x264Codec: x264CodecQTGetComponentProperty  (\'%.4s\'/\'%.4s\') called\n", (char *)&inPropClass, (char *)&inPropID);  
    
    err =  kQTPropertyNotSupportedErr;
            
    if (outPropValueSizeUsed)   *outPropValueSizeUsed = 0;
    
    return err;
}

struct x264CodecGetMaxCompressionSizeGlue
{   
    struct x264CodecGlueHeader  header;
    
    long                 *size;
    CodecQ               quality;    
    short                depth;    
    const Rect           *srcRect;
    PixMapHandle         src;
};

struct x264CodecGetMaxCompressionSizeWithSourcesGlue
{   
    struct x264CodecGlueHeader  header;
    
    long                    *size;
    CDSequenceDataSourcePtr sourceData;
    CodecQ                  quality;    
    short                   depth;    
    const Rect              *srcRect;
    PixMapHandle            src;
};

ComponentResult x264CodecGetMaxCompressionSizeWithSources(x264CodecComponentContext *context,
                                                          PixMapHandle             src,
                                                          const Rect               *srcRect,
                                                          short                    depth,
                                                          CodecQ                   quality,
                                                          CDSequenceDataSourcePtr  sourceData,
                                                          long                     *size )
{
    ComponentResult err = noErr;    
    
    fprintf(stderr, "x264Codec: x264CodecGetMaxCompressionWithSourcesSize called\n");
    
    *size =  1024 * 1024;
    
    return err;
}

struct x264CodecGetCompressionTimeGlue
{   
    struct x264CodecGlueHeader  header;
    
    unsigned long *     time;
    CodecQ *            temporalQuality;
    CodecQ *            spatialQuality;
    short               depth;
    const Rect *        srcRect;    
    PixMapHandle        src;
};

ComponentResult x264CodecGetCompressionTime (x264CodecComponentContext* context,
                                             PixMapHandle        src,
                                             const Rect *        srcRect,
                                             short               depth,
                                             CodecQ *            spatialQuality,
                                             CodecQ *            temporalQuality,
                                             unsigned long *     time)
{
    ComponentResult err = noErr;    
    
    fprintf(stderr, "x264Codec: x264CodecGetCompressionTime called\n");
    
    *time =  200;
    
    return err;
}

ComponentResult x264CodecSetSettings (x264CodecComponentContext* context,
                                      ComponentInstance    self,
                                      Handle               settings )
{
    ComponentResult err = noErr;   
    
    fprintf(stderr, "x264Codec: x264CodecSetSettings called\n"); 
    
    SetHandleSize(context->settings, GetHandleSize(settings));
    
    memcpy(*settings, *context->settings, GetHandleSize(settings));
    
    return err;
}

struct x264CodecStandardParameterDialogDoActionGlue
{   
    struct x264CodecGlueHeader  header;
    
    QTParameterDialog    createdDialog;
    long                 action;
    void                 *param;
};

ComponentResult x264CodecStandardParameterDialogDoAction(x264CodecComponentContext* context,
                                                         QTParameterDialog    createdDialog,
                                                         long                 action,
                                                         void                 *params)
{
    ComponentResult err = noErr;    
    
    fprintf(stderr, "x264Codec: just got a request for an action of %ld\n", action);
    
    return err;
}

struct x264CodecExtractAndCombineFieldsGlue
{   
    struct x264CodecGlueHeader  header;
    
    ImageDescriptionHandle     descOut;
    long *                     outDataSize;
    void *                     outputData;
    ImageDescriptionHandle     desc2;
    long                       dataSize2;
    void *                     data2;
    ImageDescriptionHandle     desc1;
    long                       dataSize1;
    void *                     data1;    
    long                       fieldFlags;
};

ComponentResult x264CodecExtractAndCombineFields(x264CodecComponentContext  *context,
                                                 long                       fieldFlags,
                                                 void *                     data1,
                                                 long                       dataSize1,
                                                 ImageDescriptionHandle     desc1,
                                                 void *                     data2,
                                                 long                       dataSize2,
                                                 ImageDescriptionHandle     desc2,
                                                 void *                     outputData,
                                                 long *                     outDataSize,
                                                 ImageDescriptionHandle     descOut)
{
    ComponentResult err = noErr;
    
    err = ImageCodecExtractAndCombineFields(context->delegateComponent,
                                            fieldFlags,
                                            data1,
                                            dataSize1,
                                            desc1,
                                            data2,
                                            dataSize2,
                                            desc2,
                                            outputData,
                                            outDataSize,
                                            descOut);
    
    return err;
}

struct x264CodecPrepareToCompressFramesGlue
{   
    struct x264CodecGlueHeader  header;
    
    CFDictionaryRef *                 compressorPixelBufferAttributesOut;
    void *                            reserved;
    ImageDescriptionHandle            imageDescription;
    ICMCompressionSessionOptionsRef   compressionSessionOptions;
    ICMCompressorSessionRef           session;
};

ComponentResult x264CodecPrepareToCompressFrames(x264CodecComponentContext         *context,
                                                 ICMCompressorSessionRef           session,
                                                 ICMCompressionSessionOptionsRef   compressionSessionOptions,
                                                 ImageDescriptionHandle            imageDescription,
                                                 void *                            reserved,
                                                 CFDictionaryRef *                 compressorPixelBufferAttributesOut)
{
    ComponentResult err = noErr;    
    
    fprintf(stderr, "x264Codec: x264CodecPrepareToCompressFrames called\n"); 
        
    if (context->session == session) {
        fprintf(stderr, "x264Codec: has already been prepared!\n");  
        return err;
    } else {
        fprintf(stderr, "x264Codec: has NOT already been prepared!\n"); 
    }
    
    if (session) {        
        context->session = session;        
    } else {
        err = paramErr;
    }
    
    if (compressionSessionOptions) {
        ICMCompressionSessionOptionsRetain(compressionSessionOptions);
        context->compressionSessionOptions = compressionSessionOptions;        
    }
    
    if (compressorPixelBufferAttributesOut) {
        *compressorPixelBufferAttributesOut = CVPixelFormatDescriptionCreateWithPixelFormatType(NULL, '2vuy');  
    } else {
        err = paramErr;
    }
    
    context->sourceFrameQueue = CFArrayCreateMutable(NULL, 100, NULL);
    
    SInt32   dataRate = 0;
    
    err = ICMCompressionSessionOptionsGetProperty(compressionSessionOptions, 
                                                  kQTPropertyClass_ICMCompressionSessionOptions,
                                                  kICMCompressionSessionOptionsPropertyID_AverageDataRate,
                                                  sizeof(dataRate),
                                                  &dataRate,
                                                  NULL);
    if (err)    return err;
        
    fprintf(stderr, "x264Codec: kICMCompressionSessionOptionsPropertyID_AverageDataRate is %d\n", (int)dataRate);
    
    // data rate seems to come in bytes/sec    
    
    if (dataRate > 0) {
        context->encoderParams.rc.i_bitrate = (dataRate * 8) / 1000; // in kbps  
        
        context->encoderParams.rc.f_qcompress = 0.9;
        
        context->encoderParams.rc.b_cbr = 1;      
    } else {
        
        context->encoderParams.rc.b_cbr = 0; 
        
    }
    
    context->needsToRunFirstPass = TRUE;
    
    context->encoderParams.i_width = (*imageDescription)->width;
    context->encoderParams.i_height = (*imageDescription)->height;
    
    if (context->sourcePictureIsAllocated) {
        x264_picture_clean(&context->sourcePicture);
        context->sourcePictureIsAllocated = FALSE;
    }
    
    x264_picture_alloc(&context->sourcePicture, X264_CSP_I420, (*imageDescription)->width, (*imageDescription)->height);
    context->sourcePictureIsAllocated = TRUE;
    
    x264_nal_t   *nal;
    int     nal_i;
    
    // We need to briefly open an encoder in order to get the SPS/PPS...    
    context->encoder = x264_encoder_open(&context->encoderParams);
    
    x264_encoder_headers(context->encoder, &nal, &nal_i);
    
    int     spsCount = 0, *spsIndexes = malloc(nal_i * sizeof(int));
    int     ppsCount = 0, *ppsIndexes = malloc(nal_i * sizeof(int));
    
    int i;
    
    // Scan through the NALs to find the SPSs and PPSs
    for (i = 0; i < nal_i; i++) {            
        switch (nal[i].i_type) {
            case NAL_SPS:
                spsIndexes[spsCount] = i;
                spsCount++;                    
                break;
            case NAL_PPS:
                ppsIndexes[ppsCount] = i;
                ppsCount++;   
        }
    }
    
    // Construct an 'avcC' atom
    
    // 4K buffer for storing encoded SPS/PPS should be plenty
    // ^ famous last words
    UInt8               *tempBuffer = malloc(4096);
    CFMutableDataRef    avcCData = CFDataCreateMutable(NULL, 1024);
    
    UInt8   aByte;
    UInt16  aShort;
    
    // -> 1 byte version = 8-bit hex version  (current = 1)
    aByte = (UInt8)0x1;
    CFDataAppendBytes(avcCData, &aByte, 1);
    
    if (spsCount) {
        // AVCProfileIndication = 8-bit
        aByte = (UInt8)nal[spsIndexes[0]].p_payload[0];
        CFDataAppendBytes(avcCData, &aByte, 1);
        
        // profile_compatibility = 8-bit
        aByte = (UInt8)nal[spsIndexes[0]].p_payload[1];
        CFDataAppendBytes(avcCData, &aByte, 1);
        
        // AVCLevelIndication = 8-bit
        aByte = (UInt8)nal[spsIndexes[0]].p_payload[2];
        CFDataAppendBytes(avcCData, &aByte, 1);
    } else {
        // If we have no SPSs, we need to guess.
        // These may be insane.
        
        // AVCProfileIndication = 8-bit
        aByte = (UInt8)0x4D;
        CFDataAppendBytes(avcCData, &aByte, 1);
        
        // profile_compatibility = 8-bit
        aByte = (UInt8)0x40;
        CFDataAppendBytes(avcCData, &aByte, 1);
        
        // AVCLevelIndication = 8-bit
        aByte = (UInt8)0x28;
        CFDataAppendBytes(avcCData, &aByte, 1);        
    }    
    
    // this appears to be a lie below
    
    // 6 bits of 111111 + 2 bits of lengthSizeMinusOne = 8-bit total
    aByte = (UInt8)0xFC + (UInt8)0x3; // we seem to always use 4 bytes
    CFDataAppendBytes(avcCData, &aByte, 1);
    
    // 3 bits of 111 + 5 bits numOfSequenceParameterSets = 8-bit total   
    aByte = (UInt8)0xE0 + (UInt8)spsCount;
    CFDataAppendBytes(avcCData, &aByte, 1);
    
    // Append the list of SPSs
    for (i = 0; i < spsCount; i++) {
        int spsIndex = spsIndexes[i];                
        int bufferSize = 4096;
        
        // Encode the SPS NAL
        bufferSize = x264_nal_encode(tempBuffer, &bufferSize, 1, &nal[spsIndex]);
        
        // Now we need to lop off the start of the SPS, which is just the size
        // in 32-bit int form.
        bufferSize -= sizeof(UInt32);
        
        // sequenceParameterSetLength
        aShort = (UInt16)bufferSize; 
        CFDataAppendBytes(avcCData, (UInt8 *)&aShort, 2);
        
        // sequenceParameterSetNALUnit
        // (advanced 4 bytes for the size, which we don't need)
        CFDataAppendBytes(avcCData, tempBuffer + sizeof(UInt32), bufferSize);            
    }        
    
    // -> numOfPictureParameterSets = 8-bits
    aByte = (UInt8)ppsCount;
    CFDataAppendBytes(avcCData, &aByte, 1);
    
    // Append the list of PPSs
    for (i = 0; i < ppsCount; i++) {      
        int ppsIndex = ppsIndexes[i];                
        int bufferSize = 4096;
        
        // Encode the PPS NAL
        bufferSize = x264_nal_encode(tempBuffer, &bufferSize, 1, &nal[ppsIndex]);
        
        // Now we need to lop off the start of the PPS, which is just the size
        // in 32-bit int form.
        bufferSize -= sizeof(UInt32);
        
        // pictureParameterSetLength = 16-bit
        aShort = (UInt16)bufferSize; 
        CFDataAppendBytes(avcCData, (UInt8 *)&aShort, 2);
        
        // pictureParameterSetNALUnit
        // (advanced 4 bytes for the size, which we don't need)
        CFDataAppendBytes(avcCData, tempBuffer + sizeof(UInt32), bufferSize);              
    }        
    
    Handle  avcCHandle = NULL;
    
    PtrToHand(CFDataGetBytePtr(avcCData), &avcCHandle, CFDataGetLength(avcCData));
    
    // Add it to the image description
    err = AddImageDescriptionExtension(imageDescription, avcCHandle, 'avcC');
    
    DisposeHandle(avcCHandle);    
    CFRelease(avcCData);
    free(tempBuffer);
    
    
    // Need to close the encoder so it can be re-opened with the correct pass properties.
    x264_encoder_close(context->encoder);
    context->encoder = NULL;
    
    
    return err;
}

struct x264CodecProcessBetweenPassesGlue
{
    struct x264CodecGlueHeader  header;
    
    ICMCompressionPassModeFlags *  requestedNextPassModeFlagsOut;
    Boolean *                      interpassProcessingDoneOut;  
    ICMMultiPassStorageRef         multiPassStorage;
};

ComponentResult x264CodecProcessBetweenPasses(x264CodecComponentContext     *context,
                                              ICMMultiPassStorageRef        multiPassStorage,
                                              Boolean                       *interpassProcessingDoneOut,
                                              ICMCompressionPassModeFlags   *requestedNextPassModeFlagsOut)
{
    ComponentResult err = noErr;    
    
    fprintf(stderr, "x264Codec: x264CodecProcessBetweenPasses called\n"); 
    
    if (context->needsToRunFirstPass == TRUE) {
        *requestedNextPassModeFlagsOut = kICMCompressionPassMode_WriteToMultiPassStorage | kICMCompressionPassMode_NotReadyToOutputEncodedFrames;
    } else {        
        *requestedNextPassModeFlagsOut = kICMCompressionPassMode_ReadFromMultiPassStorage | kICMCompressionPassMode_OutputEncodedFrames;
    }
    
    *interpassProcessingDoneOut = TRUE;
    
    return err;    
}

struct x264CodecBeginPassGlue
{   
    struct x264CodecGlueHeader  header;
    
    ICMMultiPassStorageRef           multiPassStorage;
    UInt32                           flags;
    ICMCompressionPassModeFlags      passModeFlags;
};

ComponentResult x264CodecBeginPass(x264CodecComponentContext        *context,
                                   ICMCompressionPassModeFlags      passModeFlags,
                                   UInt32                           flags,
                                   ICMMultiPassStorageRef           multiPassStorage)
{
    ComponentResult err = noErr;    
    
    fprintf(stderr, "x264Codec: x264CodecBeginPass called\n"); 
    
    if (!context->statFilePath) context->statFilePath = x264CodecFindTempFile();
    
    if (passModeFlags & kICMCompressionPassMode_OutputEncodedFrames) {
        context->encoderParams.rc.b_stat_write = FALSE; 
        context->encoderParams.rc.b_stat_read = TRUE;        
        
        CFStringGetCString(context->statFilePath,
                           context->encoderParams.rc.psz_stat_in,
                           STAT_FILE_PATH_BUFFER_SIZE,
                           kCFStringEncodingUTF8);
    } else {   
        context->encoderParams.rc.b_stat_read = FALSE;
        context->encoderParams.rc.b_stat_write = TRUE;
        
        CFStringGetCString(context->statFilePath,
                           context->encoderParams.rc.psz_stat_out,
                           STAT_FILE_PATH_BUFFER_SIZE,
                           kCFStringEncodingUTF8);              
    }
    
    context->encoder = x264_encoder_open(&context->encoderParams);
    
    fprintf(stderr, "passModeFlags & kICMCompressionPassMode_WriteToMultiPassStorage: %ld\n", passModeFlags & kICMCompressionPassMode_WriteToMultiPassStorage);
    fprintf(stderr, "passModeFlags & kICMCompressionPassMode_ReadFromMultiPassStorage: %ld\n", passModeFlags & kICMCompressionPassMode_ReadFromMultiPassStorage);
    fprintf(stderr, "passModeFlags & kICMCompressionPassMode_OutputEncodedFrames: %ld\n", passModeFlags & kICMCompressionPassMode_OutputEncodedFrames);
    fprintf(stderr, "passModeFlags & kICMCompressionPassMode_NotReadyToOutputEncodedFrames: %ld\n", passModeFlags & kICMCompressionPassMode_NotReadyToOutputEncodedFrames);
    fprintf(stderr, "passModeFlags & kICMCompressionPassMode_NoSourceFrames: %ld\n", passModeFlags & kICMCompressionPassMode_NoSourceFrames);
    
    context->totalDuration = 0;
    context->totalFrames = 0;
    
    return err;
}

ComponentResult x264CodecEmitFrameFromNAL(x264CodecComponentContext *context,
                                          x264_nal_t                *networkAbstractionLayer, 
                                          int                       nalCount,
                                          x264_picture_t            *pic_out)
{
    ComponentResult err = noErr;    
    
    ICMMutableEncodedFrameRef   frame;
    ICMFrameType                frameType;
    MediaSampleFlags            mediaSampleFlags;
    int i;
    
    
    mediaSampleFlags = 0;
    
    switch (pic_out->i_type) {
        case X264_TYPE_B:       
            mediaSampleFlags |= mediaSampleIsNotDependedOnByOthers;
        case X264_TYPE_P:
        case X264_TYPE_BREF:   
        default:
            mediaSampleFlags |= mediaSampleNotSync;
            break;
        case X264_TYPE_I:
        case X264_TYPE_IDR:
            mediaSampleFlags |= mediaSampleIsDependedOnByOthers;
    }
    
    switch (pic_out->i_type) {
        case X264_TYPE_I:
        case X264_TYPE_IDR:
            frameType = kICMFrameType_I;
            break;
        case X264_TYPE_P:
            frameType = kICMFrameType_P;
            break;
        case X264_TYPE_B:   
        case X264_TYPE_BREF:   
            frameType = kICMFrameType_B;
            break;
        default:
            frameType = kICMFrameType_Unknown;
    }
    
    int bufferSpaceLeft = 1024 * 1024;
    
    ICMCompressorSourceFrameRef firstUnencodedSourceFrame;
    
    firstUnencodedSourceFrame = (ICMCompressorSourceFrameRef)CFArrayGetValueAtIndex(context->sourceFrameQueue, 0);
    
    err = ICMEncodedFrameCreateMutable(context->session,
                                       firstUnencodedSourceFrame,
                                       bufferSpaceLeft, // this needs to be a max H.264 frame size
                                       &frame);
    if (err) {
        ICMEncodedFrameRelease(frame);
        return err;       
    }  
    
    TimeValue64 displayDuration, sourceDisplayTimeStamp;
    
    err = ICMCompressorSourceFrameGetDisplayTimeStampAndDuration(firstUnencodedSourceFrame,
                                                                 &sourceDisplayTimeStamp,
                                                                 &displayDuration,
                                                                 NULL, // <#TimeScale * timeScaleOut#>,
                                                                 NULL); //<#ICMValidTimeFlags * validTimeFlagsOut#>);
    if (err) {
        ICMEncodedFrameRelease(frame);
        return err;           
    }
    
    //fprintf(stderr, "x264Codec: displayDuration is %lld, pic_out->i_pts is %lld\n", displayDuration, pic_out->i_pts);
    
    err = ICMEncodedFrameSetDisplayDuration(frame, displayDuration);
    
    err = ICMEncodedFrameSetDecodeTimeStamp(frame, sourceDisplayTimeStamp);
    
    err = ICMEncodedFrameSetDisplayTimeStamp(frame, pic_out->i_pts);
    if (err) {
        ICMEncodedFrameRelease(frame);
        return err;       
    }    
    
    err = ICMEncodedFrameSetMediaSampleFlags(frame, mediaSampleFlags);
    if (err) {
        ICMEncodedFrameRelease(frame);
        return err;       
    }  
    
    err = ICMEncodedFrameSetFrameType(frame, frameType);
    if (err) {
        ICMEncodedFrameRelease(frame);
        return err;       
    }  
    
    err = ICMEncodedFrameSetValidTimeFlags(frame, kICMValidTime_DisplayTimeStampIsValid);
    if (err) {
        ICMEncodedFrameRelease(frame);
        return err;       
    }  
    
    unsigned char *outPtr = ICMEncodedFrameGetDataPtr(frame);
    
    int totalSize = 0;
    
    for (i = 0; i < nalCount; i++) {
        
        int thisSize = x264_nal_encode(outPtr, &bufferSpaceLeft, 1, &networkAbstractionLayer[i]);
        
        // need to set the size of this frame for some reason(?)
        if (thisSize > 0) {
            *(UInt32 *)outPtr = htonl(thisSize - sizeof(UInt32));
            outPtr += thisSize;
            totalSize += thisSize;
        } else {
            fprintf(stderr, "x264Codec: Trouble encoding NAL\n");
        }
    }
    
    err = ICMEncodedFrameSetDataSize(frame, totalSize);
    if (err) {
        ICMEncodedFrameRelease(frame);
        return err;       
    }             
    
    err = ICMCompressorSessionEmitEncodedFrame(context->session,
                                               frame,
                                               1,
                                               &firstUnencodedSourceFrame);                        
    ICMEncodedFrameRelease(frame);
    
    CFArrayRemoveValueAtIndex(context->sourceFrameQueue, 0);
    
    ICMCompressorSourceFrameRelease(firstUnencodedSourceFrame);
    
    return err;
}

struct x264CodecEncodeFrameGlue 
{
    struct x264CodecGlueHeader  header;
    
    UInt32                        flags;
    ICMCompressorSourceFrameRef   sourceFrame;
};

struct x264CodecEncodeFrame2vuyBlock
{
    unsigned char   cb;
    unsigned char   y0;
    unsigned char   cr;
    unsigned char   y1;
};

typedef struct x264CodecEncodeFrame2vuyBlock x264CodecEncodeFrame2vuyBlock;

struct x264CodecEncodeFrameyuvsBlock
{
    unsigned char   y0;
    unsigned char   cb;
    unsigned char   y1;
    unsigned char   cr;
};

typedef struct x264CodecEncodeFrameyuvsBlock x264CodecEncodeFrameyuvsBlock;

ComponentResult x264CodecEncodeFrame(x264CodecComponentContext  *context,
                                     ICMCompressorSourceFrameRef sourceFrame,
                                     UInt32                      flags)
{
    
    ComponentResult err = noErr;    
        
    if (!sourceFrame)  return err;
    
    //fprintf(stderr, "x264Codec: x264CodecEncodeFrame called\n");
    
    CVPixelBufferRef    pixelBuffer = ICMCompressorSourceFrameGetPixelBuffer(sourceFrame);
        
    if (!context->needsToRunFirstPass) {
        ICMCompressorSourceFrameRetain(sourceFrame);
        
        CFArrayAppendValue(context->sourceFrameQueue, sourceFrame);
    }
    
    TimeValue64 displayDur;
    
    err = ICMCompressorSourceFrameGetDisplayTimeStampAndDuration(sourceFrame, NULL, &displayDur, &context->timeScale, NULL);
    
    context->totalDuration += displayDur;
    
    context->totalFrames += 1;
        
    if (pixelBuffer) {
                
        OSType pixelFormatType = CVPixelBufferGetPixelFormatType(pixelBuffer);
        
        
        if (pixelFormatType == k422YpCbCr8CodecType) {
            
            unsigned char   *yCopyPtr = context->sourcePicture.img.plane[0];
            unsigned char   *cbCopyPtr = context->sourcePicture.img.plane[1];
            unsigned char   *crCopyPtr = context->sourcePicture.img.plane[2];
            
            CVPixelBufferLockBaseAddress(pixelBuffer, 0);
            
            x264CodecEncodeFrame2vuyBlock  *inBlocks = CVPixelBufferGetBaseAddress(pixelBuffer);
            
            int x, y;
            int height = CVPixelBufferGetHeight(pixelBuffer);
            int usableBlocksPerRow = (CVPixelBufferGetWidth(pixelBuffer) * 2) / sizeof(x264CodecEncodeFrame2vuyBlock);
            int blocksPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer) / sizeof(x264CodecEncodeFrame2vuyBlock);
            
            for (y = 0; y < height; y+=2) {
                
                for (x = 0; x < usableBlocksPerRow; x++) {
                    cbCopyPtr[x] = inBlocks[x].cb;
                    yCopyPtr[x*2]  = inBlocks[x].y0;
                    crCopyPtr[x] = inBlocks[x].cr;
                    yCopyPtr[x*2+1] = inBlocks[x].y1;
                }
                
                inBlocks += blocksPerRow;
                yCopyPtr += context->sourcePicture.img.i_stride[0];       
                
                for (x = 0; x < usableBlocksPerRow; x++) {
                    yCopyPtr[x*2]  = inBlocks[x].y0;
                    yCopyPtr[x*2+1] = inBlocks[x].y1;
                }
                
                inBlocks += blocksPerRow;
                yCopyPtr += context->sourcePicture.img.i_stride[0]; 
                cbCopyPtr += context->sourcePicture.img.i_stride[1];
                crCopyPtr += context->sourcePicture.img.i_stride[2];   
            }
            
            CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);      
        } else if (kComponentVideoUnsigned) {
            unsigned char   *yCopyPtr = context->sourcePicture.img.plane[0];
            unsigned char   *cbCopyPtr = context->sourcePicture.img.plane[1];
            unsigned char   *crCopyPtr = context->sourcePicture.img.plane[2];
            
            CVPixelBufferLockBaseAddress(pixelBuffer, 0);
            
            x264CodecEncodeFrameyuvsBlock  *inBlocks = CVPixelBufferGetBaseAddress(pixelBuffer);
            
            int x, y;
            int height = CVPixelBufferGetHeight(pixelBuffer);
            int usableBlocksPerRow = (CVPixelBufferGetWidth(pixelBuffer) * 2) / sizeof(x264CodecEncodeFrame2vuyBlock);
            int blocksPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer) / sizeof(x264CodecEncodeFrame2vuyBlock);
            
            for (y = 0; y < height; y+=2) {
                
                for (x = 0; x < usableBlocksPerRow; x++) {
                    cbCopyPtr[x] = inBlocks[x].cb;
                    yCopyPtr[x*2]  = inBlocks[x].y0;
                    crCopyPtr[x] = inBlocks[x].cr;
                    yCopyPtr[x*2+1] = inBlocks[x].y1;
                }
                
                inBlocks += blocksPerRow;
                yCopyPtr += context->sourcePicture.img.i_stride[0];       
                
                for (x = 0; x < usableBlocksPerRow; x++) {
                    yCopyPtr[x*2]  = inBlocks[x].y0;
                    yCopyPtr[x*2+1] = inBlocks[x].y1;
                }
                
                inBlocks += blocksPerRow;
                yCopyPtr += context->sourcePicture.img.i_stride[0]; 
                cbCopyPtr += context->sourcePicture.img.i_stride[1];
                crCopyPtr += context->sourcePicture.img.i_stride[2];   
            }
            
            CVPixelBufferUnlockBaseAddress(pixelBuffer, 0); 
        } else if (pixelFormatType == 'I420') {
            CVPixelBufferLockBaseAddress(pixelBuffer, 0);
            
            int lumaRowBytes = CVPixelBufferGetBytesPerRow(pixelBuffer);
            int chromaRowBytes = lumaRowBytes / 2;
            
            // May need to reallocate the bitmap buffer to match the source
            
            if (context->sourcePicture.img.i_stride[0] > lumaRowBytes) {                
                context->sourcePicture.img.i_stride[0] = lumaRowBytes;                
            } else if (context->sourcePicture.img.i_stride[0] < lumaRowBytes) {                
                free(context->sourcePicture.img.plane[0]);                
                context->sourcePicture.img.plane[0] = malloc(lumaRowBytes * CVPixelBufferGetHeight(pixelBuffer));                
            }
            
            if (context->sourcePicture.img.i_stride[1] > chromaRowBytes) {                
                context->sourcePicture.img.i_stride[1] = chromaRowBytes;
            } else if (context->sourcePicture.img.i_stride[1] < chromaRowBytes) {                
                free(context->sourcePicture.img.plane[1]);                
                context->sourcePicture.img.plane[1] = malloc(lumaRowBytes * CVPixelBufferGetHeight(pixelBuffer) / 2);                
            }
            
            if (context->sourcePicture.img.i_stride[2] > chromaRowBytes) {                
                context->sourcePicture.img.i_stride[2] = chromaRowBytes;
            } else if (context->sourcePicture.img.i_stride[2] < chromaRowBytes) {                
                free(context->sourcePicture.img.plane[2]);                
                context->sourcePicture.img.plane[2] = malloc(lumaRowBytes * CVPixelBufferGetHeight(pixelBuffer) / 2);                
            }
            
            int sizeOfPlane = lumaRowBytes * CVPixelBufferGetHeight(pixelBuffer);
            
            unsigned char *inPixels = CVPixelBufferGetBaseAddress(pixelBuffer);
            
            unsigned char   *yCopyPtr = context->sourcePicture.img.plane[0];
            unsigned char   *cbCopyPtr = context->sourcePicture.img.plane[1];
            unsigned char   *crCopyPtr = context->sourcePicture.img.plane[2];
            
            memcpy(yCopyPtr, inPixels, sizeOfPlane);
            
            inPixels += sizeOfPlane;
            
            sizeOfPlane >>= 2;
            
            memcpy(cbCopyPtr, inPixels, sizeOfPlane);
            
            inPixels += sizeOfPlane;
            
            memcpy(crCopyPtr, inPixels, sizeOfPlane);
            
            CVPixelBufferUnlockBaseAddress(pixelBuffer, 0); 
        } else {
            fprintf(stderr, "x264Codec: Got unsupported pixel format: 0x%x/\'%.4s\'\n", (int)pixelFormatType, (char *)&pixelFormatType);
            
        }
        
        x264_nal_t      *networkAbstractionLayer;
        x264_picture_t  pic_out;
        int             nalCount;
                
        context->sourcePicture.i_type = X264_TYPE_AUTO;
        
        err = ICMCompressorSourceFrameGetDisplayTimeStampAndDuration(sourceFrame,
                                                                     &context->sourcePicture.i_pts,
                                                                     NULL, //<#TimeValue64 * displayDurationOut#>,
                                                                     NULL, //<#TimeScale * timeScaleOut#>,
                                                                     NULL); //<#ICMValidTimeFlags * validTimeFlagsOut#>);
        if (err) return err;
        
        int x264err = x264_encoder_encode(context->encoder,
                                          &networkAbstractionLayer,
                                          &nalCount,
                                          &context->sourcePicture,
                                          &pic_out);
        if (x264err < 0) {
            fprintf(stderr, "x264Codec: x264_encoder_encode failed (%d)", x264err);
            err = x264err;
        }
        
        if (context->needsToRunFirstPass) {
            // first pass stuff?
        } else if (nalCount) {
            err = x264CodecEmitFrameFromNAL(context, networkAbstractionLayer, nalCount, &pic_out);
        }        
    } else {        
        fprintf(stderr, "x264Codec: No pixel buffer given!\n");
    }
    
    return err;
}

struct x264CodecCompleteFrameGlue {
    struct x264CodecGlueHeader  header;
    
    UInt32                      flags;
    ICMCompressorSourceFrameRef sourceFrame;
};

ComponentResult x264CodecCompleteFrame(x264CodecComponentContext    *context,
                                       ICMCompressorSourceFrameRef  sourceFrame,
                                       UInt32                       flags) 
{
    ComponentResult err = noErr;
    
    if (context->needsToRunFirstPass)   return err;
    
    // Need to empty out delayed b-frames
    while (CFArrayGetCount(context->sourceFrameQueue)) {
        
        x264_nal_t      *networkAbstractionLayer;
        x264_picture_t  pic_out;
        int             nalCount;
        
        context->sourcePicture.i_type = X264_TYPE_AUTO;
        
        int x264err = x264_encoder_encode(context->encoder,
                                          &networkAbstractionLayer,
                                          &nalCount,
                                          NULL,
                                          &pic_out);
        if (x264err < 0) {
            fprintf(stderr, "x264Codec: x264_encoder_encode failed (%d)", x264err);
            err = x264err;
        }
        
        if (context->needsToRunFirstPass) {
            // first pass stuff?
        } else if (nalCount) {
            err = x264CodecEmitFrameFromNAL(context, networkAbstractionLayer, nalCount, &pic_out);
        }
    } 
    
    return err;
}

ComponentResult x264CodecEndPass(x264CodecComponentContext *context)
{
    ComponentResult err = noErr;    
    
    fprintf(stderr, "x264Codec: x264CodecEndPass called\n"); 
    
    // Need to empty out delayed b-frames
    while (CFArrayGetCount(context->sourceFrameQueue)) {
        
        x264_nal_t      *networkAbstractionLayer;
        x264_picture_t  pic_out;
        int             nalCount;
        
        context->sourcePicture.i_type = X264_TYPE_AUTO;
        
        int x264err = x264_encoder_encode(context->encoder,
                                          &networkAbstractionLayer,
                                          &nalCount,
                                          NULL,
                                          &pic_out);
        if (x264err < 0) {
            fprintf(stderr, "x264Codec: x264_encoder_encode failed (%d)", x264err);
            err = x264err;
        }
        
        if (context->needsToRunFirstPass) {
            // first pass stuff?
        } else if (nalCount) {
            err = x264CodecEmitFrameFromNAL(context, networkAbstractionLayer, nalCount, &pic_out);
        }
    }   
    
    context->encoderParams.i_fps_num = context->totalFrames * 1000;
    context->encoderParams.i_fps_den = ((context->totalDuration * (long long)1000) / context->timeScale);
    
    fprintf(stderr, "x264Codec: framerate is %d/%d fps\n", context->encoderParams.i_fps_num, context->encoderParams.i_fps_den); 
    
    
    // Well, we're done with at least one pass
    context->needsToRunFirstPass = FALSE;
    
    if (context->encoder) {
        x264_encoder_close(context->encoder);
        context->encoder = NULL;
    }
    
    return err;
}

ComponentResult	x264CodecComponentDispatch(ComponentParameters* inParameters, x264CodecComponentContext* context)
{	
    ComponentResult	theError = 0;
    
    ComponentInstance self = (ComponentInstance)inParameters->params[0];
    
    switch (inParameters->what) {
        
        case kComponentOpenSelect:
            theError = x264CodecComponentOpen(context, self);
            break;
        case kComponentCloseSelect:
            theError = x264CodecComponentClose(context);
            break;
            
        case kComponentCanDoSelect:
            
            switch (*((SInt16*)&inParameters->params[1])) {
                case kComponentOpenSelect:
                case kComponentCloseSelect:
                case kComponentCanDoSelect:
                case kComponentRegisterSelect:
                case kComponentVersionSelect:
                case kImageCodecGetCodecInfoSelect:
                case kImageCodecGetMaxCompressionSizeSelect:
                case kImageCodecSetSettingsSelect:
                case kImageCodecStandardParameterDialogDoActionSelect:
                case kImageCodecBeginPassSelect:
                case kImageCodecEndPassSelect:
                case kImageCodecProcessBetweenPassesSelect:
                case kImageCodecEncodeFrameSelect:
                case kImageCodecCompleteFrameSelect:
                case kImageCodecGetCompressionTimeSelect:
                case kImageCodecGetMaxCompressionSizeWithSourcesSelect:
                case kImageCodecExtractAndCombineFieldsSelect:
                    theError = 1;
                    break;
                default:                      
                    fprintf(stderr, "x264Codec: just now got a request for a check on 0x%04x\n", (int)(*((SInt16*)&inParameters->params[1])));
                    theError = DelegateComponentCall(inParameters, context->delegateComponent);
            }
            break;
        default:
            // The rest of these need to use the context pointer
            
            if (context != NULL) {
                switch (inParameters->what) {
                    
                    case kComponentRegisterSelect:
                        // don't really need to do anything here
                        break;
                        
                    case kComponentVersionSelect:
                        theError = X264_BUILD;
                        break;
                    case kImageCodecGetCodecInfoSelect:
                        theError = x264CodecComponentGetCodecInfo(context, (CodecInfo *)inParameters->params[0]);
                        break;
                    case kQTGetComponentPropertySelect:
                    {
                        struct x264CodecQTGetComponentPropertyGlue   *glue;
                        
                        glue = (struct x264CodecQTGetComponentPropertyGlue *)inParameters;
                        
                        theError = x264CodecQTGetComponentProperty(context,
                                                                   glue->inPropClass,
                                                                   glue->inPropID,
                                                                   glue->inPropValueSize,
                                                                   glue->outPropValueAddress,
                                                                   glue->outPropValueSizeUsed);                        
                    }
                        break;
                    case kImageCodecGetMaxCompressionSizeSelect:
                    {
                        struct x264CodecGetMaxCompressionSizeGlue   *glue;
                        
                        glue = (struct x264CodecGetMaxCompressionSizeGlue *)inParameters;
                        
                        theError = x264CodecGetMaxCompressionSizeWithSources(context,
                                                                             glue->src,
                                                                             glue->srcRect,
                                                                             glue->depth,
                                                                             glue->quality,
                                                                             NULL,
                                                                             glue->size);                        
                    }
                        
                        break;
                        
                    case kImageCodecGetMaxCompressionSizeWithSourcesSelect:
                    {
                        struct x264CodecGetMaxCompressionSizeWithSourcesGlue   *glue;
                        
                        glue = (struct x264CodecGetMaxCompressionSizeWithSourcesGlue *)inParameters;
                        
                        theError = x264CodecGetMaxCompressionSizeWithSources(context,
                                                                             glue->src,
                                                                             glue->srcRect,
                                                                             glue->depth,
                                                                             glue->quality,
                                                                             glue->sourceData,
                                                                             glue->size);                        
                    }
                        break;
                    case kImageCodecGetCompressionTimeSelect:
                    {
                        struct x264CodecGetCompressionTimeGlue   *glue;
                        
                        glue = (struct x264CodecGetCompressionTimeGlue *)inParameters;
                        
                        theError = x264CodecGetCompressionTime(context,
                                                               glue->src,
                                                               glue->srcRect,
                                                               glue->depth,
                                                               glue->spatialQuality,
                                                               glue->temporalQuality,
                                                               glue->time);
                    }
                        break;
                    case kImageCodecSetSettingsSelect:
                        theError = x264CodecSetSettings(context, self, (Handle)inParameters->params[1]);
                        break;
                        
                    case kImageCodecStandardParameterDialogDoActionSelect:
                    {
                        struct x264CodecStandardParameterDialogDoActionGlue   *glue;
                        
                        glue = (struct x264CodecStandardParameterDialogDoActionGlue *)inParameters;
                        
                        theError = x264CodecStandardParameterDialogDoAction(context,
                                                                            glue->createdDialog,
                                                                            glue->action,
                                                                            glue->param);                        
                    }
                        break;
                    case kImageCodecPrepareToCompressFramesSelect:
                    {
                        struct x264CodecPrepareToCompressFramesGlue   *glue;
                        
                        glue = (struct x264CodecPrepareToCompressFramesGlue *)inParameters;
                        
                        theError = x264CodecPrepareToCompressFrames(context,
                                                                    glue->session,
                                                                    glue->compressionSessionOptions,
                                                                    glue->imageDescription,
                                                                    glue->reserved,
                                                                    glue->compressorPixelBufferAttributesOut);                        
                    }
                        break;
                    case kImageCodecBeginPassSelect:
                    {
                        struct x264CodecBeginPassGlue   *glue;
                        
                        glue = (struct x264CodecBeginPassGlue *)inParameters;
                        
                        theError = x264CodecBeginPass(context,
                                                      glue->passModeFlags,
                                                      glue->flags,
                                                      glue->multiPassStorage);                        
                    }
                        break;
                    case kImageCodecEndPassSelect:
                    {                        
                        theError = x264CodecEndPass(context);                        
                    }
                        break;
                        
                    case kImageCodecProcessBetweenPassesSelect:
                    {
                        struct x264CodecProcessBetweenPassesGlue   *glue;
                        
                        glue = (struct x264CodecProcessBetweenPassesGlue *)inParameters;
                        
                        theError = x264CodecProcessBetweenPasses(context,
                                                                 glue->multiPassStorage,
                                                                 glue->interpassProcessingDoneOut,
                                                                 glue->requestedNextPassModeFlagsOut);                        
                    }
                        break;
                    case kImageCodecEncodeFrameSelect:
                    {
                        struct x264CodecEncodeFrameGlue   *glue;
                        
                        glue = (struct x264CodecEncodeFrameGlue *)inParameters;
                        
                        theError = x264CodecEncodeFrame(context,
                                                        glue->sourceFrame,
                                                        glue->flags);                        
                    }
                        break;    
                    case kImageCodecCompleteFrameSelect:
                    {
                        struct x264CodecCompleteFrameGlue   *glue;
                        
                        glue = (struct x264CodecCompleteFrameGlue *)inParameters;
                        
                        theError = x264CodecCompleteFrame(context,
                                                          glue->sourceFrame,
                                                          glue->flags);                        
                    }
                        break;
                    case kImageCodecExtractAndCombineFieldsSelect:
                    {
                        struct x264CodecExtractAndCombineFieldsGlue   *glue;
                        
                        glue = (struct x264CodecExtractAndCombineFieldsGlue *)inParameters;
                        
                        theError = x264CodecExtractAndCombineFields(context, 
                                                                    glue->fieldFlags,
                                                                    glue->data1,
                                                                    glue->dataSize1,
                                                                    glue->desc1,
                                                                    glue->data2,
                                                                    glue->dataSize2,
                                                                    glue->desc2,
                                                                    glue->outputData,
                                                                    glue->outDataSize,
                                                                    glue->descOut);                        
                    }
                        
                        break;
                        /*
                    case kImageCodecPreCompressSelect:
                    case kImageCodecBandCompressSelect:
                    case kImageCodecCompleteFrameSelect:
                    case kImageCodecTrimImageSelect:
                        // We don't really do anything here, but still need to do it
                        break;*/
                    default:                             
                        fprintf(stderr, "x264Codec: just got a request for 0x%04x\n", (int)inParameters->what);
                        theError = DelegateComponentCall(inParameters, context->delegateComponent);
                        
                }
                
            } else {
                fprintf(stderr, "x264Codec: just got a request for 0x%04x with no params\n", (int)inParameters->what);                
                theError = DelegateComponentCall(inParameters, context->delegateComponent);
            }
            
            
    }
    
    return theError;
    
}