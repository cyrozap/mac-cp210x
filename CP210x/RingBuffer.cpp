/*
 * Author: Landon Fuller <landonf@plausible.coop>
 *
 * Copyright (c) 2012 Plausible Labs Cooperative, Inc.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "RingBuffer.h"

// TODO: lift out into a common header?

#define MAX(x, y) ({ \
    __typeof__(x) idempotent_x = (x); \
    __typeof__(y) idempotent_y = (y); \
    idempotent_x > idempotent_y ? idempotent_x : idempotent_y; \
})

#define MIN(x, y) ({ \
    __typeof__(x) idempotent_x = (x); \
    __typeof__(y) idempotent_y = (y); \
    idempotent_x < idempotent_y ? idempotent_x : idempotent_y; \
})

// Define the superclass
#define super OSObject

OSDefineMetaClassAndStructors(coop_plausible_CP210x_RingBuffer, super);

/**
 * Initialize a new ring buffer instance.
 *
 * @param capacity Total buffer capacity to be allocated, in bytes.
 */
bool coop_plausible_CP210x_RingBuffer::init (uint32_t capacity) {
    _buf = (uint8_t *) IOMalloc(capacity);

    _capacity = capacity;
    _readPos = 0;
    _writePos = 0;
    _length = 0;

    return true;
}

/**
 * Return the total capacity of the buffer in bytes.
 */
uint32_t coop_plausible_CP210x_RingBuffer::getCapacity () {
    return _capacity;
}

/**
 * Return the current number of bytes that are readable from
 * the buffer.
 */
uint32_t coop_plausible_CP210x_RingBuffer::getLength () {
    return _length;
}

/**
 * Discard all buffered data.
 */
void coop_plausible_CP210x_RingBuffer::flush () {
    _readPos = 0;
    _writePos = 0;
    _length = 0;
}

/**
 * Read up to @a nbyte from the buffer into @a buf. The actual number of bytes
 * read will be returned, and will be 0 if the buffer is empty.
 */
uint32_t coop_plausible_CP210x_RingBuffer::read (void *buf, uint32_t nbyte) {
    /* Determine the maximum readable amount */
    uint32_t adjlen = MIN(nbyte, _length);

    /* Truncate the length to what we can read before we hit the end of the buffer */
    adjlen = MIN(adjlen, _capacity - _readPos);
    assert(adjlen != 0);

    /* Perform the read and adjust the read position */
    memcpy(buf, _buf+_readPos, adjlen);
    _writePos = (_readPos + adjlen) % _capacity;
    _length -= adjlen;
    
    /* If more bytes can be read, use recursion to do so */
    if (adjlen < nbyte && _length > 0) {
        return adjlen + read((uint8_t *)buf+adjlen, nbyte-adjlen);
    } else {
        return adjlen;
    }

    return 0;
}

/**
 * Write up to @a len bytes from @a buf to the buffer. The number of bytes
 * written will be returned. If the buffer is full, 0 bytes will be written.
 */
uint32_t coop_plausible_CP210x_RingBuffer::write (const void *buf, uint32_t len) {
    /* Determine the maximum writable amount */
    uint32_t adjlen = MIN(len, _capacity - _length);

    /* Truncate the length to what can fit before we hit the end of the buffer */
    adjlen = MIN(adjlen, _capacity - _writePos);
    assert(adjlen != 0);

    /* Perform the write and adjust the write position */
    memcpy(_buf+_writePos, buf, adjlen);
    _writePos = (_writePos + adjlen) % _capacity;
    _length += adjlen;

    /* If more bytes can be written, use recursion to do so */
    if (adjlen < len && _length < _capacity) {
        return adjlen + write((uint8_t *)buf+adjlen, len-adjlen);
    } else {
        return adjlen;
    }
}

void coop_plausible_CP210x_RingBuffer::free () {
    IOFree(_buf, _capacity);

    super::free();
}

#if DEBUG

#define assertTrue(expr, message, ...) do { \
    if (!expr) { \
        IOLog("%s:%d:%s: " # expr " is false: " message "\n", __FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__); \
        goto cleanup; \
    } \
} while (0);

#define assertNotNull(expr, message, ...) assertTrue(expr != NULL, message, ## __VA_ARGS__)

#define assertEquals(expr, value, message, ...) assertTrue(expr == value, message __VA_ARGS__)

void coop_plausible_CP210x_RingBuffer_tests () {
    uint8_t byte = 0xFF;
    uint8_t readbuf[4] = { 0xA, 0xA, 0xA, 0xA };

    /* Create a new buffer */
    coop_plausible_CP210x_RingBuffer *rbuf = new coop_plausible_CP210x_RingBuffer();
    assertNotNull(rbuf, "Failed to create ring buffer");
    assertTrue(rbuf->init(2), "Failed to initialize ring buffer");
    
    /* Ensure an empty read returns 0 */
    assertEquals(rbuf->read(readbuf, 10), 0, "Read of empty buffer should return 0");

    /* Verify that we can perform a partial write into an empty buffer */
    assertEquals(rbuf->write(&byte, sizeof(byte)), 1, "Failed to write 1 byte to buffer");
    assertEquals(rbuf->write(&byte, sizeof(byte)), 1, "Failed to write 1 byte to buffer");
    
    /* Ensure that writing to a full buffer fails */
    assertEquals(rbuf->getLength(), rbuf->getCapacity(), "Buffer should be full");
    assertEquals(rbuf->write(&byte, sizeof(byte)), 0, "Writing 1 byte to a full buffer should have failed");

    /* Attempt reading a single byte. This will also position the buffer so that we can test reading wrap-around */
    assertEquals(rbuf->read(readbuf, 1), 1, "Failed to read a single byte off the buffer");
    assertEquals(readbuf[0], 0xFF, "Read incorrect data");
    assertEquals(readbuf[1], 0xA, "Read more data than requested");

    /* Write an addition byte, such that the buffer is again full. This will allow us to test a read that
     * crosses the buffer boundary */
    byte = 0xB;
    assertEquals(rbuf->write(&byte, sizeof(byte)), 1, "Failed to write 1 byte to buffer");
    assertEquals(rbuf->getLength(), rbuf->getCapacity(), "Buffer should be full");

    /* The buffer is now full, and the read position is directly in the middle. Attempt a full buffer read */
    readbuf[0] = 0xA; readbuf[1] = 0xA; readbuf[2] = 0xA;

    assertEquals(rbuf->read(readbuf, 2), 2, "Failed to read the expected 2 bytes");
    assertEquals(readbuf[0], 0xFF, "Read incorrect data");
    assertEquals(readbuf[1], 0xB, "Read incorrect data");
    assertEquals(readbuf[2], 0xA, "Read more data than requested");
    
    /* The buffer is now empty, and the write position should be directly in the middle. Attempt a full buffer write. */
    assertEquals(rbuf->getLength(), 0, "Buffer should be empty");
    assertEquals(rbuf->write(readbuf, 2), 2, "Failed to write 2 bytes to buffer");
    assertEquals(rbuf->getLength(), rbuf->getCapacity(), "Buffer should be full");

    /* Verify the last write */
    readbuf[0] = 0xA; readbuf[1] = 0xA; readbuf[2] = 0xA;
    assertEquals(rbuf->read(readbuf, 2), 2, "Failed to read the expected 2 bytes");
    assertEquals(readbuf[0], 0xFF, "Read incorrect data");
    assertEquals(readbuf[1], 0xB, "Read incorrect data");
    assertEquals(readbuf[2], 0xA, "Read more data than requested");
    assertEquals(rbuf->getLength(), 0, "Buffer should be empty");
    
    /* Verify that flushing a buffer empties its contents, and that writing works after a flush. */
    assertEquals(rbuf->write(readbuf, 3), 3, "Failed to write 2 bytes to buffer");
    rbuf->flush();
    assertEquals(rbuf->getLength(), 0, "Buffer length is not 0");

    readbuf[0] = 0xA; readbuf[1] = 0xB; readbuf[2] = 0xC;
    assertEquals(rbuf->write(readbuf, 3), 3, "Failed to write 2 bytes to buffer");

    assertEquals(rbuf->read(readbuf, 3), 3, "Failed to read the expected 2 bytes");
    assertEquals(readbuf[0], 0xA, "Read incorrect data");
    assertEquals(readbuf[1], 0xB, "Read incorrect data");
    assertEquals(readbuf[2], 0xC, "Read more data than requested");

cleanup:
    if (rbuf != NULL)
        rbuf->release();
}

#endif /* DEBUG */