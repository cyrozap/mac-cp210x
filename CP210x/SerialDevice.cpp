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

#include "SerialDevice.h"

#include <IOKit/serial/IORS232SerialStreamSync.h>
#include <IOKit/serial/IOSerialKeys.h>

// Define the superclass
#define super OSObject

OSDefineMetaClassAndStructors(coop_plausible_CP210x_SerialDevice, super);

/**
 *
 */
bool coop_plausible_CP210x_SerialDevice::init (IOUSBDevice *device) {
    
    if (!super::init())
        return false;
    
    /* Create our child driver */
    bool result = true;

    _stream = new IORS232SerialStreamSync();
    if (_stream == NULL)
        return false;
    
    /* Initialize and attach */
    if ((result = _stream->init(0, 0)) == false)
        goto finish;

#if 0
    // TODO
    if ((result = _stream->attach(this)) == false)
        goto finish;
#endif

    /*
     * Configure the device node name. We use a fixed base name, and a suffix
     * based on uniquely identifying data.
     */
    {
        OSString *suffix = this->getDeviceNameSuffix(device);

        _stream->setProperty(kIOTTYBaseNameKey, "cp210x");
        _stream->setProperty(kIOTTYSuffixKey, suffix->getCStringNoCopy());

        suffix->release();
    }

#if 0
    // TODO
    /* Allow matching against our serial nub */
    _stream->registerService();
#endif
    
    // Fall-through on success
finish:
    if (!result) {
        _stream->release();
        _stream = NULL;
    }

    return result;
}

void coop_plausible_CP210x_SerialDevice::free() {
    if (_stream != NULL)
        _stream->release();
    
    super::free();
}

/* 
 * Determine a unique device name suffix for @a device. We attempt to determine
 * a value that will be unique across hardware models while also being constant
 * over time.
 *
 * @param device The CP210x USB device for which a suffix should be derived.
 *
 * @return Returns a retained device name suffix string. It is the caller's responsibility to release this string.
 *
 * @note Unless a manufacturer programs the EEPROM of the CP210x, the serial number will
 * be set to a default value of 0001, as per the CP210x data sheets. Unfortunately, this
 * is the case with Aeon Labs Z-Stick Series 2, and likely other CP210x devices.
 *
 * We attempt to identify this case, and avoid the use of the non-unique serial number
 * as a suffix.
 */
OSString *coop_plausible_CP210x_SerialDevice::getDeviceNameSuffix (IOUSBDevice *device) {
    UInt8 idx;

    /* First, we try to use the device serial number */
    idx = device->GetSerialNumberStringIndex();
    if (idx != 0) {
        /* 256 is the maximum possible descriptor length */
        char serialStr[256];

        IOReturn ret = device->GetStringDescriptor(idx, serialStr, sizeof(serialStr));
        if (ret == kIOReturnSuccess && strnlen(serialStr, sizeof(serialStr)) > 0) {
            return OSString::withCString(serialStr);
        }
        
        /* Otherwise, fall through to the other methods */
    }

    // TODO
    return OSString::withCString("todo");
}