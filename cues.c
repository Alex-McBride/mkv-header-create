#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "context.h"
#include "cues.h"

typedef struct context {
    context_common_t common;
} context_cues_t;

typedef enum { READ_VID_BLOCK,
               SKIPPED_VID_BLOCK,
               SKIPPED_NON_VID_BLOCK,
               BLOCK_READ_ERROR } readBlockResult_t;

int equalsClusterID( uint8_t bytes[] )
{
    int x;
    uint8_t clusterId[] = { CLUSTER_EBML_ID };
    for (x = 0; x < sizeof(clusterId); x++)
    {
        if ( clusterId[x] != bytes[x] )
            return 0;
    }

    return 1;
}


int readNBytes( uint8_t *buf, int n, FILE *file)
{
    int bytesread;
    bytesread = fread( buf, sizeof(uint8_t), n, file );
    if( bytesread != n )
    {
        printf("ERROR: Tried to read %d bytes, actually read %d bytes\n",
                n,
                bytesread );
    }
    return bytesread;
}

uint32_t readLength( FILE *vidptr )
{
    uint32_t length;
    uint8_t buf;
    uint8_t numBytesInLengthDescriptor;
    uint8_t mask;

    readNBytes( &buf, 1, vidptr );
    mask = 0x80;
    numBytesInLengthDescriptor = 1;
    while( (buf & mask) == 0 )
    {
        if (mask == 0)
        {
            printf( "ERROR: first byte of length descriptor was %d\n", buf );
        }
        mask >>= 1;
        numBytesInLengthDescriptor++;
    }
    /* mask is now left with the bit set at the position of the first 1.
     * ~mask will get us everything in the first byte that isn't the first 1.*/
    length = (buf & (~mask));
    while ( numBytesInLengthDescriptor-- > 1 )
    {
        length <<= 8;
        readNBytes( &buf, 1, vidptr );
        length |= buf;
    }
    return length;
}

readBlockResult_t generateCueFromBlock( uint64_t clusterOffset,
                                        uint64_t clusterEndOfIDOffset,
                                        uint64_t clusterTimestamp,
                                        cue_t **ret,
                                        uint8_t vidBlocksToSkip,
                                        FILE *vidptr )
{
    cue_t *cue;
    uint8_t *buf;
    int16_t blockTimestamp;
    uint64_t blockEndPos;
    uint64_t blockLength;

    cue = malloc( sizeof(cue_t) );
    buf = malloc( 8 * sizeof(uint8_t) );
    
    cue->clusterpos = clusterOffset;
    
    /* blockrelativepos refers to the difference between the position at
     * the end of the cluster length, and the position at the "A3" block
     * ID. */  
    cue->blockrelativepos = ftell( vidptr ) - clusterEndOfIDOffset;
    readNBytes( buf, 1, vidptr );
    if( !( buf[0] == SIMPLEBLOCK_EBML_ID ) )
    {
        printf( "ERROR: Expected simpleblock id, got 0x%02x at 0x%lx\n", buf[0], ftell( vidptr ) );
        free( cue );
        free( buf );
        return BLOCK_READ_ERROR;
    }
    /* Now read the length of the block so we can skip over it afterwards */
    blockLength = readLength( vidptr );
    blockEndPos = ftell ( vidptr ) + blockLength;

    /* Now read which track this block belongs to.
     * It's formatted like a length, so we can call just call readLength */
    cue->track = readLength( vidptr );
    /* If this isn't a video block, don't bother creating a cue for it */
    /* TODO: Actually know which track is video, rather than assume track 1 */
    if( cue->track != 1 )
    {
        free( buf );
        free( cue );
        fseek( vidptr, blockEndPos, SEEK_SET );
        return SKIPPED_NON_VID_BLOCK;
    }
    /* Now read the timecode, which is a signed int16. */
    readNBytes( buf, 2, vidptr );
    blockTimestamp = ( buf[0] << 8 ) | buf[1];
    cue->cuetime = clusterTimestamp + blockTimestamp;

    /* if vidBlocksToSkip > 0, then we should skip making a cue for this video block 
     * We still fill in the ret pointer because the caller might want to know the
     * timestamp of this block anyway */

    fseek( vidptr, blockEndPos, SEEK_SET );
    *ret = cue;
    free( buf );
    if( vidBlocksToSkip > 0 )
        return SKIPPED_VID_BLOCK;
    return READ_VID_BLOCK;
}

void writeCueTime( context_cues_t *ctx, cue_t *cue )
{
    writeElementWithUint64_tData( &(ctx->common), 0xb3, cue->cuetime );
}

void writeCueClusterPosition( context_cues_t *ctx, cue_t *cue )
{
    writeElementWithUint64_tData( &(ctx->common), 0xf1, cue->clusterpos );
}

void writeCueTrack( context_cues_t *ctx, cue_t *cue )
{
    writeElementWithUint64_tData( &(ctx->common), 0xf7, cue->track );
}

void writeCueRelativePosition( context_cues_t *ctx, cue_t *cue )
{
    writeElementWithUint64_tData( &(ctx->common), 0xf0, cue->blockrelativepos );
}

void writeCueTrackPositions( context_cues_t *ctx, cue_t *cue )
{
    uint8_t *preLength;
    uint8_t *postLength;
    uint64_t length;

    *ctx->common.ptr++ = 0xb7;

    preLength = ctx->common.ptr;
    encodeAndWriteLengthUsing8BytesWithCtx( &(ctx->common), 1 );
    postLength = ctx->common.ptr;

    writeCueClusterPosition( ctx, cue );
    writeCueTrack( ctx, cue );
    writeCueRelativePosition( ctx, cue );
    
    length = (uint64_t)(ctx->common.ptr - postLength);
    encodeAndWriteLengthUsing8Bytes( &preLength, length );
}

void writeCuePoint( context_cues_t *ctx, cue_t *cue )
{
    uint8_t *preLength;
    uint8_t *postLength;
    uint64_t length;

    *ctx->common.ptr++ = 0xbb;
    
    preLength = ctx->common.ptr;
    encodeAndWriteLengthUsing8BytesWithCtx( &(ctx->common), 1 );
    postLength = ctx->common.ptr;

    writeCueTime( ctx, cue );
    writeCueTrackPositions( ctx, cue );

    length = (uint64_t)(ctx->common.ptr - postLength);
    encodeAndWriteLengthUsing8Bytes( &preLength, length );
}

uint64_t generateAndWriteCues( uint8_t **cuesBytes,
                               uint64_t fileLength,
                               uint64_t endOfSegmentLengthOffset,
                               uint64_t *lastBlockTimestamp,
                               FILE *vidptrs[],
                               uint64_t vidLengths[],
                               int vidCount )
{
    FILE *vidptr;
    uint8_t *cues;
    cue_t *nextCue;
    uint8_t *preLength;
    uint8_t *postLength;
    uint64_t length;
    uint64_t bytesAlreadyProcessedInAnotherFile = 0;
    uint64_t clusterStartPos;
    uint64_t clusterEndOfIDPos;
    uint64_t clusterEndPos;
    uint64_t clusterTimestamp;
    uint8_t clusterTimestampLength;
    uint8_t vidBlocksToSkip = 0;
    uint64_t lastTimestampFound = 0;
    readBlockResult_t readResult;
    uint8_t *buf;
    int x;
    int vidIndex;
    context_cues_t ctx;

    cues = malloc( 1000000 );
    ctx.common.ptr = cues;
    buf = (uint8_t *)malloc( 8 * sizeof(uint8_t) ); /* we'll read at most 8 bytes at a time */

    *ctx.common.ptr++ = 0x1c;
    *ctx.common.ptr++ = 0x53;
    *ctx.common.ptr++ = 0xbb;
    *ctx.common.ptr++ = 0x6b;

    preLength = ctx.common.ptr;
    encodeAndWriteLengthUsing8BytesWithCtx( &(ctx.common), 1 );
    postLength = ctx.common.ptr;
    for( vidIndex = 0; vidIndex < vidCount; vidIndex++ )
    {
        vidptr = vidptrs[vidIndex];
        while( ftell( vidptr ) < vidLengths[vidIndex] )
        {
            clusterStartPos = ftell( vidptr ) + bytesAlreadyProcessedInAnotherFile;
            readNBytes( buf, 4, vidptr );
            if( !equalsClusterID( buf ) )
            {
                printf( "ERROR: expected cluster ID, got " );
                for( x = 0; x < 4; x++ )
                {
                    printf( "0x%02X ", buf[x] );
                }
                printf("\n");
                goto error;
            } 
            clusterEndPos = readLength( vidptr ) + ftell( vidptr );
            clusterEndOfIDPos = ftell( vidptr );
            /* now read the cluster timestamp */
            readNBytes( buf, 1, vidptr );
            if( !( buf[0] == TIMECODE_EBML_ID ) )
            {
                printf( "ERROR: expected timecode ebml id, got 0x%02x\n", buf[0] );
                goto error;
            }
            clusterTimestampLength = readLength( vidptr );
            readNBytes( buf, clusterTimestampLength, vidptr );
            clusterTimestamp = buf[0];
            for ( x = 1; x < clusterTimestampLength; x++ )
            {
                clusterTimestamp <<= 8;
                clusterTimestamp |= buf[x];
            }

            while( ftell ( vidptr ) < clusterEndPos )
            {
                readResult = generateCueFromBlock( clusterStartPos + endOfSegmentLengthOffset,
                                                   clusterEndOfIDPos,
                                                   clusterTimestamp,
                                                   &nextCue,
                                                   vidBlocksToSkip,
                                                   vidptr );
                if ( readResult == READ_VID_BLOCK )
                {
                    /* printf( "Generated cue for timestamp %lu\n", nextCue->cuetime ); */
                    vidBlocksToSkip = 11;
                    /* Write cue to memory */
                    lastTimestampFound = nextCue->cuetime;
                    writeCuePoint( &ctx, nextCue ); 
                    free( nextCue );
                } 
                else if ( readResult == SKIPPED_VID_BLOCK )
                {
                    vidBlocksToSkip--;
                    lastTimestampFound = nextCue->cuetime;
                    free( nextCue );
                }
            } 
        }
        /* we need to know how many bytes we have processed, so we can pretend
         * that a cluster in a subsequent video file exists in a position 
         * just "after" this video file */
        bytesAlreadyProcessedInAnotherFile += vidLengths[vidIndex];
    }
    
    length = (uint64_t)(ctx.common.ptr - postLength);
    encodeAndWriteLengthUsing8Bytes( &preLength, length );
    
    *cuesBytes = cues; 
    *lastBlockTimestamp = lastTimestampFound;
    free( buf ); 
    return length + 12; /*12 accounts for ID + length of cues element itself */
error:
    free( buf );
    return 1; 
}
