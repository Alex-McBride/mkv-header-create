/* Copyright (C) Alex McBride - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Alex McBride <alexmcb@gmail.com>, 2015
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "context.h"

void copyBackwards( uint8_t *dest, void *_src, size_t size )
{
    uint8_t *src = _src;
    int i;

    for( i = size - 1; i >= 0; i-- )
    {
        *dest++ = src[i];
    } 
}

void writeuint64_t( context_common_t *ctx, uint64_t num, uint8_t bytesToRepresent )
{
    uint8_t shiftAmount = (8 * (bytesToRepresent - 1));
    uint64_t mask = (uint64_t)0xFF << shiftAmount; 
    while( mask > 0 )
    {
        *ctx->ptr++ = (num & mask) >> shiftAmount;
        mask >>= 8;
        shiftAmount -= 8;
    }
}

void writeDouble( context_common_t *ctx, double num )
{
    
    copyBackwards( ctx->ptr, &num, sizeof(double) );
    ctx->ptr += sizeof(double);
}

void writeString( context_common_t *ctx, char *str )
{
    uint64_t stringLength;
    int i;
    stringLength = strlen( str );
    for (i = 0; i < stringLength; i++)
    {
        *ctx->ptr++ = str[i];
    }
}

uint8_t bytesToRepresentNum( uint64_t number )
{
    uint64_t mask = ~(0xFF);
    uint8_t bytesToRepresent = 1;
    while ( (mask & number) > 0 )
    {
        bytesToRepresent++;
        mask <<= 8;
    }
    return bytesToRepresent;
}

uint8_t encodeLength( uint64_t length, uint8_t **lengthBytes)
{
    uint8_t shiftAmount;
    uint8_t bytesNeededToRepresent;
    uint8_t *lengthDescriptor;
    uint8_t mask;
    uint8_t x;
    
    bytesNeededToRepresent = 1;
    shiftAmount = 7;
    while ( length > ( (2 << shiftAmount) - 2 ) )
    {
        bytesNeededToRepresent++;
        shiftAmount += 7;
    }
     
    lengthDescriptor = (uint8_t *)malloc( bytesNeededToRepresent * sizeof(uint8_t) );
    lengthDescriptor[0] = 1 << (8 - (bytesNeededToRepresent) );
    mask = ~(lengthDescriptor[0]);
    lengthDescriptor[0] |= ( mask << ( 8 * (bytesNeededToRepresent - 1) ) ) & length;
    for ( x = 1; x < bytesNeededToRepresent; x++ )
    {
        lengthDescriptor[x] = (0xFF << ( 8 * (bytesNeededToRepresent - x - 1) ) ) & length; 
    }
    *lengthBytes = lengthDescriptor;
    return bytesNeededToRepresent;
}

uint8_t encodeLengthUsing8Bytes( uint64_t length, uint8_t **lengthBytes )
{
    uint8_t *lengthDescriptor;
    uint64_t mask;
    uint8_t shiftAmount = 0;
    int x;

    lengthDescriptor = malloc( 8 * sizeof(uint8_t) );
    lengthDescriptor[0] = 0x01;
    mask = 0xFF;
    for ( x = 7; x > 0; x-- )
    {
        lengthDescriptor[x] = (length & mask) >> shiftAmount;
        mask <<= 8;
        shiftAmount += 8;
    }
    *lengthBytes = lengthDescriptor;
    return 8;
}

void encodeAndWriteLength( uint8_t **ptr, uint64_t length )
{
    uint8_t *lengthBytes;
    uint64_t lengthOfLengthBytes = encodeLength( length, &lengthBytes );
    memcpy( *ptr, lengthBytes, lengthOfLengthBytes );
    *ptr += lengthOfLengthBytes;
    free( lengthBytes );
}

void encodeAndWriteLengthUsing8Bytes( uint8_t **ptr, uint64_t length )
{    
    uint8_t *lengthBytes;
    uint64_t lengthOfLengthBytes = encodeLengthUsing8Bytes( length, &lengthBytes );
    memcpy( *ptr, lengthBytes, lengthOfLengthBytes );
    *ptr += lengthOfLengthBytes;
    free( lengthBytes );
}

void encodeAndWriteLengthWithCtx( context_common_t *ctx, uint64_t length )
{
    int i = 0;
    uint8_t *lengthBytes;
    uint64_t lengthOfLengthBytes = encodeLength( length, &lengthBytes );
    for (i = 0; i < lengthOfLengthBytes; i++)
        *ctx->ptr++ = lengthBytes[i];
    free( lengthBytes );
}

void encodeAndWriteLengthUsing8BytesWithCtx( context_common_t *ctx, uint64_t length )
{
    int i = 0;
    uint8_t *lengthBytes;
    uint64_t lengthOfLengthBytes = encodeLengthUsing8Bytes( length, &lengthBytes );
    for (i = 0; i < lengthOfLengthBytes; i++)
        *ctx->ptr++ = lengthBytes[i];
    free( lengthBytes );
}

void writeElementWithUint64_tDataWithBigID( context_common_t *ctx, uint8_t id[], uint8_t idlen, uint64_t data )
{
    uint8_t bytesNeededToRepresent;
    int i;
    for( i = 0; i < idlen; i++ )
        *ctx->ptr++ = id[i];
    bytesNeededToRepresent = bytesToRepresentNum( data );
    encodeAndWriteLengthWithCtx( ctx, bytesNeededToRepresent );
    writeuint64_t( ctx, data, bytesNeededToRepresent ); 
}

/* used to write an element with id length 1 and contents of a uint64_t */
void writeElementWithUint64_tData( context_common_t *ctx, uint8_t id, uint64_t data )
{
    uint8_t bytesNeededToRepresent;
    *ctx->ptr++ = id;
    bytesNeededToRepresent = bytesToRepresentNum( data );
    encodeAndWriteLengthWithCtx( ctx, bytesNeededToRepresent );
    writeuint64_t( ctx, data , bytesNeededToRepresent ); 
}
