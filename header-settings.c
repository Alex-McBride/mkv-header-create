/* Copyright (C) Alex McBride - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Alex McBride <alexmcb@gmail.com>, 2015
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h> 
#include "mkv-header-create.h"
#include "context.h"

typedef struct {
    context_common_t common;
    uint64_t infoOffset;
    uint64_t tracksOffset;
    uint64_t cuesOffset;
    uint64_t lastBlockTimestamp;
} context_header_t;

static uint8_t private[] = { 0x00, 0x00, 0x01, 0xB0, 0x01, 0x00, 0x00, 0x01, 0xB5,
                             0x89, 0x13, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
                             0x20, 0x00, 0xC4, 0xF8, 0xB0, 0xBD, 0x88, 0x00, 0xCD, 
                             0x0A, 0x04, 0x16, 0x14, 0x43, 0x00, 0x00, 0x01, 0xB2,
                             0x4C, 0x61, 0x76, 0x63, 0x35, 0x35, 0x2E, 0x33, 0x39,
                             0x2E, 0x31, 0x30, 0x31 };
static char ourApp[] = "EdesixMeercat (simples)";
static uint64_t timecodeScale = 1000000;

static uint64_t vtrackNumber = 1;    
static uint64_t vtrackUID = 1;
static uint64_t vflagLacing = 0;
static char *vlanguage = "und";
static char *vcodecID = "V_MPEG4/ISO/ASP";
static uint64_t vtrackType = 1;
static uint64_t vdefaultDuration = 40000000;
static uint64_t vpixelWidth = 320;
static uint64_t vpixelHeight = 176;
static uint64_t vdisplayWidth = 306;
static uint64_t vdisplayHeight = 176;
static uint8_t *vcodecPrivate;
static uint64_t vcodecPrivateLength;

static uint64_t atrackNumber = 2;    
static uint64_t atrackUID = 2;
static uint64_t aflagLacing = 0;
static char *alanguage = "und";
static char *acodecID = "A_MPEG/L2";
static uint64_t atrackType = 2;
static uint64_t achannels = 1;
static double asamplingFrequency = 48000.0;
static uint64_t abitdepth = 16;


/**** HELPER FUNCTIONS ****/



/**** BEGINNING OF ELEMENTS ****/


void writeEBML( context_header_t *ctx )
{
    static const uint8_t headerblob[] = 
                        { 0x1A, 0x45, 0xDF, 0xA3, 0x01, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x23, 0x42, 0x86, 0x81, 0x01, 0x42,
                        0xF7, 0x81, 0x01, 0x42, 0xF2, 0x81, 0x04, 0x42, 0xF3,
                        0x81, 0x08, 0x42, 0x82, 0x88, 0x6D, 0x61, 0x74, 0x72,
                        0x6F, 0x73, 0x6B, 0x61, 0x42, 0x87, 0x81, 0x04, 0x42,
                        0x85, 0x81, 0x02 };
    memcpy( ctx->common.ptr, headerblob, sizeof(headerblob) );
    ctx->common.ptr += sizeof(headerblob);
}

void writeSeek( context_header_t *ctx, uint8_t seekID[], uint64_t seekpos )
{
    uint8_t *preSeekLength;
    uint8_t *postSeekLength;
    uint64_t seekLength;
    int i;

    *ctx->common.ptr++ = 0x4d;
    *ctx->common.ptr++ = 0xbb;
    
    preSeekLength = ctx->common.ptr;
    encodeAndWriteLengthUsing8BytesWithCtx( &ctx->common, 1 );
    postSeekLength = ctx->common.ptr;
    
    *ctx->common.ptr++ = 0x53;
    *ctx->common.ptr++ = 0xab;
    encodeAndWriteLengthWithCtx( &ctx->common , 4);
    /* seekID is 4 bytes because we can only seek to level 1 elements
     * and level 1 elements have 4 byte IDs  */
    for (i = 0; i < 4; i++)
    {
       *ctx->common.ptr++ = seekID[i]; 
    }
    *ctx->common.ptr++ = 0x53;
    *ctx->common.ptr++ = 0xac;
    encodeAndWriteLengthUsing8BytesWithCtx( &ctx->common, 8 );
    writeuint64_t( &ctx->common, seekpos, 8 );
    /* now we've written everything, we know the length.
     * Go back and patch it */
    
    seekLength = (uint64_t)(ctx->common.ptr - postSeekLength);
    encodeAndWriteLengthUsing8Bytes( &preSeekLength, seekLength );

}

void writeSeekHead( context_header_t *ctx ) 
{
    uint8_t *preLength;
    uint8_t *postLength;
    uint8_t infoID[] = { 0x15, 0x49, 0xa9, 0x66 };
    uint8_t tracksID[] = { 0x16, 0x54, 0xae, 0x6b };
    uint8_t cuesID[] = { 0x1c, 0x53, 0xbb, 0x6b };
    uint64_t length;

    *ctx->common.ptr++ = 0x11;
    *ctx->common.ptr++ = 0x4d;
    *ctx->common.ptr++ = 0x9b;
    *ctx->common.ptr++ = 0x74;

    preLength = ctx->common.ptr;
    encodeAndWriteLengthUsing8BytesWithCtx( &ctx->common, 1 );
    postLength = ctx->common.ptr;

    writeSeek( ctx, infoID, ctx->infoOffset );
    writeSeek( ctx, tracksID, ctx->tracksOffset );
    writeSeek( ctx, cuesID, ctx->cuesOffset );

    length = (uint64_t)(ctx->common.ptr - postLength);
    encodeAndWriteLengthUsing8Bytes( &preLength, length );
      
}

void writeTimecodeScale( context_header_t *ctx )
{
    uint8_t id[] = { 0x2a, 0xd7, 0xb1 };
    writeElementWithUint64_tDataWithBigID( &ctx->common, id, sizeof(id), timecodeScale );
}

void writeMuxingApp( context_header_t *ctx )
{
    *ctx->common.ptr++ = 0x4d;
    *ctx->common.ptr++ = 0x80;
    
    encodeAndWriteLengthWithCtx( &ctx->common, strlen( ourApp ) );   
    writeString( &ctx->common, ourApp );    
}

void writeWritingApp( context_header_t *ctx )
{
    *ctx->common.ptr++ = 0x57;
    *ctx->common.ptr++ = 0x41;
    
    encodeAndWriteLengthWithCtx( &ctx->common, strlen( ourApp ) );
    writeString( &ctx->common, ourApp );
}

void writeDuration( context_header_t *ctx )
{
    double duration;
    uint64_t millisecondsPerFrame;
    *ctx->common.ptr++ = 0x44;
    *ctx->common.ptr++ = 0x89;
   
    millisecondsPerFrame = vdefaultDuration / timecodeScale;
    /* duration is last timestamp + 1 more frame's length */
    duration = ctx->lastBlockTimestamp + millisecondsPerFrame;
    encodeAndWriteLengthWithCtx( &ctx->common, 8 );
    writeDouble( &ctx->common, duration ); 
}

void writeInfo( context_header_t *ctx )
{
    uint8_t *preLength;
    uint8_t *postLength;
    uint64_t length;

    *ctx->common.ptr++ = 0x15;
    *ctx->common.ptr++ = 0x49;
    *ctx->common.ptr++ = 0xa9;
    *ctx->common.ptr++ = 0x66;

    preLength = ctx->common.ptr;
    encodeAndWriteLengthUsing8BytesWithCtx( &ctx->common, 1 );
    postLength = ctx->common.ptr;

    writeTimecodeScale( ctx );   
    writeMuxingApp( ctx );
    writeWritingApp( ctx );
    writeDuration( ctx );

    length = (uint64_t)(ctx->common.ptr - postLength);
    encodeAndWriteLengthUsing8Bytes( &preLength, length );
    
}

void writeTrackNumber( context_header_t *ctx, uint64_t trackNumber )
{
    writeElementWithUint64_tData( &ctx->common, 0xd7, trackNumber );
}

void writeTrackUID( context_header_t *ctx, uint64_t trackUID )
{
    uint8_t id[] = { 0x73, 0xc5 };
    writeElementWithUint64_tDataWithBigID( &ctx->common, id, sizeof(id), trackUID );
}

void writeFlagLacing( context_header_t *ctx, uint64_t flagLacing )
{
    writeElementWithUint64_tData( &ctx->common, 0x9c, flagLacing );
}

void writeLanguage( context_header_t *ctx, char *lang )
{
    *ctx->common.ptr++ = 0x22;
    *ctx->common.ptr++ = 0xb5;
    *ctx->common.ptr++ = 0x9c;
    
    encodeAndWriteLengthWithCtx( &ctx->common, strlen( lang ) );
    writeString( &ctx->common, lang );
}

void writeCodecID( context_header_t *ctx, char *codecId )
{
    *ctx->common.ptr++ = 0x86;
    
    encodeAndWriteLengthWithCtx( &ctx->common, strlen( codecId ) );
    writeString( &ctx->common, codecId );
}

void writeTrackType( context_header_t *ctx, uint64_t trackType )
{
    writeElementWithUint64_tData( &ctx->common, 0x83, trackType );
}

void writeDefaultDuration( context_header_t *ctx, uint64_t defaultDuration )
{
    uint8_t id[] = { 0x23, 0xe3, 0x83 };
    writeElementWithUint64_tDataWithBigID( &ctx->common, id, sizeof(id), defaultDuration );
}

void writePixelWidth( context_header_t *ctx, uint64_t pixelWidth ) 
{
    writeElementWithUint64_tData( &ctx->common, 0xb0, pixelWidth );
}

void writePixelHeight( context_header_t *ctx, uint64_t pixelHeight )
{
    writeElementWithUint64_tData( &ctx->common, 0xba, pixelHeight );
}

void writeDisplayWidth( context_header_t *ctx, uint64_t displayWidth )
{
    uint8_t id[] = { 0x54, 0xb0 };
    writeElementWithUint64_tDataWithBigID( &ctx->common, id, sizeof(id), displayWidth );
}

void writeDisplayHeight( context_header_t *ctx, uint64_t displayHeight )
{
    uint8_t id[] = { 0x54, 0xba };
    writeElementWithUint64_tDataWithBigID( &ctx->common, id, sizeof(id), displayHeight );
}

void writeVideo( context_header_t *ctx , uint64_t pixelWidth,
                                  uint64_t pixelHeight,
                                  uint64_t displayWidth,
                                  uint64_t displayHeight )
{
    uint8_t *preLength;
    uint8_t *postLength;
    uint64_t length;

    *ctx->common.ptr++ = 0xe0;

    preLength = ctx->common.ptr;
    encodeAndWriteLengthUsing8BytesWithCtx( &ctx->common, 1 );
    postLength = ctx->common.ptr;
    
    writePixelWidth( ctx, pixelWidth );
    writePixelHeight( ctx, pixelHeight );
    writeDisplayWidth( ctx, displayWidth );
    writeDisplayHeight( ctx, displayHeight );
    
    length = (uint64_t)(ctx->common.ptr - postLength);
    encodeAndWriteLengthUsing8Bytes( &preLength, length );
}

void writeCodecPrivate( context_header_t *ctx, uint8_t *codecPrivate, uint64_t length )
{
    *ctx->common.ptr++ = 0x63;
    *ctx->common.ptr++ = 0xa2;
    encodeAndWriteLengthWithCtx( &ctx->common, length );
    memcpy( ctx->common.ptr, codecPrivate, length );
    ctx->common.ptr += length; 
}

void writeVideoTrack( context_header_t *ctx, uint64_t trackNumber,
                                      uint64_t trackUID,
                                      uint64_t flagLacing,
                                      char *language,
                                      char *codecID,
                                      uint64_t trackType,
                                      uint64_t defaultDuration,
                                      uint64_t pixelWidth,
                                      uint64_t pixelHeight,
                                      uint64_t displayWidth,
                                      uint64_t displayHeight,
                                      uint8_t *codecPrivate,
                                      uint64_t codecPrivateLength )
{
    uint8_t *preLength;
    uint8_t *postLength;
    uint64_t length;
    
    *ctx->common.ptr++ = 0xae;

    preLength = ctx->common.ptr;
    encodeAndWriteLengthUsing8BytesWithCtx( &ctx->common, 1 );
    postLength = ctx->common.ptr;

    writeTrackNumber( ctx, trackNumber );
    writeTrackUID( ctx, trackUID );
    writeFlagLacing( ctx, flagLacing );
    writeLanguage( ctx, language );
    writeCodecID( ctx, codecID );
    writeTrackType( ctx, trackType );
    writeDefaultDuration( ctx, defaultDuration );
    writeVideo( ctx, pixelWidth, pixelHeight, displayWidth, displayHeight);
    writeCodecPrivate( ctx, codecPrivate, codecPrivateLength );
     
    length = (uint64_t)(ctx->common.ptr - postLength);
    encodeAndWriteLengthUsing8Bytes( &preLength, length );
}

void writeChannels( context_header_t *ctx, uint64_t channels )
{
    writeElementWithUint64_tData( &ctx->common, 0x9f, channels );
}

void writeSamplingFrequency( context_header_t *ctx, double samplingFrequency )
{
    *ctx->common.ptr++ = 0xb5;
    
    encodeAndWriteLengthWithCtx( &ctx->common, 8 );
    writeDouble( &ctx->common, samplingFrequency ); 
}

void writeBitdepth( context_header_t *ctx, uint64_t bitdepth )
{
    uint8_t id[] = { 0x62, 0x64 };
    writeElementWithUint64_tDataWithBigID( &ctx->common, id, sizeof(id), bitdepth );
}

void writeAudio( context_header_t *ctx, uint64_t channels,
                                 double samplingFrequency,
                                 uint64_t bitdepth )
{
    uint8_t *preLength;
    uint8_t *postLength;
    uint64_t length;

    *ctx->common.ptr++ = 0xe1;

    preLength = ctx->common.ptr;
    encodeAndWriteLengthUsing8BytesWithCtx( &ctx->common, 1 );
    postLength = ctx->common.ptr;

    writeChannels( ctx, channels );
    writeSamplingFrequency( ctx, samplingFrequency );
    writeBitdepth( ctx, bitdepth );

    length = (uint64_t)(ctx->common.ptr - postLength);
    encodeAndWriteLengthUsing8Bytes( &preLength, length );
}

void writeAudioTrack( context_header_t *ctx, uint64_t trackNumber,
                                      uint64_t trackUID,
                                      uint64_t flagLacing,
                                      char *language,
                                      char *codecID,
                                      uint64_t trackType,
                                      uint64_t channels,
                                      double samplingFrequency,
                                      uint64_t bitdepth )
{
    uint8_t *preLength;
    uint8_t *postLength;
    uint64_t length;

    *ctx->common.ptr++ = 0xae;

    preLength = ctx->common.ptr;
    encodeAndWriteLengthUsing8BytesWithCtx( &ctx->common, 1 );
    postLength = ctx->common.ptr;

    writeTrackNumber( ctx, trackNumber );
    writeTrackUID( ctx, trackUID );
    writeFlagLacing( ctx, flagLacing );
    writeLanguage( ctx, language );
    writeCodecID( ctx, codecID );
    writeTrackType( ctx, trackType );
    writeAudio( ctx, channels, samplingFrequency, bitdepth );
 
    length = (uint64_t)(ctx->common.ptr - postLength);
    encodeAndWriteLengthUsing8Bytes( &preLength, length );
}

uint64_t getCodecPrivate( uint8_t **ret )
{
    *ret = malloc( sizeof(private) * sizeof(uint8_t) );
    memcpy( *ret, private, sizeof(private) ); 
    return sizeof(private);
}

void writeTracks( context_header_t *ctx )
{
    uint8_t *preLength;
    uint8_t *postLength;
    uint64_t length;
    

    *ctx->common.ptr++ = 0x16;
    *ctx->common.ptr++ = 0x54;
    *ctx->common.ptr++ = 0xae;
    *ctx->common.ptr++ = 0x6b;

    preLength = ctx->common.ptr;
    encodeAndWriteLengthUsing8BytesWithCtx( &ctx->common, 1 );
    postLength = ctx->common.ptr;

    vcodecPrivateLength = getCodecPrivate( &vcodecPrivate );
    writeVideoTrack( ctx,
                     vtrackNumber,
                     vtrackUID,
                     vflagLacing,
                     vlanguage,
                     vcodecID,
                     vtrackType,
                     vdefaultDuration,
                     vpixelWidth,
                     vpixelHeight,
                     vdisplayWidth,
                     vdisplayHeight,
                     vcodecPrivate,
                     vcodecPrivateLength );
    free( vcodecPrivate );
    writeAudioTrack( ctx,
                     atrackNumber,
                     atrackUID,
                     aflagLacing,
                     alanguage,
                     acodecID,
                     atrackType,
                     achannels,
                     asamplingFrequency,
                     abitdepth );
    
    length = (uint64_t)(ctx->common.ptr - postLength);
    encodeAndWriteLengthUsing8Bytes( &preLength, length );
}

void writeSegment( context_header_t *ctx,  uint64_t clustersSize )
{
    uint8_t *segmentLengthStart;
    uint8_t *segmentDataStart;
    uint64_t length;
    
    *ctx->common.ptr++ = 0x18;
    *ctx->common.ptr++ = 0x53;
    *ctx->common.ptr++ = 0x80;
    *ctx->common.ptr++ = 0x67;

    segmentLengthStart = ctx->common.ptr;
    encodeAndWriteLengthUsing8BytesWithCtx( &ctx->common, 1 );
    segmentDataStart = ctx->common.ptr;

    writeSeekHead( ctx );
    ctx->infoOffset = (uint64_t)(ctx->common.ptr - segmentDataStart);
    writeInfo( ctx );
    ctx->tracksOffset = (uint64_t)(ctx->common.ptr - segmentDataStart);
    writeTracks( ctx ); 
    /* cues will come after clusters */
    ctx->cuesOffset = (uint64_t)(ctx->common.ptr - segmentDataStart) + clustersSize;
    
    length = ((uint64_t)(ctx->common.ptr - segmentDataStart)) + clustersSize;
    encodeAndWriteLengthUsing8Bytes( &segmentLengthStart, length );
}

uint64_t getHeader( uint8_t **headerptr,
                    uint8_t **segmentLengthStartPtr,
                    uint64_t *infoOffset,
                    uint64_t *tracksOffset,
                    uint64_t *cuesOffset,
                    uint64_t lastBlockTimestamp,
                    uint64_t clustersSize )
{
    uint8_t *headerbytes = malloc( 2000 ); /* should be enough... */
    context_header_t ctx;
    uint8_t *segmentStart;
    uint64_t headerLength;
    
    ctx.common.ptr = headerbytes;
    ctx.infoOffset = *infoOffset;
    ctx.tracksOffset = *tracksOffset;
    ctx.cuesOffset = *cuesOffset;
    ctx.lastBlockTimestamp = lastBlockTimestamp;

    writeEBML( &ctx );
    segmentStart = ctx.common.ptr;
    *segmentLengthStartPtr = segmentStart + 4; /* start of segment + ID bytes */
    writeSegment( &ctx, clustersSize );
     
    *headerptr = headerbytes;
    *infoOffset = ctx.infoOffset;
    *tracksOffset = ctx.tracksOffset;
    *cuesOffset = ctx.cuesOffset;
    headerLength = (uint64_t)(ctx.common.ptr - headerbytes);
    return headerLength;
}
