#pragma once

// Reassembly of the streamfork CursorShape message (0x3106). This file has no
// dependencies besides the C standard library so it can be tested on its own.
//
// CursorShape payload (little-endian):
//   u32 shapeId, u16 width, u16 height, u16 hotX, u16 hotY, u32 totalLength, u32 offset, data[]
// The data is the cursor image in BGRA (straight alpha), width * height * 4 bytes,
// split into chunks that are sent in order on the reliable channel.

#include <stdbool.h>
#include <stdint.h>

#define SF_CURSOR_SHAPE_HEADER_SIZE (4 + 2 + 2 + 2 + 2 + 4 + 4)
#define SF_CURSOR_MAX_DIMENSION     256

typedef struct _SF_CURSOR_SHAPE_RECV {
    uint32_t shapeId;
    uint16_t width;
    uint16_t height;
    uint16_t hotX;
    uint16_t hotY;
    uint32_t totalLength;
    uint32_t received;
    uint8_t* data; // NULL when no shape is in progress or the current one is being ignored
} SF_CURSOR_SHAPE_RECV, *PSF_CURSOR_SHAPE_RECV;

// Results of sfCursorShapeReceiveChunk()
#define SF_CURSOR_CHUNK_PENDING  0 // Chunk stored, more chunks are needed
#define SF_CURSOR_CHUNK_COMPLETE 1 // The shape is complete in recv->data
#define SF_CURSOR_CHUNK_IGNORED  2 // Chunk of a shape we skip (missed start or invalid shape)
#define SF_CURSOR_CHUNK_INVALID  3 // Malformed or out of order chunk, the shape was dropped

// Handles one CursorShape payload. A new shapeId (or offset 0) replaces any
// incomplete shape. When SF_CURSOR_CHUNK_COMPLETE is returned, the caller
// consumes recv->data and then calls sfCursorShapeReset().
int sfCursorShapeReceiveChunk(PSF_CURSOR_SHAPE_RECV recv, const uint8_t* payload, uint32_t length);

// Frees any shape in progress
void sfCursorShapeReset(PSF_CURSOR_SHAPE_RECV recv);
