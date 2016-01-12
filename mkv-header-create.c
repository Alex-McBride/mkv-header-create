#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "mkv-header-create.h"
#include "context.h"
#include "cues.h"
#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)


int main( int argc, char *argv[] )
{
    FILE *vidptr;
    FILE *vidptrs[argc - 1];
    uint64_t vidLengths[argc - 1];
    FILE *headerptr;
    FILE *cuesptr;
    uint8_t *vidbuf;
    uint8_t *headerbytes;
    uint64_t headerlen;
    uint8_t *segmentLengthStartPtr;
    uint8_t *cuesBytes;
    uint64_t cuesLength;
    uint64_t infoOffset;
    uint64_t tracksOffset;
    uint64_t cuesOffset;
    uint64_t lastBlockTimestamp;
    uint64_t totalVideoLength = 0;
    int x;
    
    if ( IS_BIG_ENDIAN )
    {
        printf( "ERROR: this doesn't run on big endian systems.\n" );
        return -1;
    }

    if ( argc < 4 ) {
        printf( "ERROR: please pass in the name of the header file, the name of the cue"
                "file, and then 1 or more clusters files\n" );
        return -1;   
    }
    for ( x = 3; x < argc; x++ )
    { 
        vidptr = fopen( argv[x], "rb" );
        vidbuf = (uint8_t *)malloc( 8 * sizeof(uint8_t) );
        /* Read the first 4 bytes to check that we're being passed a file
         * containing clusters. */ 
        readNBytes( vidbuf, 4, vidptr );
        if( !equalsClusterID( vidbuf ) )
        {
            printf("ERROR: first 4 bytes of file '%s' were not equal to Cluster ID!\n"
                    "Are you sure this file contains just clusters?\n", argv[x]);
            return 0;
        }
        /* Find out how big the clusters file is */
        fseek( vidptr, 0, SEEK_END);
        vidLengths[x-3] = ftell( vidptr );
        totalVideoLength += vidLengths[x-3];
        rewind( vidptr );
        free( vidbuf );
        vidptrs[x-3] = vidptr;
        
    }
    /* Generate the header */
    printf( "Generating header...\n" );
    headerlen = getHeader( &headerbytes,
                           &segmentLengthStartPtr,
                           &infoOffset,
                           &tracksOffset,
                           &cuesOffset,
                           lastBlockTimestamp,
                           totalVideoLength );

    
    printf( "Generating cues...\n" );
    cuesLength = generateAndWriteCues( &cuesBytes,
                                       totalVideoLength,
                                       headerlen - END_OF_SEGMENT_LENGTH_OFFSET,
                                       &lastBlockTimestamp,
                                       vidptrs,
                                       vidLengths,
                                       (argc - 3));
    
    printf( "Generating header again!\n" );
    headerlen = getHeader( &headerbytes,
                           &segmentLengthStartPtr,
                           &infoOffset,
                           &tracksOffset,
                           &cuesOffset,
                           lastBlockTimestamp,
                           totalVideoLength );

    for( x = 0; x < argc - 3; x++ )
    {
        fclose( vidptrs[x] );
    }
    /* patch in the segment length to include cues size */
    encodeAndWriteLengthUsing8Bytes( &segmentLengthStartPtr, headerlen + totalVideoLength + cuesLength );
    printf( "Generated, writing...\n" );
    headerptr = fopen( argv[1], "wb" );
    fwrite( headerbytes, 1, headerlen, headerptr );
    free( headerbytes );
    fclose( headerptr );

    cuesptr = fopen( argv[2], "wb" );
    fwrite( cuesBytes, 1, cuesLength, cuesptr );
    free( cuesBytes );
    fclose( cuesptr ); 

    printf( "Done!\n");
    return 0;
}
