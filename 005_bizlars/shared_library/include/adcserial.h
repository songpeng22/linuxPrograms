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

using namespace std;

class AdcSerial : public AdcInterface
{
    typedef list <string>   DeviceList;

public:
	AdcSerial();
	AdcSerial(const string &port);
	~AdcSerial();

	bool            Open(string &adcName);
	bool			Open();
	bool			Open(unsigned long baudrate, unsigned char bytesize, unsigned char stopbit, unsigned char parity);
    bool            Close();
	bool			Reconnect();
   
    unsigned long   Write(void *pData, unsigned long size);
    unsigned long   Read(void *pData, unsigned long size);

	void			GetPort(string &port);
    
private:
    void 			Init(const string &port);

	string m_port;

};

