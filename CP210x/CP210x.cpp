/*
 * Author: Landon Fuller <landonf@plausible.coop>
 *
 * Information on interfacing with the CP210x chipset was derived
 * from the FreeBSD uslcom(4) driver from FreeBSD 9.0. The uslcom(4)
 * driver was originally written by Johnathan Gray <jsg@openbsd.org>
 * for OpenBSD.
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
#include "CP210xConfig.h"
#include "RingBuffer.h"

#include "logging.h"

/* Default ring buffer size */
#define BUFFER_SIZE (PAGE_SIZE * 3)

// Define the superclass
#define super IOSerialDriverSync

OSDefineMetaClassAndStructors(coop_plausible_driver_CP210x, super);

/** Mask on all possible state values */
#define STATE_ALL (PD_RS232_S_MASK | PD_S_MASK)

/** Mask on all state values that may be exposed externally */
#define STATE_EXTERNAL (PD_S_MASK | (PD_RS232_S_MASK & ~PD_RS232_S_LOOP))


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

    /* Set port defaults. These will be set by the BSD termios intialization code path
     * on first open. */
    _baudRate = 0;
    _characterLength = 0;
    _txParity = PD_RS232_PARITY_DEFAULT;
    _rxParity = PD_RS232_PARITY_DEFAULT;
    _twoStopBits = false;
    _xonChar = '\0';
    _xoffChar = '\0';

    /* Fetch our USB provider */
    _provider = OSDynamicCast(IOUSBInterface, provider);
    if (_provider == NULL) {
        LOG_ERR("Received invalid provider");
        return false;
    }
    _provider->retain();

    /* Configure TX/RX buffers */
    _txBuffer = new coop_plausible_CP210x_RingBuffer();
    if (!_txBuffer->init(BUFFER_SIZE)) {
        LOG_ERR("Could not create TX buffer");
        return false;
    }
    
    _rxBuffer = new coop_plausible_CP210x_RingBuffer();
    if (!_rxBuffer->init(BUFFER_SIZE)) {
        LOG_ERR("Could not create RX buffer");
        return false;
    }

    /* Create our child serial stream */
    _serialDevice = new coop_plausible_CP210x_SerialDevice();
    if (!_serialDevice->init(this, _provider)) {
        LOG_ERR("Could not create serial stream");
        return false;
    }
    
    LOG_DEBUG("Driver started");

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
    
    if (_txBuffer != NULL) {
        _txBuffer->release();
        _txBuffer = NULL;
    }
    
    if (_rxBuffer != NULL) {
        _rxBuffer->release();
        _rxBuffer = NULL;
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
    LOG_DEBUG("Acquire Port");

    IOLockLock(_lock); {
        /* Verify that the port has not already been acquired, and if so, optionally
         * wait for it to be released. */
        while (_state & PD_S_ACQUIRED) {
            /* If blocking operation has not been requested, return immediately. */
            if (sleep == false) {
                IOLockUnlock(_lock);
                return kIOReturnExclusiveAccess;
            }

            /* Otherwise, wait for our requisite state */
            UInt32 reqState = 0;
            IOReturn rtn = watchState(&reqState, PD_S_ACQUIRED, refCon, true);
            if (rtn != kIOReturnSuccess) {
                IOLockUnlock(_lock);
                return rtn;
            }
        }

        /* Set initial port state */
        setState(PD_S_ACQUIRED, STATE_ALL, refCon, true);
    }
    IOLockUnlock(_lock);

    return kIOReturnSuccess;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::releasePort(void *refCon) {
    LOG_DEBUG("Release Port");

    IOLockLock(_lock); {
        /* Validate that the port is actually open */
        if ((_state & PD_S_ACQUIRED) == 0) {
            IOLockUnlock(_lock);
            return kIOReturnNotOpen;
        }
        
        /* Reset the state to closed */
        setState(0, STATE_ALL, refCon, true);
    }
    IOLockUnlock(_lock);

    return kIOReturnSuccess;
}

// from IOSerialDriverSync
UInt32 coop_plausible_driver_CP210x::getState(void *refCon) {
    LOG_DEBUG("Get State");

    IOLockLock(_lock);
    UInt32 res = _state & STATE_EXTERNAL;
    IOLockUnlock(_lock);

    return res;
}

/**
 * Directly the internal state.
 *
 * @param state The updated state to be set.
 * @param mask The mask to use when setting @a state
 * @param refCon Reference constant.
 * @param haveLock If true, the method will assume that _lock is held. If false, the lock will be acquired
 * automatically.
 */
IOReturn coop_plausible_driver_CP210x::setState (UInt32 state, UInt32 mask, void *refCon, bool haveLock) {
    LOG_DEBUG("setState(0x%x 0x%x, %p, %x)", state, mask, refCon, (uint32_t)haveLock);

    if (!haveLock)
        IOLockLock(_lock);

    /* If the port has neither been acquired, nor is it being acquired, then inform the caller that
     * the port is not open. */
    if ((_state & PD_S_ACQUIRED) == 0 && ((state & mask) & PD_S_ACQUIRED) == 0) {
        if (!haveLock)
            IOLockUnlock(_lock);
        return kIOReturnNotOpen;
    }
    
    // TODO - Forbid state modifications we can't support via setState().
#if 0
    if (state & STATE_SET_UNSUPPORTED) {
        if (!haveLock)
            IOLockUnlock(_lock);
        return kIOReturnBadArgument;
    }
#endif

    /* Compute the new state, as well as the state delta */
    UInt32 newState = (_state & ~mask) | (state & mask);
    UInt32 deltaState = newState ^ _state;

    /* Update the internal state */
    _state = newState;
    
    /* Check if any state in the delta is being observed in watchState, and if so,
     * trigger a wake on our condition lock. */
    if (deltaState & _watchState) {
        /* Reset the watchState -- it will be reinitialized by any woken threads that
         * are not satisfied by this updated state */
        _watchState = 0x0;
        
        /* Wake up all waiting threads. They will block on _lock until we release the
         * mutex (or, if haveLock is false, when our caller releases _lock). */
        IOLockWakeup(_lock, &_stateEvent, false);
    }

    if (!haveLock)
        IOLockUnlock(_lock);

    return kIOReturnSuccess;
}


// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::setState(UInt32 state, UInt32 mask, void *refCon) {
    return setState(state, mask, refCon, false);
}

/**
 * Block until the backing state matches @a state, masked by @a mask has changed.
 *
 * @param state The required state values to wait for.
 * @param mask The mask to use when evaluating @a state
 * @param refCon Reference constant.
 * @param haveLock If true, the method will assume that _lock is held. If false, the lock will be acquired
 * automatically.
 */
IOReturn coop_plausible_driver_CP210x::watchState (UInt32 *state, UInt32 mask, void *refCon, bool haveLock) {
    LOG_DEBUG("Watch State");

    if (!haveLock)
        IOLockLock(_lock);
    
    /* Validate that at least one state is being observed. These return values match Apple's USBCDCDMM driver
     * implementation, but it's unclear why a state of 0 returns kIOReturnBadARgument, while a mask of 0x0 is 
     * considered valid. XXX TODO: Review further, this behavior seems buggy, even if it does match Apple's
     * driver. */
    if (state == 0) {
        LOG_DEBUG("Watch request with 0 state");
        return kIOReturnBadArgument;
    }

    if (mask == 0) {
        LOG_DEBUG("Watch request with 0 mask");
        return kIOReturnSuccess;
    }
    
    /* Limit mask to EXTERNAL_MASK. There are no comments or documentation describing why this is
     * necessary, but this matches Apple's USBCDCDMM driver implementation. */
    mask &= STATE_EXTERNAL;

    /* There's nothing left to watch if the port has not been opened. */
    if ((_state & PD_S_ACQUIRED) == 0) {
        if (!haveLock)
            IOLockUnlock(_lock);
        return kIOReturnNotOpen;
    }
    
    UInt32 watchState = *state;
    
    /* To properly handle closure of the serial port, we must always track the PD_S_ACTIVE or PD_S_ACQUIRED state.
     * If the caller is not already doing so, we register our interest here. */
    bool autoActive = false;
    if ((mask & (PD_S_ACQUIRED | PD_S_ACTIVE)) == 0) {
        /* Watch for low on PD_S_ACTIVE bit */
        mask |= PD_S_ACTIVE;
        watchState &= ~PD_S_ACTIVE;

        /* Record that we enabled PD_S_ACTIVE monitoring */
        autoActive = true;
    }
    
    /* Loop (and block) until one of our watched state values is achieved */
    while (true) {
        /* Determine the currently matching states. We invert the current state mask with ~, and then use ^ to implement
         * XNOR. Truth table:
         *
         * X Y   O
         * 1 0 = 0
         * 0 0 = 1
         * 0 1 = 0
         * 1 1 = 1
         */
        UInt32 matched = (watchState ^ ~_state) & mask;
        if (matched != 0) {
            *state = _state & STATE_EXTERNAL;
            
            /* Ensure that we drop our lock before returning. No further access to internal
             * mutable state is required after this. */
            if (!haveLock)
                IOLockUnlock(_lock);

            /* If the port has been closed (and the caller was not tracking PD_S_ACTIVE),
             * return an error. Otherwise we're just informing the caller that PD_S_ACTIVE was set low,
             * closing the port. This must necessarily differ from success, as the caller's state changes
             * of interest have not been detected.
             */
            if (autoActive && (matched & PD_S_ACTIVE)) {
                return kIOReturnIOError;
            } else {
                return kIOReturnSuccess;
            }
        }

        /* Update the watched bits. This will reset the watchState on every loop -- it is reset to 0 when threads 
         * are signaled. */
        _watchState |= mask;
        
        /* Wait to be signaled on a state change */
        int rtn = IOLockSleep(_lock, &_stateEvent, THREAD_UNINT);
        if (rtn == THREAD_TIMED_OUT) {
            if (!haveLock)
                IOLockUnlock(_lock);
            return kIOReturnTimeout;
        } else if (rtn != THREAD_INTERRUPTED) {
            if (!haveLock)
                IOLockUnlock(_lock);
            return kIOReturnAborted;
        }

        // TODO - Should we be monitoring for termination of our driver here?
    }

    if (!haveLock)
        IOLockUnlock(_lock);

    /* Should not be reachable */
    LOG_ERR("Reached unreachable end of watchState()");
    return kIOReturnOffline;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::watchState(UInt32 *state, UInt32 mask, void *refCon) {
    return watchState(state, mask, refCon, false);
}

// from IOSerialDriverSync
UInt32 coop_plausible_driver_CP210x::nextEvent(void *refCon) {
    IOReturn ret;

    /* This implementation matches AppleUSBCDCDMM, which doesn't
     * provide any event queueing. */
    IOLockLock(_lock); {
        // TODO - check if stopping?

        if (_state & PD_S_ACTIVE) {
            ret = kIOReturnSuccess;
        } else {
            ret = kIOReturnNotOpen;
        }
    }
    IOLockUnlock(_lock);

    return ret;
}

/**
 * Write the stop bits, parity, and character length settings to the device.
 *
 * @param txParity The USLCOM_PARITY_* constant to be used to configure the device.
 * @param twoStopBits If true, set stop bits to 2. Otherwise 1.
 * @param charLength The character length. Must be a value >= 5, <= 8.
 */
IOReturn coop_plausible_driver_CP210x::writeCP210xDataConfig (uint32_t txParity, bool twoStopBits, uint32_t charLength) {
    LOG_DEBUG("Writing data config");
    uint16_t data = 0;

    /* Configure the bit field */
    if (twoStopBits) {
        data = USLCOM_STOP_BITS_2;
    } else {
        data = USLCOM_STOP_BITS_1;
    }
    
    if (txParity == PD_RS232_PARITY_ODD) {
        data |= USLCOM_PARITY_ODD;
    } else if (txParity == PD_RS232_PARITY_EVEN) {
        data |= USLCOM_PARITY_EVEN;
    } else {
        data |= USLCOM_PARITY_NONE;
    }

    if (charLength >= 5 && charLength <= 8) {
        data |= USLCOM_SET_DATA_BITS(charLength);
    } else {
        LOG_ERR("Incorrect character length value configured: %u", charLength);
    }
    
    /* Set up the UART request */
    IOUSBDevRequest req;
    req.bmRequestType = USLCOM_WRITE;
    req.bRequest = USLCOM_DATA;
    req.wValue = data;
    req.wIndex = USLCOM_PORT_NO;
    req.wLength = 0;
    
    /* Issue request */
    IOReturn ret = _provider->GetDevice()->DeviceRequest(&req, 5000, 0);
    if (ret != kIOReturnSuccess) {
        LOG_ERR("Set USLCOM_DATA failed: %u", ret);
    }
        
    return ret;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::executeEvent(UInt32 event, UInt32 data, void *refCon) {
    IOReturn ret = kIOReturnSuccess;
    UInt32 stateUpdate;
    UInt32 stateMask;
    
    IOLockLock(_lock);
    
    // TODO check if stopping?
    
    switch (event) {
        case PD_E_ACTIVE: {
            LOG_DEBUG("executeEvent(PD_E_ACTIVE, %u, %p)", data, refCon);

            /* Start or stop the UART */
            bool start = data;
            
            /* Skip if already started, already stopped */
            if (start && (_state & PD_S_ACTIVE) != 0) {
                break;
            } else if (!start && (_state & PD_S_ACTIVE) == 0) {
                break;
            }

            /* Set up the UART request */
            IOUSBDevRequest req;
            req.bmRequestType = USLCOM_WRITE;
            req.bRequest = USLCOM_UART;
            req.wIndex = USLCOM_PORT_NO;
            req.wLength = 0;

            stateMask = PD_S_ACTIVE;
            if (start) {
                LOG_DEBUG("Enabling UART");
                stateUpdate = PD_S_ACTIVE;
                req.wValue = USLCOM_UART_ENABLE;
            } else {
                LOG_DEBUG("Disabling UART");
                stateUpdate = 0;
                req.wValue = USLCOM_UART_DISABLE;
            }

            /* Issue request */
            ret = _provider->GetDevice()->DeviceRequest(&req, 5000, 0);
            if (ret != kIOReturnSuccess) {
                LOG_ERR("Set PD_E_ACTIVE (data=%u) failed: %u", data, ret);
                break;
            }

            /* Update state */
            setState(stateUpdate, stateMask, refCon, true);
            
            // TODO - Restore any line state?
            
            break;
        }


        case PD_E_RXQ_SIZE:
            /* Adjust receive queue size. We're not required to support this. */
            LOG_DEBUG("executeEvent(PD_E_RXQ_SIZE, %u, %p)", data, refCon);
            break;
        case PD_E_TXQ_SIZE:
            /* Adjust send queue size. We're not required to support this. */
            LOG_DEBUG("executeEvent(PD_E_TXQ_SIZE, %u, %p)", data, refCon);
            break;
            
        case PD_E_RXQ_HIGH_WATER:
            /* Optional */
            LOG_DEBUG("executeEvent(PD_E_RXQ_HIGH_WATER, %u, %p)", data, refCon);
            break;

        case PD_E_RXQ_LOW_WATER:
            /* Optional */
            LOG_DEBUG("executeEvent(PD_E_RXQ_HIGH_WATER, %u, %p)", data, refCon);
            break;

        case PD_E_TXQ_HIGH_WATER:
            /* Optional */
            LOG_DEBUG("executeEvent(PD_E_TXQ_HIGH_WATER, %u, %p)", data, refCon);
            break;

        case PD_E_TXQ_LOW_WATER:
            /* Optional */
            LOG_DEBUG("executeEvent(PD_E_TXQ_LOW_WATER, %u, %p)", data, refCon);
            break;
            
        case PD_E_TXQ_FLUSH:
            /* No-op. */
            LOG_DEBUG("executeEvent(PD_E_TXQ_FLUSH, %u, %p)", data, refCon);
            break;
            
        case PD_E_RXQ_FLUSH:
            /* No-op. */
            LOG_DEBUG("executeEvent(PD_E_RXQ_FLUSH, %u, %p)", data, refCon);
            break;
            
        case PD_E_DATA_RATE: {
            /* Set the baud rate. */
            LOG_DEBUG("executeEvent(PD_E_DATA_RATE, %u>>1, %p)", data, refCon);
            
            /*
             * IOSerialBSDClient shifts the speed << 1 before issuing a PD_E_DATA_RATE,
             * claiming that the speed is stored in half-bits, but this does not appear
             * to be the case. Comments in Apple's serial drivers' PD_E_DATA_RATE merely
             * state 'For API compatiblilty with Intel' before reversing the shift.
             *
             * Summary: This is necessary to keep IOSerialBSDClient happy, and why
             * IOSerialBSDClient requires this is lost to the history of whatever
             * Apple engineer is responsible.
             */
            UInt32 baud = data >> 1;
            
            /* Set up the UART request */
            IOUSBDevRequest req;
            req.bmRequestType = USLCOM_WRITE;
            req.bRequest = USLCOM_BAUD_RATE;
            req.wValue = USLCOM_BAUD_REF / baud;
            req.wIndex = USLCOM_PORT_NO;
            req.wLength = 0;
            
            /* Issue request */
            ret = _provider->GetDevice()->DeviceRequest(&req, 5000, 0);
            if (ret == kIOReturnSuccess) {
                _baudRate = baud;
            } else {
                LOG_ERR("Set USLCOM_BAUD_RATE failed: %u", ret);
                break;
            }

            break;
        }
            
        case PD_E_RX_DATA_RATE:
            /* We don't support setting an independent RX data rate to anything but 0. It's unclear
             * why we need to support a value of zero, but this matches Apple's USBCDCDMM implementation. */
            LOG_DEBUG("executeEvent(PD_E_RX_DATA_RATE, %u>>1, %p)", data, refCon);
            if (data != 0)
                ret = kIOReturnBadArgument;
            break;
            
        case PD_E_DATA_INTEGRITY:
            // Fall-through
        case PD_E_RX_DATA_INTEGRITY:
            if (event == PD_E_DATA_INTEGRITY) {
                LOG_DEBUG("executeEvent(PD_E_DATA_INTEGRITY, %u, %p)", data, refCon);
            } else {
                LOG_DEBUG("executeEvent(PD_E_DATA_INTEGRITY, %u, %p)", data, refCon);
            }

            switch (data) {
                case PD_RS232_PARITY_NONE:
                case PD_RS232_PARITY_ODD:
                case PD_RS232_PARITY_EVEN:
                    /* Set TX+RX vs. RX-only parity */
                    if (event == PD_E_DATA_INTEGRITY) {
                        /* Attempt to write the new configuration */
                        ret = writeCP210xDataConfig(data, _twoStopBits, _characterLength);
                        if (ret == kIOReturnSuccess) {
                            /* Update internal state on success */
                            _txParity = data;
                            _rxParity = PD_RS232_PARITY_DEFAULT;
                        }

                    } else {
                        _rxParity = data;
                    }

                    break;
                    
                default:
                    /* Unsupported parity setting */
                    ret = kIOReturnBadArgument;
                    break;
            }

            break;
            
        case PD_RS232_E_STOP_BITS:
            /* Set the stop bits */
            LOG_DEBUG("executeEvent(PD_RS232_E_STOP_BITS, %u>>1, %p)", data, refCon);

            /* Provided as half bits */
            data >>= 1;
            bool newTwoStopBits;

            if (data == 1) {
                newTwoStopBits = false;
            } else if (data == 2) {
                newTwoStopBits = true;
            } else {
                LOG_ERR("PD_RS232_E_STOP_BITS with invalid data=%u", data);
                ret = kIOReturnBadArgument;
            }

            /* Attempt to write the new configuration */
            ret = writeCP210xDataConfig(_txParity, newTwoStopBits, _characterLength);
            if (ret == kIOReturnSuccess) {
                _twoStopBits = newTwoStopBits;
            }

            break;
            
        case PD_RS232_E_RX_STOP_BITS:
            /* We don't support setting an independent RX stop bit value to anything but 0. It's unclear
             * why we need to support a value of zero, but this matches Apple's USBCDCDMM implementation. */
            LOG_DEBUG("executeEvent(PD_RS232_E_RX_STOP_BITS, %u>>1, %p)", data, refCon);
            if (data != 0)
                ret = kIOReturnBadArgument;
            break;
            
        case PD_E_DATA_SIZE: {
            /* Set the character bit size */
            LOG_DEBUG("executeEvent(PD_E_DATA_SIZE, %u>>1, %p)", data, refCon);

            /* Provided as half bits */
            data >>= 1;
            
            if (data < 5 || data > 8) {
                ret = kIOReturnBadArgument;
                break;
            }

            /* Attempt to write the new configuration */
            ret = writeCP210xDataConfig(_txParity, _twoStopBits, data);
            if (ret == kIOReturnSuccess) {
                _characterLength = data;
            }

            break;
        }
            
        case PD_E_RX_DATA_SIZE:
            /* We don't support setting an independent RX data size to anything but 0. It's unclear
             * why we need to support a value of zero, but this matches Apple's USBCDCDMM implementation. */
            LOG_DEBUG("executeEvent(PD_E_RX_DATA_SIZE, %u>>1, %p)", data, refCon);
            if (data != 0)
                ret = kIOReturnBadArgument;
            break;
            
        case PD_RS232_E_XON_BYTE:
            LOG_DEBUG("executeEvent(PD_RS232_E_XON_BYTE, %u, %p)", data, refCon);
            _xonChar = data;
            break;
            
        case PD_RS232_E_XOFF_BYTE:
            LOG_DEBUG("executeEvent(PD_RS232_E_XOFF_BYTE, %u, %p)", data, refCon);
            _xoffChar = data;
            break;

        default:
            LOG_DEBUG("Unsupported executeEvent(%x, %u, %p)", event, data, refCon);
            ret = kIOReturnBadArgument;
            break;
    }

    IOLockUnlock(_lock);
    return ret;
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::requestEvent(UInt32 event, UInt32 *data, void *refCon) {
    IOReturn ret = kIOReturnSuccess;
    
    // TODO check if stopping?

    if (data == NULL) {
        LOG_DEBUG("requestEvent() NULL data argument");
        return kIOReturnBadArgument;
    }

    IOLockLock(_lock);
    switch (event) {
        case PD_E_ACTIVE:
            /* Return active status */
            if (_state & PD_S_ACTIVE) {
                *data = true;
            } else {
                *data = false;
            }
            
            LOG_DEBUG("requestEvent(PD_E_ACTIVE, %u, %p)", *data, refCon);
            break;
            
        case PD_E_RXQ_SIZE:
            /* Return rx buffer size */
            *data = _rxBuffer->getCapacity();

            LOG_DEBUG("requestEvent(PD_E_RXQ_SIZE, %u, %p)", *data, refCon);
            break;

        case PD_E_TXQ_SIZE:            
            /* Return tx buffer size */
            *data = _txBuffer->getCapacity();
            
            LOG_DEBUG("requestEvent(PD_E_TXQ_SIZE, %u, %p)", *data, refCon);
            break;
            
        case PD_E_RXQ_HIGH_WATER:
            /* Unsupported. */
            *data = 0;
            
            LOG_DEBUG("requestEvent(PD_E_RXQ_HIGH_WATER, %u, %p)", *data, refCon);
            break;
            
        case PD_E_RXQ_LOW_WATER:
            /* Unsupported. */
            *data = 0;
            
            LOG_DEBUG("requestEvent(PD_E_RXQ_HIGH_WATER, %u, %p)", *data, refCon);
            break;
            
        case PD_E_TXQ_HIGH_WATER:
            /* Unsupported. */
            *data = 0;
            
            LOG_DEBUG("requestEvent(PD_E_TXQ_HIGH_WATER, %u, %p)", *data, refCon);
            break;
            
        case PD_E_TXQ_LOW_WATER:
            /* Unsupported. */
            *data = 0;

            LOG_DEBUG("requestEvent(PD_E_TXQ_LOW_WATER, %u, %p)", *data, refCon);
            break;
            
            
        case PD_E_TXQ_AVAILABLE:
            /* Return the free space in the TX buffer */
            *data = _txBuffer->getCapacity() - _txBuffer->getLength();
            
            LOG_DEBUG("requestEvent(PD_E_TXQ_AVAILABLE, %u, %p)", *data, refCon);
            break;
            
        case PD_E_RXQ_AVAILABLE:
            /* Return the number of bytes available in the RX buffer */
            *data = _rxBuffer->getLength();
            
            LOG_DEBUG("requestEvent(PD_E_RXQ_AVAILABLE, %u, %p)", *data, refCon);
            break;
            
        case PD_E_DATA_RATE:
            /*
             * IOSerialBSDClient shifts the speed >>1 after receiving the PD_E_DATA_RATE data,
             * claiming that the speed is stored in half-bits; this is not actually the case.
             *
             * Comments in Apple's serial drivers' PD_E_DATA_RATE merely
             * state 'For API compatiblilty with Intel' before reversing the shift.
             *
             * Summary: This is necessary to keep IOSerialBSDClient happy, and why
             * IOSerialBSDClient requires this is lost to the history of whatever
             * Apple engineer is responsible.
             */
            *data = _baudRate << 1;
            LOG_DEBUG("requestEvent(PD_E_DATA_RATE, %u>>1, %p)", *data, refCon);
            break;
            
        case PD_E_RX_DATA_RATE:
            /* We don't support setting an independent RX data rate to anything but 0. It's unclear
             * why we need to return a value of zero, but this matches Apple's USBCDCDMM implementation. */
            *data = 0x0;
            LOG_DEBUG("requestEvent(PD_E_RX_DATA_RATE, %u, %p)", *data, refCon);
            break;
            
        case PD_E_DATA_INTEGRITY:
            /* Return the tx parity value */
            *data = _txParity;
            LOG_DEBUG("requestEvent(PD_E_DATA_INTEGRITY, %u, %p)", *data, refCon);
            break;
            
        case PD_E_RX_DATA_INTEGRITY:
            /* Return the rx parity value */
            *data = _rxParity;
            LOG_DEBUG("requestEvent(PD_E_RX_DATA_INTEGRITY, %u, %p)", *data, refCon);
            break;
            
        case PD_RS232_E_STOP_BITS:
            /* Return the stop bit value (required to be half-bits). */
            if (_twoStopBits) {
                *data = 2 << 1;
            } else {
                *data = 1 << 1;
            }
            LOG_DEBUG("requestEvent(PD_RS232_E_STOP_BITS, %u>>1, %p)", *data, refCon);
            break;
            
        case PD_RS232_E_RX_STOP_BITS:
            /* We don't support setting an independent RX stop bit value to anything but 0. It's unclear
             * why we need to return a value of zero, but this matches Apple's USBCDCDMM implementation. */
            *data = 0x0;
            LOG_DEBUG("requestEvent(PD_RS232_E_RX_STOP_BITS, %u, %p)", *data, refCon);
            break;
            
        case PD_E_DATA_SIZE: {
            /* Return the character bit length (required to be half-bits). */
            *data = _characterLength << 1;
            LOG_DEBUG("requestEvent(PD_E_DATA_SIZE, %u>>1, %p)", *data, refCon);
            break;
        }
            
        case PD_E_RX_DATA_SIZE: {
            /* Return the RX-specific character bit length. We only support 0x0. */
            *data = 0x00;
            LOG_DEBUG("requestEvent(PD_E_RX_DATA_SIZE, %u, %p)", *data, refCon);
            break;
        }
            
        case PD_RS232_E_XON_BYTE:            
            *data = _xonChar;
            LOG_DEBUG("requestEvent(PD_RS232_E_XON_BYTE, %u, %p)", *data, refCon);
            break;

        case PD_RS232_E_XOFF_BYTE:
            *data = _xoffChar;
            LOG_DEBUG("requestEvent(PD_RS232_E_XOFF_BYTE, %u, %p)", *data, refCon);
            break;
            
        default:
            LOG_DEBUG("Unsupported requestEvent(%x, %p, %p)", event, data, refCon);
            ret = kIOReturnBadArgument;
            break;
    }

    IOLockUnlock(_lock);
    return ret;
}


// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::enqueueEvent(UInt32 event, UInt32 data, bool sleep, void *refCon) {
    return executeEvent(event, data, refCon);
}

// from IOSerialDriverSync
IOReturn coop_plausible_driver_CP210x::dequeueEvent(UInt32 *event, UInt32 *data, bool sleep, void *refCon) {
    /* Since we don't impelement event reporting from nextEvent(), this implementation is not required
     * to provide a queued event. We return success if the port is open and the arguments
     * are not invalid. */

    if (event == NULL || data == NULL)
        return kIOReturnBadArgument;

    if (getState(refCon) & PD_S_ACTIVE)
        return kIOReturnSuccess;

    return kIOReturnNotOpen;
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
