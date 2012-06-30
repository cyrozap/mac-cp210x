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

#include <IOKit/IOService.h>

#include <IOKit/serial/IOSerialDriverSync.h>
#include <IOKit/serial/IORS232SerialStreamSync.h>

#include <IOKit/usb/USB.h>

class coop_plausible_driver_CP210x : public IOSerialDriverSync {
    OSDeclareDefaultStructors(coop_plausible_driver_CP210x);
    
private:
    bool createSerialStream ();

public:
    
    // IOService
    
	virtual IOService *probe (IOService *provider, SInt32 *score);
    virtual bool start (IOService *provider);
    virtual void stop (IOService *provider);
    virtual void free (void);

    // TODO
    // virtual IOReturn message(UInt32 type, IOService *provider, void *argument = 0);

    // IOSerialDriverSync
    virtual IOReturn acquirePort (bool sleep, void *refCon);
    virtual IOReturn releasePort (void *refCon);

    virtual UInt32 getState (void *refCon);
    virtual IOReturn setState (UInt32 state, UInt32 mask, void *refCon);
    virtual IOReturn watchState (UInt32 *state, UInt32 mask, void *refCon);

    virtual UInt32 nextEvent (void *refCon);
    virtual IOReturn executeEvent (UInt32 event, UInt32 data, void *refCon);
    virtual IOReturn requestEvent (UInt32 event, UInt32 *data, void *refCon);
    virtual IOReturn enqueueEvent (UInt32 event, UInt32 data, bool sleep, void *refCon);
    virtual IOReturn dequeueEvent (UInt32 *event, UInt32 *data, bool sleep, void *refCon);

    virtual IOReturn enqueueData (UInt8 *buffer, UInt32 size, UInt32 * count, bool sleep, void *refCon);
    virtual IOReturn dequeueData (UInt8 *buffer, UInt32 size, UInt32 *count, UInt32 min, void *refCon);
};