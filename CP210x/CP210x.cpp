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

#include <IOKit/IOLib.h>

#include "CP210x.h"
#include "RingBuffer.h"

#include "logging.h"

// Define the superclass
#define super IOSerialDriverSync

OSDefineMetaClassAndStructors(coop_plausible_driver_CP210x, super);

// from IOService base class 
IOService *coop_plausible_driver_CP210x::probe (IOService *provider, SInt32 *score) {
    IOService *res = super::probe(provider, score);
    LOG_DEBUG("probe");
    return res;
}

// from IOService base class
bool coop_plausible_driver_CP210x::start (IOService *provider) {
    LOG_DEBUG("Driver starting");

    if (!super::start(provider)) {
        LOG_ERR("super::start() failed");
        return false;
    }
    
    _lock = IOLockAlloc();

    /* Fetch our USB provider */
    _provider = OSDynamicCast(IOUSBInterface, provider);
    if (_provider == NULL) {
        LOG_ERR("Received invalid provider");
        return false;
    }

    /* Create our child serial stream */
    _serialDevice = new coop_plausible_CP210x_SerialDevice();
    if (!_serialDevice->init(this, _provider)) {
        LOG_ERR("Could not create serial stream");
        return false;
    }
    
    LOG_DEBUG("Driver started");

    _provider->retain();

#if DEBUG
    /* Run the debug build unit tests */
    LOG_DEBUG("Running tests");
    coop_plausible_CP210x_RingBuffer_tests();
    LOG_DEBUG("Tests complete");
#endif

    return true;
}

// from IOService base class
void coop_plausible_driver_CP210x::stop (IOService *provider) {
    LOG_DEBUG("Driver stopping");

    if (_lock != NULL) {
        IOLockFree(_lock);
        _lock = NULL;
    }

    if (_provider != NULL) {
        _provider->release();
        _provider = NULL;
    }
    
    if (_serialDevice != NULL) {
        _serialDevice->release();
        _serialDevice = NULL;
    }

    LOG_DEBUG("Driver stopped");
    super::stop(provider);
}

// from IOService base class
void coop_plausible_driver_CP210x::free (void) {
    LOG_DEBUG("free\n");
    super::free();
}

// from IOService base class
IOReturn coop_plausible_driver_CP210x::message(UInt32 type, IOService *provider, void *argument) {
    // TODO

    switch (type) {
        case kIOMessageServiceIsTerminated:
            return kIOReturnSuccess;			
        case kIOMessageServiceIsSuspended: 	
            break;			
        case kIOMessageServiceIsResumed: 	
            break;			
        case kIOMessageServiceIsRequestingClose: 
            break;
        case kIOMessageServiceWasClosed: 	
            break;
        case kIOMessageServiceBusyStateChange: 	
            break;
        case kIOUSBMessagePortHasBeenResumed: 	
            return kIOReturnSuccess;
        case kIOUSBMessageHubResumePort:
            break;
        case kIOUSBMessagePortHasBeenReset:
            return kIOReturnSuccess;
        default:
            break;
    }
    
    return super::message(type, provider, argument);
}


// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::acquirePort(bool sleep, void *refCon) {
    // TODO
    return kIOReturnOffline;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::releasePort(void *refCon) {
    // TODO
    return kIOReturnOffline;
}

// from IOSerialDriverSync
UInt32 coop_plausible_driver_CP210x::getState(void *refCon) {
    // TODO
    return 0;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::setState(UInt32 state, UInt32 mask, void *refCon) {
    // TODO
    return kIOReturnOffline;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::watchState(UInt32 *state, UInt32 mask, void *refCon) {
    // TODO
    return kIOReturnOffline;
}

// from IOSerialDriverSync
UInt32 coop_plausible_driver_CP210x::nextEvent(void *refCon) {
    // TODO
    return kIOReturnOffline;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::executeEvent(UInt32 event, UInt32 data, void *refCon) {
    // TODO
    return kIOReturnOffline;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::requestEvent(UInt32 event, UInt32 *data, void *refCon) {
    // TODO
    return kIOReturnOffline;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::enqueueEvent(UInt32 event, UInt32 data, bool sleep, void *refCon) {
    // TODO
    return kIOReturnOffline;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::dequeueEvent(UInt32 *event, UInt32 *data, bool sleep, void *refCon) {
    // TODO
    return kIOReturnOffline;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::enqueueData(UInt8 *buffer, UInt32 size, UInt32 * count, bool sleep, void *refCon) {
    // TODO
    return kIOReturnOffline;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::dequeueData(UInt8 *buffer, UInt32 size, UInt32 *count, UInt32 min, void *refCon) {
    // TODO
    return kIOReturnOffline;
}

/* Private Methods */
