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

#include "usbdevs.h"

/**
 * The default USB vendor/product identifiers used for CP210x devices.
 */
const pl_usb_device_id coop_plausible_driver_CP210x_default_ids[] = {
    /* CP210x USB to UART bridge */
    {
        .vendor = 4292,
        .product = 60000
    },
    
    /* CP210x USB to UART Bridge (Enhanced/Standard Port) */
    {
        .vendor = 4292,
        .product = 60016
    },
    
    /* CP210x USB to UART Bridge (USBXpress VCP) */
    {
        .vendor = 4292,
        .product = 60001
    },
};

/**
 * The count of elements in coop_plausible_driver_CP210x_default_ids.
 */
const size_t coop_plausible_driver_CP210x_default_ids_count = sizeof(coop_plausible_driver_CP210x_default_ids) / sizeof(coop_plausible_driver_CP210x_default_ids[0]);