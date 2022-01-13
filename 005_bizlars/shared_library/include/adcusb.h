/**
******************************************************************************
* File       : adcusb.h
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcusb class: impement the interface usb
******************************************************************************
*/
#pragma once
#include <string>
#include <list>
#include "public.h"
#include "adcinterface.h"
#include "ringbuffer.h"
using namespace std;

class AdcUsb : public AdcInterface
{
#ifndef USE_LIBUSB
    typedef list <string>			DeviceList;			// device list as string
#else
	typedef list <libusb_device*>	DeviceList;			// device list with libusb
#endif


public:
	AdcUsb();
	~AdcUsb();

	bool            Open(string &adcName);
	bool			Open();
    bool            Close();
	bool			Reconnect();
    bool            IOControl(unsigned long ulControlCommand,
                              void *pInputData, unsigned long inputSize,
                              void *pOutputData, unsigned long outputSize,
                              unsigned long *pBytesReturn);
    unsigned long   Write(void *pData, unsigned long size);
    unsigned long   Read(void *pData, unsigned long size);
	bool			ResetInterface();
	bool			DriverVersion(unsigned short *major, unsigned short *minor);
	void			GetPort(string &port);
    
    TYbizUsbAdcInfo m_usbInfo;

private:
	static const short	TIMEOUT_RECONNECT_ATTEMPTS = 3;

#ifndef USE_LIBUSB
    void			GetUsbInfo();
#else
	void GetUsbInfo(libusb_device *dev);
	void ConvertLibUsbErrorCodeToString(char* errorMessage, int errorCode);
#endif	// USE_LIBUSB
	unsigned long   ReadFromADC(void *pData, unsigned long size);
	bool GetAllUsbAdcDeviceNames();

    DeviceList  	m_usbDeviceList;
	string			m_usbIdentifier;
	RingBuffer<char>	*m_ringBuffer;
	char			*m_readCache;

#if defined __GNUC__ && defined USE_LIBUSB
	void GetUsbEndpoints(libusb_device *dev);

	libusb_context 			*m_context;
	libusb_device 			**m_devs;
	libusb_device_handle	*m_libusbHandle;

	unsigned char			m_bulkInEndpointAddr;
	unsigned char			m_bulkOutEndpointAddr;
#endif
};

