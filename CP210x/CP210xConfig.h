/*
 * Derived from FreeBSD 9.0's uslcom(4) driver.
 *
 * Copyright (c) 2006 Jonathan Gray <jsg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef CP210x_CP210xConfig_h
#define CP210x_CP210xConfig_h

#include <IOKit/usb/USB.h>

/* Configuration constants derived from FreeBSD's uslcom(4) */

#define	USLCOM_SET_DATA_BITS(x)	((x) << 8)

/* Request types */
#define	USLCOM_WRITE    USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBInterface)
#define	USLCOM_READ     USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBInterface)

/* Request codes */
#define	USLCOM_UART             0x00
#define	USLCOM_BAUD_RATE        0x01	
#define	USLCOM_DATA             0x03
#define	USLCOM_BREAK            0x05
#define	USLCOM_CTRL             0x07
#define	USLCOM_RCTRL            0x08
#define	USLCOM_SET_FLOWCTRL     0x13
#define	USLCOM_VENDOR_SPECIFIC  0xff

/* USLCOM_UART values */
#define	USLCOM_UART_DISABLE 0x00
#define	USLCOM_UART_ENABLE  0x01

/* USLCOM_CTRL/USLCOM_RCTRL values */
#define	USLCOM_CTRL_DTR_ON  0x0001	
#define	USLCOM_CTRL_DTR_SET 0x0100
#define	USLCOM_CTRL_RTS_ON  0x0002
#define	USLCOM_CTRL_RTS_SET 0x0200
#define	USLCOM_CTRL_CTS     0x0010
#define	USLCOM_CTRL_DSR     0x0020
#define	USLCOM_CTRL_RI      0x0040
#define	USLCOM_CTRL_DCD     0x0080

/* USLCOM_BAUD_RATE values */
#define	USLCOM_BAUD_REF     0x384000

/* USLCOM_DATA values */
#define	USLCOM_STOP_BITS_1  0x00
#define	USLCOM_STOP_BITS_2  0x02
#define	USLCOM_PARITY_NONE  0x00
#define	USLCOM_PARITY_ODD   0x10
#define	USLCOM_PARITY_EVEN  0x20

/* The default port number */
#define	USLCOM_PORT_NO      0x0000

/* USLCOM_BREAK configuration values */
#define	USLCOM_BREAK_OFF    0x00
#define	USLCOM_BREAK_ON     0x01

/* USLCOM_SET_FLOWCTRL values - 1st word */
#define	USLCOM_FLOW_DTR_ON      0x00000001 /* DTR static active */
#define	USLCOM_FLOW_CTS_HS      0x00000008 /* CTS handshake */
/* USLCOM_SET_FLOWCTRL values - 2nd word */
#define	USLCOM_FLOW_RTS_ON      0x00000040 /* RTS static active */
#define	USLCOM_FLOW_RTS_HS      0x00000080 /* RTS handshake */

/* USLCOM_VENDOR_SPECIFIC values */
#define	USLCOM_WRITE_LATCH  0x37E1
#define	USLCOM_READ_LATCH   0x00C2


#endif
