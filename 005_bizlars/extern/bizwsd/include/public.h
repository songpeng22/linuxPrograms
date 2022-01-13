/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    BulkUsr.h

Abstract:

Environment:

    User & Kernel mode

--*/

#ifndef _USER_H
#define _USER_H

#ifdef _MSC_VER
#include <initguid.h>

// {515412DF-A981-48FF-8C53-3317BD30C58C}
DEFINE_GUID(GUID_CLASS_BIZWSD_USB,
0x515412df, 0xa981, 0x48ff, 0x8c, 0x53, 0x33, 0x17, 0xbd, 0x30, 0xc5, 0x8c);

#define IOCTL_INDEX             0x0000


#define IOCTL_BIZWSD_GET_CONFIG_DESCRIPTOR CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     IOCTL_INDEX,     \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)
                                                   
#define IOCTL_BIZWSD_RESET_DEVICE          CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     IOCTL_INDEX + 1, \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)

#define IOCTL_BIZWSD_RESET_PIPE            CTL_CODE(FILE_DEVICE_UNKNOWN,     \
                                                     IOCTL_INDEX + 2, \
                                                     METHOD_BUFFERED,         \
                                                     FILE_ANY_ACCESS)
// 18.08.2015 buckt
#define IOCTL_BIZWSD_ADC_INFO          CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                     IOCTL_INDEX + 50,      \
                                                     METHOD_BUFFERED,       \
                                                     FILE_ANY_ACCESS)

#define IOCTL_BIZWSD_ADC_VERSION       CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                                                     IOCTL_INDEX + 51,      \
                                                     METHOD_BUFFERED,       \
                                                     FILE_ANY_ACCESS)
// 18.08.2015 buckt
#endif

#ifdef __GNUC__
// 18.08.2015 buckt
#define IOCTL_BIZWSD_ADC_INFO 			51
#define IOCTL_BIZWSD_GET_DEVPATH		52
#define IOCTL_BIZWSD_ADC_VERSION		53
// 18.08.2015 buckt
#endif

// 18.08.2015 buckt
// USB defines for Bizerba weighing system driver
#define BIZERBA_VENDOR_ID               0x11B2
#define BIZERBA_WSD_ADC505              0x3201

/* Bizerba USB-ADC information structure */
typedef struct
{
	unsigned short idVendor;
	unsigned short idProduct;
} TYbizUsbAdcInfo;

typedef struct
{
	unsigned short major;
	unsigned short minor;
} TYbizDriverVers;

#ifdef __GNUC__
/* structure for ioctl under linux */
typedef struct
{
	void *pInputData;
	unsigned long inputSize;
	void *pOutputData;
	unsigned long outputSize;
	unsigned long bytesReturn;
} TYbizIoctl;
#endif
// 18.08.2015 buckt

#endif
