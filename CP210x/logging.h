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

#ifndef CP210x_debug_h
#define CP210x_debug_h

#include <IOKit/IOLib.h>

/* Define to 0 to disable debug logging */
#define CP210x_DEBUG 1

/** Log a message. It will be automatically prefixed with the driver name. */
#define CP210x_LOG(fmt,...) do { \
    IOLog("CP210x: " fmt "\n",## __VA_ARGS__); \
} while (0)

/** Log an error message. */
#define LOG_ERR(fmt,...) CP210x_LOG("[ERROR] " fmt,## __VA_ARGS__)

/** Log an informative message. */
#define LOG_INFO(fmt,...) CP210x_LOG(fmt,## __VA_ARGS__)

#if CP210x_DEBUG

/** Log a debugging message. */
#define LOG_DEBUG(fmt,...) CP210x_LOG("[D] " fmt,## __VA_ARGS__)

#else /* CP210x_DEBUG */
#define LOG_DEBUG(...) do { } while (0)
#endif /* !CP210x_DEBUG */


#endif /* CP210x_debug_h */
