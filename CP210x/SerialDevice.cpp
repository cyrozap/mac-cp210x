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
#include "usbdevs.h"
#include "logging.h"

#include <IOKit/serial/IORS232SerialStreamSync.h>
#include <IOKit/serial/IOSerialKeys.h>

// Define the superclass
#define super OSObject

OSDefineMetaClassAndStructors(coop_plausible_CP210x_SerialDevice, super);

/**
 * Initialize a new IOSerialStreamSync serial device.
 *
 * @param provider The IOKit provider to which the device should be attached.
 * @param device The USB interface to which the newly initialized serial device will provide access. This is used
 * to fetch an appropriate device name for the serial device.
 */
bool coop_plausible_CP210x_SerialDevice::init (IOService *provider, IOUSBInterface *interface) {
    
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

    if ((result = _stream->attach(provider)) == false)
        goto finish;

    /*
     * Configure the device node name. We use a fixed base name, and a suffix
     * based on uniquely identifying data.
     */
    {
        OSString *suffix = this->getDeviceNameSuffix(interface);
        char dashSuffix[suffix->getLength() + 1 + 1]; // we include room for prepended '-' and terminating NULL
        snprintf(dashSuffix, sizeof(dashSuffix), "-%s", suffix->getCStringNoCopy());

        _stream->setProperty(kIOTTYBaseNameKey, "CP210x");
        _stream->setProperty(kIOTTYSuffixKey, dashSuffix);

        suffix->release();
    }

    /* Register our new IOKit service. */
    _stream->registerService();


    /* Fall-through on success */
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
 * @param interface The CP210x USB interface for which a suffix should be derived.
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
OSString *coop_plausible_CP210x_SerialDevice::getDeviceNameSuffix (IOUSBInterface *interface) {
    OSString *result = NULL;
    UInt8 idx;
    
    IOUSBDevice *device = interface->GetDevice();

    /* Determine whether the device is using the default vendor/product identifiers. If so, then there's a good
     * chance the EEPROM was not programed. We test for that below. */
    uint16_t prodId = device->GetProductID();
    uint16_t vendorId = device->GetVendorID();
    bool defaultEEPROMID = false;
    for (size_t i = 0; i < coop_plausible_driver_CP210x_default_ids_count; i++) {
        if (vendorId != coop_plausible_driver_CP210x_default_ids[i].vendor)
            continue;
        
        if (prodId != coop_plausible_driver_CP210x_default_ids[i].product)
            continue;

        /* Note that default EEPROM identifiers are in use */
        defaultEEPROMID = true;
        break;
    }

    /* First, we try to use the device serial number */
    if (result == NULL) {
        idx = device->GetSerialNumberStringIndex();
        if (idx != 0) {
            /* 256 is the maximum possible descriptor length */
            char serialStr[256];

            /* Fetch the serial */
            IOReturn ret = device->GetStringDescriptor(idx, serialStr, sizeof(serialStr));
            
            /* If fetch succeeded and returned a non-empty string ... */
            if (ret == kIOReturnSuccess && strnlen(serialStr, sizeof(serialStr)) > 0) {

                /* ... If the default EEPROM IDs were left in place, ensure that the serial number is not also
                 * set to the default value */
                if (!defaultEEPROMID || strcmp(serialStr, SILABS_DEFAULT_EEPROM_SERIAL) != 0)
                    result = OSString::withCString(serialStr);
            }
            
            /* Otherwise, fall through to the other methods */
        }
    }

    /* Next, try the location id. This is based on the USB toplogy, and should at least remain consistent
     * if the topology is not changed. */
    if (result == NULL) {
        OSNumber *location = OSDynamicCast(OSNumber, device->getProperty(kUSBDevicePropertyLocationID));
        if (location != NULL) {
            /* Convert to hex */
            char locStr[9];
            snprintf(locStr, sizeof(locStr), "%x", location->unsigned32BitValue());
            result = OSString::withCString(locStr);
        }
    }

    /* Bail out early on error, rather than trying to permute the NULL string below */
    if (result == NULL) {
        LOG_ERR("Failed to locate a valid serial number or USB location to use for device node naming");
        return OSString::withCString("unknown");
    }


    /* Ensure the result is unique in case there's more than one interface on the device. */
    {
        char uniqueStr[result->getLength() + 3 + 1]; // strlen, plus uint8_t interface number, plus terminating NULL
        snprintf(uniqueStr, sizeof(uniqueStr), "%s%x", result->getCStringNoCopy(), (int) interface->GetInterfaceNumber());
    
        result->release();
        result = OSString::withCString(uniqueStr);
    }

    return result;
}