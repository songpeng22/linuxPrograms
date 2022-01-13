/**
******************************************************************************
* File       : adcinterface.h
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcinterface class (virtual)
******************************************************************************
*/
#pragma once
#include <string>

using namespace std;

// Device handle 
#ifdef  _MSC_VER 
typedef void*           adcHandle;
#endif

#ifdef __GNUC__
typedef long   					adcHandle;
#endif

class AdcInterface
{
public:
    static const short  INTERFACE_USB = 0;
    static const short  INTERFACE_SERIAL = 1;
    
    AdcInterface();
    virtual                 ~AdcInterface();
    
	virtual bool            Open(string &adcName) = 0;
	virtual bool			Open() = 0;
    virtual bool            Close() = 0;
	virtual bool			Reconnect() = 0;
    virtual short           GetInterfaceType();
    virtual unsigned long   Write(void *pData, unsigned long size) = 0;
    virtual unsigned long   Read(void *pData, unsigned long size) = 0;
	virtual bool			UseCRC16();
	virtual bool			ResetInterface();
	virtual bool			DriverVersion(unsigned short *major, unsigned short  *minor);
	virtual void			GetPort(string &port);
    
protected:
    adcHandle   m_hDevice;
    short       m_InterfaceType;
	string		m_adcName;
	bool		m_useCRC16;
};

