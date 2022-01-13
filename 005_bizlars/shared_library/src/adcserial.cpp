/**
******************************************************************************
* File       : adcusb.cpp
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcusb class: impement the interface usb
******************************************************************************
*/
#ifdef  _MSC_VER
#include <windows.h>
#include <setupapi.h>
#include <AtlBase.h>
#include <AtlConv.h>
#endif
#ifdef  __GNUC__
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include "public.h"
#include "adctrace.h"
#include "adcserial.h"


AdcSerial::AdcSerial() : AdcInterface()
{
	string port = "//./COM1";
	Init(port);
}


AdcSerial::AdcSerial(const string &port) : AdcInterface()
{
	string tmpPort = "//./" + port;
	Init(tmpPort);
}

AdcSerial::~AdcSerial()
{
	m_InterfaceType = INTERFACE_SERIAL;
	// Defaultport COM1
	m_port = "//./COM1";

	m_useCRC16 = true;
}


void AdcSerial::Init(const string &port)
{
	m_InterfaceType = INTERFACE_SERIAL;
	// Defaultport COM1
	m_port = port;

	m_useCRC16 = true;
}

#ifdef _MSC_VER

/**
******************************************************************************
* Open - Open the connection the Bizerba weighing system
*
* @param    adcName     name of the device
*
* @return
* @remarks
******************************************************************************
*/
bool AdcSerial::Open(string &adcName)
{
	m_adcName = adcName;

	return Open();
}

/**
******************************************************************************
* Open - Open the connection the Bizerba weighing system
*
* @return
* @remarks
******************************************************************************
*/
bool AdcSerial::Open()
{
	return Open(115200, 8, ONESTOPBIT, NOPARITY);
}

/**
******************************************************************************
* Open - Open the connection the Bizerba weighing system
*
* @return
* @remarks
******************************************************************************
*/
bool AdcSerial::Open(unsigned long baudrate, unsigned char bytesize, unsigned char stopbit, unsigned char parity)
{
	bool	try2Open = false;

    // first close the device before trying again to open it
    Close();

	if ((m_hDevice = CreateFileA(m_port.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,               /* no overlapped I/O             */
		NULL)) == INVALID_HANDLE_VALUE)           /* must be NULL for comm devices */
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcan't open adc %s (error: %d)", __FUNCTION__, m_port.c_str(), GetLastError());
		m_hDevice = 0;
	}
	

	//Check if seccessful
    if (m_hDevice == 0)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\topen device failed", __FUNCTION__);
        return false;
    }
    else
    {
		DCB dcb;      /* device control block */
		if (GetCommState(m_hDevice, &dcb))
		{
			/* configure the port SW-Handshake */
			dcb.BaudRate = baudrate;
			dcb.ByteSize = bytesize;
			dcb.Parity = parity;
			dcb.StopBits = stopbit;
			dcb.fDtrControl = DTR_CONTROL_DISABLE;
			dcb.fRtsControl = RTS_CONTROL_DISABLE;
			dcb.fDsrSensitivity = false;
			dcb.fOutxCtsFlow = false;
			dcb.fOutxDsrFlow = false;
			dcb.fOutX = FALSE;		// sw handshake
			dcb.fInX = FALSE;		// sw handshake

			if (!SetCommState(m_hDevice, &dcb))
				g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcan't write device control block", __FUNCTION__);
		}
		else
		{
			g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcan't read device control block", __FUNCTION__);
		}


		COMMTIMEOUTS timeouts;		/* timeout control block */
		if (GetCommTimeouts(m_hDevice, &timeouts))
		{
			timeouts.ReadIntervalTimeout = 0;
			timeouts.ReadTotalTimeoutMultiplier = 2;		/* timeout between 2 characters */
			timeouts.ReadTotalTimeoutConstant = 200;		/* additional timeout */
			timeouts.WriteTotalTimeoutMultiplier = 2;		/* timeout between 2 characters */
			timeouts.WriteTotalTimeoutConstant = 200;		/* additional timeout */

			if (!SetCommTimeouts(m_hDevice, &timeouts))
				g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcan't write timeout control block", __FUNCTION__);
		}
		else
		{
			// Workaround for Intel-Driver HSUART because api GetCommTimeouts delivers an error, SetCommTimeouts works
			g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcan't read timeout control block, try to write it", __FUNCTION__);

			timeouts.ReadIntervalTimeout = 0;
			timeouts.ReadTotalTimeoutMultiplier = 2;		/* timeout between 2 characters */
			timeouts.ReadTotalTimeoutConstant = 200;		/* additional timeout */
			timeouts.WriteTotalTimeoutMultiplier = 2;		/* timeout between 2 characters */
			timeouts.WriteTotalTimeoutConstant = 200;		/* additional timeout */

			if (!SetCommTimeouts(m_hDevice, &timeouts))
				g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcan't write timeout control block", __FUNCTION__);
		}



        return true;
    }
}



/**
******************************************************************************
* Close - Close the connection the Bizerba weighing system
*
* @param    
*
* @return       false   error close device
*               true    close device ok
* @remarks
******************************************************************************
*/

bool AdcSerial::Close()
{
    if (m_hDevice != 0)
    {
        if (!CloseHandle(m_hDevice))
        {
            g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tclose handle failed (error %d)", __FUNCTION__, GetLastError());
            return false;
        }
        m_hDevice = 0;
    }
    return true;
}

#endif  // _MSC_VER


#ifdef __GNUC__
/**
******************************************************************************
* Open - Open the connection the Bizerba weighing system
*
* @param    adcName     name of the device
*
* @return
* @remarks
******************************************************************************
*/
bool AdcSerial::Open(string &adcName)
{
	m_adcName = adcName;

	return Open();
}


/**
******************************************************************************
* Open - Open the connection the Bizerba weighing system
*
* @return
* @remarks
******************************************************************************
*/
bool AdcSerial::Open()
{
	return Open(115200, 8, 1, 0);
}

/**
******************************************************************************
* Open - Open the connection the Bizerba weighing system
*
* @return
* @remarks
******************************************************************************
*/
bool AdcSerial::Open(unsigned long baudrate, unsigned char bytesize, unsigned char stopbit, unsigned char parity)
{
	return false;
}


/**
******************************************************************************
* Close - Close the connection the Bizerba weighing system
*
* @param
*
* @return       false   error close device
*               true    close device ok
* @remarks
******************************************************************************
*/

bool AdcSerial::Close()
{
    return true;
}

#endif  // __GNUC__


/**
******************************************************************************
* Write - put data to adc
*
* @param    pData  : pointer of data buffer
* @param    size   : size of data buffer

* @return   0:	    error, see log
*			!= 0:   number of data bytes actually sends
* @remarks
******************************************************************************
*/
unsigned long AdcSerial::Write(void *pData, unsigned long size)
{
    unsigned long numWr = 0;

#ifdef  _MSC_VER
    if (WriteFile(m_hDevice, pData, size, &numWr, NULL) == 0)
    {
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror write %d", __FUNCTION__, GetLastError());
        numWr = 0;
    }
#endif  // _MSC_VER

#ifdef __GNUC__
    ssize_t bytesWritten;
    bytesWritten = write(m_hDevice, pData, size);
    if (bytesWritten == -1)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror write %d", __FUNCTION__, errno);
        numWr = 0;
    }
    else
    {
    	numWr = bytesWritten;
    }
#endif

    return numWr;
}


/**
******************************************************************************
* Read - get data from printer
*
* @param    lpHdl  : printer handle
* @param    pData  : pointer of data buffer
* @param    size   : size of data buffer
*
* @return   0:	    error, see log
*			!= 0:   number of data bytes actually receives
* @remarks
******************************************************************************
*/
unsigned long AdcSerial::Read(void *pData, unsigned long size)
{
    unsigned long numRd = 0;

#ifdef  _MSC_VER
    if (ReadFile(m_hDevice, pData, size, &numRd, NULL) == 0)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror read %d", __FUNCTION__, GetLastError());
        numRd = 0;
    }
#endif  // _MSC_VER

#ifdef __GNUC__
    ssize_t bytesRead;
    bytesRead = read(m_hDevice, pData, size);
    if (bytesRead == -1)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror read %d", __FUNCTION__, errno);
        numRd = 0;
    }
    else
    {
    	numRd = bytesRead;
    }
#endif

    return numRd;
}


/**
******************************************************************************
* Reconnect - try to reconnect to device
*
* @return   false:	    reconnect false
*			true:		reconnect successful
* @remarks
******************************************************************************
*/
bool AdcSerial::Reconnect()
{
	return true;
}


/**
******************************************************************************
* GetPort - get port
*
* @return   void
* @remarks
******************************************************************************
*/
void AdcSerial::GetPort(string &port)
{
	port = m_port;
}
