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

#ifndef CP210x_RingBuffer_h
#define CP210x_RingBuffer_h

#include <libkern/c++/OSContainers.h>
#include <IOKit/IOLib.h>

/**
 * Generic data ring buffer.
 *
 * This data structure provides no internal locking and must not be concurrently modified
 * by multiple threads.
 */
class coop_plausible_CP210x_RingBuffer : public OSObject {
    OSDeclareDefaultStructors(coop_plausible_CP210x_RingBuffer);
    
private:
    /** Total buffer capacity */
    uint32_t _capacity;

    /** Backing buffer. */
    uint8_t *_buf;
    
    /** Position of first readable element */
    uint32_t _readPos;
    
    /** Position of the first writable element */
    uint32_t _writePos;
    
    /** Length of available bytes */
    uint32_t _length;
    
public:

    virtual bool init (uint32_t capacity);
    virtual void free ();

    uint32_t write (const void *buf, uint32_t len);
    uint32_t read (void *buf, uint32_t nbyte);

    uint32_t getCapacity ();
    uint32_t getLength ();
};

#if DEBUG
extern void coop_plausible_CP210x_RingBuffer_tests ();
#endif

#endif
