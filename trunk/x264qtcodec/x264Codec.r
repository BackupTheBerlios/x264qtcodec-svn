/*
 *  x264Codec.r
 *  x264Codec
 *
 *  Created by Henry Mason on 5/23/05.
 *  Copyright 2005 Henry Mason. All rights reserved.
 *
 *  (thanks to Apple's Electric Image sample code)
 *
 */

/*
    thng_RezTemplateVersion:
        0 - original 'thng' template    <-- default
        1 - extended 'thng' template	<-- used for multiplatform things
        2 - extended 'thng' template including resource map id
*/

#define thng_RezTemplateVersion 2

/*
    cfrg_RezTemplateVersion:
        0 - original					<-- default
        1 - extended					<-- we use the extended version
*/

#define cfrg_RezTemplateVersion 1

#include <Carbon/Carbon.r>
#include <QuickTime/QuickTime.r>
#undef __CARBON_R__
#undef __CORESERVICES_R__
#undef __CARBONCORE_R__
#undef __COMPONENTS_R__

#define	kx264CodecFormatType	kH264CodecType
#define	kx264CodecFormatName	"H.264 (x264)"

// These flags specify information about the capabilities of the component
#define kx264CompFlags  ( codecInfoDoes32 | codecInfoDoesMultiPass | codecInfoDoesRateConstrain | codecInfoDoesReorder | codecInfoDoesTemporal )

// These flags specify the possible format of compressed data produced by the component
// and the format of compressed files that the component can handle during decompression
#define kx264FormatFlags    ( codecInfoDepth24 | codecInfoSequenceSensitive )

// Component Description
resource 'cdci' (256) {
	kx264CodecFormatName,	// Type
	1,						// Version
	1,						// Revision level
	'x264',					// Manufacturer
	0,			// Decompression Flags
	kx264CompFlags,						// Compression Flags
	kx264FormatFlags,		// Format Flags
	128,					// Compression Accuracy
	0,					// Decomression Accuracy
	200,					// Compression Speed
	0,					// Decompression Speed
	128,					// Compression Level
	0,						// Reserved
	16,						// Minimum Height
	16,						// Minimum Width
	0,						// Decompression Pipeline Latency
	0,						// Compression Pipeline Latency
	0						// Private Data
};

resource 'thng' (256) {
	compressorComponentType,				// Type			
	kH264CodecType,									// SubType
	'x264',									// Manufacturer
        0,							// Component flags (use componentHasMultiplePlatforms)
	0,										// Component flags Mask
	0,									// Code Type
	0,									// Code ID
	'STR ',									// Name Type
	256,									// Name ID
	'STR ',									// Info Type
	257,									// Info ID
	0,										// Icon Type
	0,										// Icon ID
	0,                                                      // Version
	componentHasMultiplePlatforms +			// Registration Flags 
	componentDoAutoVersion + cmpThreadSafe,
	0,										// Resource ID of Icon Family
	{
		kx264CompFlags, 
		'dlle',								// Code Resource type - Entry point found by symbol name 'dlle' resource
		256,								// ID of 'dlle' resource
		platformPowerPCNativeEntryPoint,
	},    
        'thnr', 128
};

// Format Type

resource 'thnr' (128) {
    {
    'cdci', 1, 0, 'cdci', 256, cmpResourceNoFlags,
    'cpix', 1, 0, 'cpix', 128, cmpResourceNoFlags
    }
};

resource 'cpix' (128) {
{
    '2vuy', 'I420', 'yuvs'
}
};

resource 'ccop' (256) {
    kCodecCompressionNoQuality
};

// Component Name
resource 'STR ' (256) {
	"x264 H.264 Encoder"
};

// Component Information
resource 'STR ' (257) {
	"Compresses video into H.264 format using the x264 library."
};

// Code Entry Point for Mach-O and Windows
resource 'dlle' (256) {
    "x264CodecComponentDispatch"
};