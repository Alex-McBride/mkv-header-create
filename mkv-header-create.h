#ifndef MKV_HEADER_CREATE_H
#define MKV_HEADER_CREATE_H

extern uint64_t getHeader( uint8_t **headerptr,
                    uint8_t **segmentLengthStartPtr,
                    uint64_t *infoOffset,
                    uint64_t *tracksOffset,
                    uint64_t *cuesOffset,
                    uint64_t lastBlockTimestamp,
                    uint64_t clustersSize );

#endif
