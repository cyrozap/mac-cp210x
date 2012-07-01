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

// Define the superclass
#define super OSObject

OSDefineMetaClassAndStructors(coop_plausible_CP210x_RingBuffer, super);

/**
 * Initialize a new ring buffer instance.
 *
 * @param capacity Total buffer capacity to be allocated, in bytes.
 */
bool coop_plausible_CP210x_RingBuffer::init (vm_size_t capacity) {
    _buf = (uint8_t *) IOMalloc(capacity);

    _capacity = capacity;
    _start = 0;
    _length = 0;

    return true;
}

/**
 * Read up to @a nbyte from the buffer into @a buf. The actual number of bytes
 * read will be returned, and will be 0 if the buffer is empty.
 */
size_t coop_plausible_CP210x_RingBuffer::read (void *buf, size_t nbyte) {
    return 0;
}

/**
 * Write up to @a len bytes from @a buf to the buffer. The number of bytes
 * written will be returned. If the buffer is full, 0 bytes will be written.
 */
size_t coop_plausible_CP210x_RingBuffer::write (const void *buf, size_t len) {
    return 0;
}

void coop_plausible_CP210x_RingBuffer::free () {
    IOFree(_buf, _capacity);

    super::free();
}

#if DEBUG

#define assertTrue(expr, message, ...) do { \
    if (!expr) { \
        IOLog("%s: " # expr " is false: " message "\n", __FUNCTION__, ## __VA_ARGS__); \
        return; \
    } \
} while (0);

#define assertNotNull(expr, message, ...) assertTrue(expr != NULL, message, ## __VA_ARGS__)

void coop_plausible_CP210x_RingBuffer_tests () {
    coop_plausible_CP210x_RingBuffer *rbuf = new coop_plausible_CP210x_RingBuffer();
    assertNotNull(rbuf, "Failed to create ring buffer");

    assertTrue(rbuf->init(2), "Failed to initialize ring buffer");

    rbuf->release();
}

#endif /* DEBUG */