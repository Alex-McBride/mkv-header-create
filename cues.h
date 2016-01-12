/* Copyright (C) Alex McBride - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Alex McBride <alexmcb@gmail.com>, 2015
 */
#ifndef CUES_H
#define CUES_H

#define EBML_EBML_ID 0X1A, 0X45, 0XDF, 0XA3
#define SEGMENT_LENGTH_OFFSET 0x34
#define END_OF_SEGMENT_LENGTH_OFFSET 0x3b
#define CLUSTER_EBML_ID 0x1f, 0x43, 0xb6, 0x75
#define TIMECODE_EBML_ID 0xe7
#define SIMPLEBLOCK_EBML_ID 0xa3
#define CUEPOINT_EBML_ID 0XBB
#define CUETIME_EBML_ID 0XB3
#define CUETRACK_POSITIONS_EBML_ID 0XB7
#define CUETRACK_EBML_ID 0XF7
#define CUECLUSTERPOS_EBML_ID 0XF1
#define CUERELATIVEPOS_EBML_ID 0XF0

typedef struct cue {
    uint64_t cuetime;
    uint32_t track;
    uint64_t clusterpos;
    uint32_t blockrelativepos;
    struct cue *next;
} cue_t;

extern int readNBytes( uint8_t *buf, int n, FILE *file);
extern uint64_t generateAndWriteCues( uint8_t **cuesBytes,
                               uint64_t fileLength,
                               uint64_t endOfSegmentLengthOffset,
                               uint64_t *lastBlockTimestamp,
                               FILE *vidptrs[],
                               uint64_t vidLengths[],
                               int vidCount );
extern int equalsClusterID( uint8_t bytes[] );



#endif

