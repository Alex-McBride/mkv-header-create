#ifndef CONTEXT_H
#define CONTEXT_H

typedef struct {
    uint8_t *ptr;
} context_common_t;

extern void copyBackwards( uint8_t *dest, void *_src, size_t size );
extern void writeuint64_t( context_common_t *ctx, uint64_t num, uint8_t bytesToRepresent );
extern void writeDouble( context_common_t *ctx, double num );
extern void writeString( context_common_t *ctx, char *str );
extern uint8_t bytesToRepresentNum( uint64_t num );
extern uint8_t encodeLength( uint64_t length, uint8_t **lengthBytes);
extern uint8_t encodeLengthUsing8Bytes( uint64_t length, uint8_t **lengthBytes );
extern void encodeAndWriteLength( uint8_t **ptr, uint64_t length );
extern void encodeAndWriteLengthUsing8Bytes( uint8_t **ptr, uint64_t length );
extern void encodeAndWriteLengthWithCtx( context_common_t *ctx, uint64_t length );
extern void encodeAndWriteLengthUsing8BytesWithCtx( context_common_t *ctx, uint64_t length );
extern void writeElementWithUint64_tDataWithBigID( context_common_t *ctx, uint8_t id[], uint8_t idlen, uint64_t data );
extern void writeElementWithUint64_tData( context_common_t *ctx, uint8_t id, uint64_t data );

#endif
