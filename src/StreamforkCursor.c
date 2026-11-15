#include "StreamforkCursor.h"

#include <stdlib.h>
#include <string.h>

static uint16_t readLe16(const uint8_t* p) {
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t readLe32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void sfCursorShapeReset(PSF_CURSOR_SHAPE_RECV recv) {
    free(recv->data);
    recv->data = NULL;
    recv->received = 0;
}

int sfCursorShapeReceiveChunk(PSF_CURSOR_SHAPE_RECV recv, const uint8_t* payload, uint32_t length) {
    uint32_t shapeId, totalLength, offset, chunkLength;
    uint16_t width, height, hotX, hotY;

    if (length < SF_CURSOR_SHAPE_HEADER_SIZE) {
        return SF_CURSOR_CHUNK_INVALID;
    }

    shapeId = readLe32(payload);
    width = readLe16(payload + 4);
    height = readLe16(payload + 6);
    hotX = readLe16(payload + 8);
    hotY = readLe16(payload + 10);
    totalLength = readLe32(payload + 12);
    offset = readLe32(payload + 16);
    chunkLength = length - SF_CURSOR_SHAPE_HEADER_SIZE;

    // A new shape replaces any incomplete one
    if (offset == 0 || shapeId != recv->shapeId) {
        sfCursorShapeReset(recv);
        recv->shapeId = shapeId;

        if (offset != 0) {
            // We missed the start of this shape, so ignore the rest of it
            return SF_CURSOR_CHUNK_IGNORED;
        }

        if (shapeId == 0 ||
                width == 0 || width > SF_CURSOR_MAX_DIMENSION ||
                height == 0 || height > SF_CURSOR_MAX_DIMENSION ||
                totalLength != (uint32_t)width * height * 4) {
            return SF_CURSOR_CHUNK_IGNORED;
        }

        recv->data = malloc(totalLength);
        if (recv->data == NULL) {
            return SF_CURSOR_CHUNK_IGNORED;
        }

        recv->width = width;
        recv->height = height;
        recv->hotX = hotX < width ? hotX : (uint16_t)(width - 1);
        recv->hotY = hotY < height ? hotY : (uint16_t)(height - 1);
        recv->totalLength = totalLength;
    }

    // Nothing to do if this shape is being ignored
    if (recv->data == NULL) {
        return SF_CURSOR_CHUNK_IGNORED;
    }

    // Chunks arrive in order on the reliable channel and never change the shape's header
    if (offset != recv->received || chunkLength > recv->totalLength - recv->received ||
            width != recv->width || height != recv->height || totalLength != recv->totalLength) {
        sfCursorShapeReset(recv);
        return SF_CURSOR_CHUNK_INVALID;
    }

    memcpy(recv->data + offset, payload + SF_CURSOR_SHAPE_HEADER_SIZE, chunkLength);
    recv->received += chunkLength;

    return recv->received == recv->totalLength ? SF_CURSOR_CHUNK_COMPLETE : SF_CURSOR_CHUNK_PENDING;
}
