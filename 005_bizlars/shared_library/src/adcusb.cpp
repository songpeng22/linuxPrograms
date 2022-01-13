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
#include <thread>         // std::this_thread::sleep_for

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

#ifdef USE_LIBUSB
//#include <libusb-1.0/libusb.h>
#include <libusb/libusb.h>
#endif	// USE_LIBUSB

#endif	// __GNUC__
#include "public.h"
#include "larsErr.h"
#include "adctrace.h"
#include "adcusb.h"
#include "ringbuffer.h"

// size of the ringbuffer, commands can not be longer than the ringbuffer size
#define	SIZE_RINGBUFFER	2048
// read always data packages with size 64 Bytes, because the adc send usb packets with 64 bytes
#define	SIZE_READ_USB_DATA_PACKAGE	64

#ifdef USE_LIBUSB
#define IOCTL_BIZWSD_RESET_DEVICE	1
#define MAX_SIZE_OF_LIBUSB_ERROR_MESSAGE 50
#endif

AdcUsb::AdcUsb() : AdcInterface()
{
	// create ring buffer
	m_ringBuffer = new RingBuffer<char>(SIZE_RINGBUFFER);
	m_ringBuffer->Clear();

	// create read cache
	m_readCache = new char[SIZE_READ_USB_DATA_PACKAGE];
	
	m_usbDeviceList.clear();
	m_usbIdentifier.clear();

    m_InterfaceType = INTERFACE_USB;
    memset((void *)&m_usbInfo, 0, sizeof(TYbizUsbAdcInfo));

#ifdef __GNUC__

#ifndef USE_LIBUSB
    // Device nodes paths
    m_usbDeviceList.push_back("/dev/bizwsd0");
#else
	int retCode;
	char errorMessage[MAX_SIZE_OF_LIBUSB_ERROR_MESSAGE];

	m_context = NULL;
	m_libusbHandle = NULL;
	m_devs = NULL;
	m_bulkInEndpointAddr = 0;
	m_bulkInEndpointAddr = 0;
	if ((retCode = libusb_init(&m_context)) < LIBUSB_SUCCESS)
	{
		ConvertLibUsbErrorCodeToString(errorMessage, retCode);
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tlibusb_init failed (error: %s)", __FUNCTION__, errorMessage);
	}
#endif

#endif
}

AdcUsb::~AdcUsb()
{
	delete m_ringBuffer;
	delete[] m_readCache;

#if defined __GNUC__ && defined USE_LIBUSB
	libusb_exit(m_context);
#endif

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
bool AdcUsb::Open(string &adcName)
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
bool AdcUsb::Open()
{
	bool	try2Open = false;
    string  usbDeviceName;

    // first close the device before trying again to open it
    Close();

	if (!GetAllUsbAdcDeviceNames())
	{
		return false;
	}

    DeviceList::const_iterator it;
	for (it = m_usbDeviceList.begin(); it != m_usbDeviceList.end(); ++it)
	{
        usbDeviceName = *it;

		try2Open = true;

		// if adc was already open, search for the entry
		if (!m_usbIdentifier.empty())
		{
			try2Open = false;

            if (usbDeviceName.find(m_usbIdentifier) != string::npos)
			{
				try2Open = true;
			}
		}

		if (try2Open)
		{
            wstring stemp = wstring(usbDeviceName.begin(), usbDeviceName.end());
            if ((m_hDevice = CreateFile(stemp.c_str(),
									   GENERIC_READ | GENERIC_WRITE,
									   0,
									   NULL,
									   OPEN_EXISTING,
				                       FILE_ATTRIBUTE_NORMAL,
				                       NULL)) == INVALID_HANDLE_VALUE)
			{
                g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcannot open adc %s (error: %d)", __FUNCTION__, usbDeviceName.c_str(), GetLastError());
                m_hDevice = 0;
				if (m_usbIdentifier.empty()) continue;
				else break;
			}
			else
			{
				// device is successfully open, save usb identifier without vendor and product id
                size_t pos = usbDeviceName.find_last_of("#");
                pos = usbDeviceName.find_last_of("#", pos - 1);
                m_usbIdentifier = usbDeviceName.substr(pos + 1, usbDeviceName.length() - pos - 1);

                // get usb Info
                GetUsbInfo();
				break;
			}
		}
	}


	//Check if seccessful
    if (m_hDevice == 0)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\topen device failed", __FUNCTION__);
        return false;
    }
    else
    {
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

bool AdcUsb::Close()
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


/**
******************************************************************************
* GetAllUsbAdcDeviceNames - Given a ptr to a driver-registered GUID, retrieve all device names from
registered device that can be used in a CreateFile() call.
* @param
*
* @return		0:	error, see log
*				1:	ok
* @remarks
******************************************************************************
*/
bool AdcUsb::GetAllUsbAdcDeviceNames()
{
	PSP_DEVICE_INTERFACE_DETAIL_DATA    functionClassDeviceData = NULL;
	ULONG                               predictedLength = 0;
	ULONG                               requiredLength = 0;
	HDEVINFO                            hardwareDeviceInfo;
	SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
	ULONG                               index;
	bool								ret = false;

	//
	// Open a handle to the plug and play dev node.
	// SetupDiGetClassDevs() returns a device information set that contains 
	// info on all installed devices of a specified class.
	//
	hardwareDeviceInfo = SetupDiGetClassDevs((LPGUID)&GUID_CLASS_BIZWSD_USB,
		NULL, // Define no enumerator (global)
		NULL, // Define no
		(DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));  // Only Devices present
	// Function class devices.
	if (hardwareDeviceInfo == INVALID_HANDLE_VALUE)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tinvalid hardware device handle", __FUNCTION__);

		ret = false;

		return ret;
	}

	// Enumerate through all devices in set
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	index = 0;
	m_usbDeviceList.clear();
	while (1)
	{
		if (SetupDiEnumDeviceInterfaces(hardwareDeviceInfo, 0, (LPGUID)&GUID_CLASS_BIZWSD_USB, index, &deviceInterfaceData))
		{
			//
			// allocate a function class device data structure to receive the
			// goods about this particular device.
			//
			SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo,
				&deviceInterfaceData,
				NULL,					// probing so no output buffer yet
				0,						// probing so output buffer length of zero
				&requiredLength,
				NULL);					// not interested in the specific dev-node


			predictedLength = requiredLength;
			// sizeof (SP_FNCLASS_DEVICE_DATA) + 512;
			if ((functionClassDeviceData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(predictedLength)))
			{
				functionClassDeviceData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

				//
				// Retrieve the information from Plug and Play.
				//
				if (SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo,
					&deviceInterfaceData,
					functionClassDeviceData,
					predictedLength,
					&requiredLength,
					NULL))
				{
					/* add device to list */
					//string *tmpString = new string((CW2A(functionClassDeviceData->DevicePath)));
					string tmpString = (CW2A((functionClassDeviceData->DevicePath)));
					m_usbDeviceList.push_back(tmpString);
					g_adcTrace.Trace(AdcTrace::TRC_INFO, "%s\tindex %d add adc %s", __FUNCTION__, index, tmpString.c_str());

					ret = true;
				}
				else
				{
					g_adcTrace.Trace(AdcTrace::TRC_INFO, "%s\tindex %d get hardware device interface detail failed (error: %d)", __FUNCTION__, index, GetLastError());

					ret = true;
				}

				free(functionClassDeviceData);
			}
		}
		else
		{
			g_adcTrace.Trace(AdcTrace::TRC_INFO, "%s\tindex %d enum device interface failed (error: %d)", __FUNCTION__, index, GetLastError());
			break;
		}
		index++;
	}

	// SetupDiDestroyDeviceInfoList() destroys a device information set
	// and frees all associated memory.
	SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);

	return ret;
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
bool AdcUsb::Open(string &adcName)
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
bool AdcUsb::Open()
{
#ifndef USE_LIBUSB
    string  usbDeviceName;
#else
    int	retCode;
#endif

    // first close the device before trying again to open it
    Close();

#ifdef USE_LIBUSB
    if (!GetAllUsbAdcDeviceNames())
	{
		return false;
	}
#endif

    DeviceList::const_iterator it;
	for (it = m_usbDeviceList.begin(); it != m_usbDeviceList.end(); ++it)
	{
#ifndef USE_LIBUSB
        usbDeviceName = *it;

		//Open the the device
        m_hDevice = open(usbDeviceName.c_str(), O_RDWR);
		//Check if seccessful
		if (m_hDevice == -1)
		{
			g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcannot open adc %s (error: %d)", __FUNCTION__, usbDeviceName.c_str(), errno);
			m_hDevice = 0;
			continue;
		}
		else
		{
			char 			usbIdentifier[40] = "";
			unsigned long 	outputSize = sizeof(usbIdentifier);
			unsigned long 	bytesReturn;

			// check usb identifier
			IOControl(IOCTL_BIZWSD_GET_DEVPATH,
				NULL, 0,
				usbIdentifier, outputSize,
				&bytesReturn);

			if (!bytesReturn)
			{
				g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tget usb identifier", __FUNCTION__);
				m_usbIdentifier.clear();
			}
			else
			{
				if (!m_usbIdentifier.empty())
				{
					if (m_usbIdentifier.find(usbIdentifier) == string::npos)
					{
						// wrong device, close device
						Close();
						continue;
					}
				}
				else
				{
					m_usbIdentifier = usbIdentifier;
				}
			}

			// get usb Info
			GetUsbInfo();
			break;
		}
#else
		char usbIdentifier[40] = "";
		char errorMessage[MAX_SIZE_OF_LIBUSB_ERROR_MESSAGE];

		GetUsbInfo(*it);
		retCode = libusb_open(*it, &m_libusbHandle);
		//Check if seccessful
		if (retCode < LIBUSB_SUCCESS)
		{
			ConvertLibUsbErrorCodeToString(errorMessage, retCode);
			g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcannot open adc Vendor-ID: 0x%04X Product-ID: 0x%04X (error: %s)", __FUNCTION__, m_usbInfo.idVendor, m_usbInfo.idProduct, errorMessage);
			m_hDevice = 0;
			continue;
		}
		else
		{
			// unref all devices and free the list of devices discovered by libusb_get_device_list
			libusb_free_device_list(m_devs, 1);

			GetUsbEndpoints(*it);

			int ifaceNr = 0;

			// check if kernel driver is active for this device
			if (libusb_kernel_driver_active(m_libusbHandle, ifaceNr) == 1)
			{
				// try to deteach kernel driver
				retCode = libusb_detach_kernel_driver(m_libusbHandle, ifaceNr);
				if (retCode != LIBUSB_SUCCESS)
				{
					ConvertLibUsbErrorCodeToString(errorMessage, retCode);
					g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror detach kernel driver %d (error: %s)", __FUNCTION__, ifaceNr, errorMessage);
				}
			}

			// claim first interface
			retCode = libusb_claim_interface(m_libusbHandle, ifaceNr);
			if (retCode != LIBUSB_SUCCESS)
			{
				ConvertLibUsbErrorCodeToString(errorMessage, retCode);
				g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror claim interface %d (error: %s)", __FUNCTION__, ifaceNr, errorMessage);
			}


			int usbBusNumber = libusb_get_bus_number(*it);
			if (sprintf(usbIdentifier, "%d", usbBusNumber) > 0)
			{
				if (!m_usbIdentifier.empty())
				{
					if (m_usbIdentifier.find(usbIdentifier) == string::npos)
					{
						// wrong device, close device
						Close();
						continue;
					}
				}
				else
				{
					m_usbIdentifier = usbIdentifier;
				}
			}

			m_hDevice = (adcHandle)m_libusbHandle;
			break;
		}
#endif
	}

	//Check if seccessful
    if (m_hDevice == 0)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\topen device failed", __FUNCTION__);
        return false;
    }
    else
    {
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

bool AdcUsb::Close()
{
    if (m_hDevice != 0)
    {
#ifndef USE_LIBUSB
        if (close(m_hDevice) == -1)
        {
            g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tclose handle failed (error %d)", __FUNCTION__, errno);
            return false;
        }
#else
        // release first interface
        int ifaceNr = 0;
        char errorMessage[MAX_SIZE_OF_LIBUSB_ERROR_MESSAGE];

        int retCode = libusb_release_interface(m_libusbHandle, ifaceNr);
		if (retCode != LIBUSB_SUCCESS)
		{
			ConvertLibUsbErrorCodeToString(errorMessage, retCode);
			g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror release interface %d (error: %s)", __FUNCTION__, ifaceNr, errorMessage);
		}

		libusb_close(m_libusbHandle);


		m_libusbHandle = 0;
#endif	// USE_LIBUSB

        m_hDevice = 0;
    }
    return true;
}

#ifdef USE_LIBUSB
/**
******************************************************************************
* GetAllUsbAdcDeviceNames - Given a ptr to a driver-registered GUID, retrieve all device names from
* registered device that can be used in a CreateFile() call.
* @param
*
* @return		0:	error, see log
*				1:	ok
* @remarks
******************************************************************************
*/
bool AdcUsb::GetAllUsbAdcDeviceNames()
{
	bool ret = false;
	size_t listOfConnectedUsbDevices;
	struct libusb_device_descriptor desc;

	m_usbDeviceList.clear();
	listOfConnectedUsbDevices = libusb_get_device_list(m_context,&m_devs);		// get a list of all usb devices

	if(listOfConnectedUsbDevices < LIBUSB_SUCCESS)
	{
		// if listOfConnectedUsbDevices < 0, this is the error code
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcannot enumerate over usb devices (error: %d)", __FUNCTION__, listOfConnectedUsbDevices);
	}
	else
	{
		for (size_t i = 0; i<listOfConnectedUsbDevices; i++)
		{
			libusb_get_device_descriptor(m_devs[i], &desc);
			if (desc.idVendor != BIZERBA_VENDOR_ID)
			{
				continue;
			}
			if (desc.idProduct == BIZERBA_WSD_ADC505)
			{
				// found valid device, add to list
				m_usbDeviceList.push_back(m_devs[i]);
			}
		}

		ret = true;
	}
	return ret;
}


/**
******************************************************************************
* GetUsbEndpoints - Get the usb endpoints from the device
*
* @param		dev			active usb device
*
* @return		void
*
* @remarks
******************************************************************************
*/
void AdcUsb::GetUsbEndpoints(libusb_device *dev)
{
	int retCode;
	struct libusb_config_descriptor *config;
	const struct libusb_interface *iface;
	const struct libusb_interface_descriptor *altsetting;
	struct libusb_endpoint_descriptor *epDescriptor;
	int idxInterfaces, idxAltSetting;

	retCode = libusb_get_active_config_descriptor(dev, &config);
	if (retCode == 0)
	{
		for(idxInterfaces = 0; idxInterfaces < config->bNumInterfaces; idxInterfaces++)
		{
			iface = &config->interface[idxInterfaces];

			for(idxAltSetting = 0; idxAltSetting < iface->num_altsetting; idxAltSetting++)
			{
				altsetting = &iface->altsetting[idxAltSetting];

				int idxEndpoint;
				for(idxEndpoint = 0; idxEndpoint < altsetting->bNumEndpoints; idxEndpoint++)
				{
					epDescriptor = (struct libusb_endpoint_descriptor *)&altsetting->endpoint[idxEndpoint];

					if ((epDescriptor->bDescriptorType == LIBUSB_DT_ENDPOINT) &&
						((epDescriptor->bmAttributes & LIBUSB_TRANSFER_TYPE_BULK) == LIBUSB_TRANSFER_TYPE_BULK))
					{
						if (epDescriptor->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)
						{
							// store endpoint
							m_bulkInEndpointAddr = epDescriptor->bEndpointAddress;
						}
						else
						{
							// store endpoint
							m_bulkOutEndpointAddr = epDescriptor->bEndpointAddress;
						}
					}
				}
			}
		}

		libusb_free_config_descriptor(config);
	}
	else
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcannot get config descriptor (error: %d)", __FUNCTION__, retCode);
	}
	return;
}
#endif	// USE_LIBUSB
#endif  // _MSC_VER


#ifndef USE_LIBUSB
/**
******************************************************************************
* SetUsbInfo - Get the usb vendor and product ID
* @param
*
* @return
* @remarks
******************************************************************************
*/
void AdcUsb::GetUsbInfo()
{
    unsigned long outputSize;
    unsigned long bytesReturn = 0;

    outputSize = sizeof(TYbizUsbAdcInfo);

    IOControl(IOCTL_BIZWSD_ADC_INFO, 
              NULL, 0,
              &m_usbInfo, outputSize,
              &bytesReturn);

    if (bytesReturn == 0)
        memset((void *)&m_usbInfo, 0, sizeof(TYbizUsbAdcInfo));
}

#else

/**
******************************************************************************
* SetUsbInfo - Get the usb vendor and product ID
* @param
*
* @return
* @remarks
******************************************************************************
*/
void AdcUsb::GetUsbInfo(libusb_device *dev)
{
	int retCode;
    struct libusb_device_descriptor desc;

    retCode = libusb_get_device_descriptor(dev, &desc);
    if (retCode == LIBUSB_SUCCESS)
    {
    	m_usbInfo.idVendor = desc.idVendor;
    	m_usbInfo.idProduct = desc.idProduct;
    }
    else
    {
    	g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcannot get device descriptor (error: %d)", __FUNCTION__, retCode);
    	memset((void *)&m_usbInfo, 0, sizeof(TYbizUsbAdcInfo));
    }
}
#endif


/**
******************************************************************************
* IOControl - read/write data over iocontrol
*
* @param    ulControlCommand : control command for driver
* @param    pInputData  : pointer of input data buffer
* @param    inputSize   : size of input data buffer
* @param    pOutputData : pointer of output data buffer
* @param    pOutputSize : pointer to size of output data buffer
* @param    pBytesReturn : pointer to variable bytes return

* @return   0:	error, see log
*			1:	ok
* @remarks
******************************************************************************
*/
bool AdcUsb::IOControl(unsigned long ulControlCommand,
                       void *pInputData, unsigned long inputSize,
                       void *pOutputData, unsigned long outputSize,
                       unsigned long *pBytesReturn)
{
    bool retCode = false;

	if (m_hDevice == 0)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tdevice not open", __FUNCTION__);
		return retCode;
	}

#if defined _MSC_VER
    unsigned long nbytes = 0;
    DWORD error;
    if (DeviceIoControl(m_hDevice, ulControlCommand,
                        pInputData, inputSize,
                        pOutputData, outputSize,
                        &nbytes, NULL) == 0)
    {
        if (((error = GetLastError()) == ERROR_SEM_TIMEOUT) &&
            (ulControlCommand != IOCTL_BIZWSD_RESET_PIPE) &&
            (ulControlCommand != IOCTL_BIZWSD_RESET_DEVICE))
        {
            // try to reset pipe/device
            DeviceIoControl(m_hDevice, IOCTL_BIZWSD_RESET_PIPE, NULL, 0, NULL, 0, NULL, NULL);
        }

        if (pBytesReturn) *pBytesReturn = 0;
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror DeviceIoControl %d", __FUNCTION__, error);
        retCode = false;
    }
    else
    {
        if (pBytesReturn) *pBytesReturn = nbytes;
        retCode = true;
    }
#endif  // _MSC_VER

#ifdef __GNUC__

#ifndef USE_LIBUSB
    TYbizIoctl linuxIoctl;

    linuxIoctl.pInputData = pInputData;
    linuxIoctl.inputSize = inputSize;
    linuxIoctl.pOutputData = pOutputData;
    linuxIoctl.outputSize = outputSize;
    linuxIoctl.bytesReturn = 0;

    if (ioctl(m_hDevice, ulControlCommand, (void *)&linuxIoctl) < 0)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror DeviceIoControl %d", __FUNCTION__, errno);
        if (pBytesReturn) *pBytesReturn = 0;
        retCode = false;
    }
    else
    {
        if (pBytesReturn) *pBytesReturn = linuxIoctl.bytesReturn;
        retCode = true;
    }
#else
    int ret;
    char errorMessage[MAX_SIZE_OF_LIBUSB_ERROR_MESSAGE];

    if (ulControlCommand == IOCTL_BIZWSD_RESET_DEVICE)
    {
    	ret = libusb_reset_device(m_libusbHandle);
    	if (ret != LIBUSB_SUCCESS)
    	{
    		ConvertLibUsbErrorCodeToString(errorMessage, ret);
            g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror reset device (error: %s)", __FUNCTION__, errorMessage);
            retCode = false;
    	}
    	else
    	{
    		retCode = true;
    	}
    }
#endif	// USE_LIBUSB

#endif	// __GNUC__

    return retCode;
}



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
unsigned long AdcUsb::Write(void *pData, unsigned long size)
{
    unsigned long numWr = 0;

	if (m_hDevice == 0)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tdevice not open", __FUNCTION__);
		return 0;
	}

#ifdef  _MSC_VER
    DWORD error;
    if (WriteFile(m_hDevice, pData, size, &numWr, NULL) == 0)
    {
        if ((error = GetLastError()) == ERROR_SEM_TIMEOUT)
        {
            // try to reset pipe/device
            IOControl(IOCTL_BIZWSD_RESET_PIPE, NULL, 0, NULL, 0, NULL);
        }
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror write %d", __FUNCTION__, error);
        numWr = 0;
    }
#endif  // _MSC_VER

#ifdef __GNUC__
    int bytesWritten;

#ifndef USE_LIBUSB
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
#else
	// write over libusb, timeout = 1000 ms
	int retCode;
	char errorMessage[MAX_SIZE_OF_LIBUSB_ERROR_MESSAGE];

	retCode = libusb_bulk_transfer(m_libusbHandle, m_bulkOutEndpointAddr, (unsigned char*)pData, size, &bytesWritten, 1000);
	if (retCode != LIBUSB_SUCCESS)
	{
		ConvertLibUsbErrorCodeToString(errorMessage, retCode);
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror write bulk transfer (error: %s)", __FUNCTION__, errorMessage);
		numWr = 0;
	}
	else
	{
		numWr = bytesWritten;
	}
#endif	// USE_LIBUSB

#endif

    return numWr;
}


/**
******************************************************************************
* Read - get data from adc
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
unsigned long AdcUsb::ReadFromADC(void *pData, unsigned long size)
{
    unsigned long numRd = 0;

	if (m_hDevice == 0)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tdevice not open", __FUNCTION__);
		return 0;
	}

#ifdef  _MSC_VER
    if (ReadFile(m_hDevice, pData, size, &numRd, NULL) == 0)
    {
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror read %d", __FUNCTION__, GetLastError());
        numRd = 0;
    }
#endif  // _MSC_VER

#ifdef __GNUC__
    int bytesRead = 0;;

#ifndef USE_LIBUSB
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
#else
	// read over libusb, timeout = 1000 ms
	int retCode;
	char errorMessage[MAX_SIZE_OF_LIBUSB_ERROR_MESSAGE];

	retCode = libusb_bulk_transfer(m_libusbHandle, m_bulkInEndpointAddr, (unsigned char*)pData, size, &bytesRead, 1000);
	if (retCode != LIBUSB_SUCCESS)
	{
		ConvertLibUsbErrorCodeToString(errorMessage, retCode);
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror read bulk transfer (error: %s)", __FUNCTION__, errorMessage);
		numRd = 0;
	}
	else
	{
		numRd = bytesRead;
	}
#endif	// USE_LIBUSB
#endif

    return numRd;
}


/**
******************************************************************************
* Read - get data from ring buffer
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
unsigned long AdcUsb::Read(void *pData, unsigned long size)
{
	unsigned long numRdAdc = 0;

	while (m_ringBuffer->GetLevel() < size)
	{
		numRdAdc = ReadFromADC(m_readCache, SIZE_READ_USB_DATA_PACKAGE);
		if (numRdAdc > 0)
		{
			// store data to ring buffer
			m_ringBuffer->SetData(m_readCache, numRdAdc);
		}
		else
		{
			g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tadc sends no data", __FUNCTION__);
			return 0;
		}
	}

	// read data from ring buffer
	return m_ringBuffer->GetData((char *)pData, size);
}


/**
******************************************************************************
* ResetInterface - reset the usb interface - pipes
*
* @param    lpHdl  : printer handle
*
* @return   false:	error, see log
*			true:	ok
* @remarks
******************************************************************************
*/
bool AdcUsb::ResetInterface()
{
#ifdef  _MSC_VER
	// try to reset pipe/device
	return (IOControl(IOCTL_BIZWSD_RESET_PIPE, NULL, 0, NULL, 0, NULL));
#endif
#ifdef __GNUC__
	return true;
#endif	// __GNUC__
}


/**
******************************************************************************
* DriverVersion - get driver version
*
* @param    major  : major version
* @param    minor  : minor version
*
* @return   false	error
*			true    ok
* @remarks
******************************************************************************
*/
bool AdcUsb::DriverVersion(unsigned short *major, unsigned short *minor)
{
	bool ret = true;
#ifndef USE_LIBUSB
	bool retCode;
	int retry = 0;
	unsigned long outputSize;
	unsigned long bytesReturn;
	TYbizDriverVers	driverVer;

	outputSize = sizeof(TYbizDriverVers);

	driverVer.major = 0;
	driverVer.minor = 0;

	do
	{
		bytesReturn = 0;
		retCode = IOControl(IOCTL_BIZWSD_ADC_VERSION, NULL, 0, &driverVer, outputSize, &bytesReturn);

		if (retCode == 0)
		{
			Reconnect();
			retry++;
		}
	} while ((retCode == 0) && (retry < 2));

	if (bytesReturn == 0)
	{
		ret = false;
		if (major) *major = 0;
		if (minor) *minor = 0;
	}
	else
	{
		if (major) *major = driverVer.major;
		if (minor) *minor = driverVer.minor;
	}
#else
	// we use libusb, so we have no driver version
	ret = false;
#endif
	return ret;
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
bool AdcUsb::Reconnect()
{
	int		reconnectAttempts = 0;
	short	reconnectErrorCode = LarsErr::E_SUCCESS;

	g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tno connection to adc, try to reconnect", __FUNCTION__);

	do
	{
		// connection error to adc, reopen connection and try again
		Close();

		if (!Open())
		{
			reconnectErrorCode = LarsErr::E_NO_DEVICE;
			if (reconnectAttempts < TIMEOUT_RECONNECT_ATTEMPTS) std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	} while ((reconnectErrorCode == LarsErr::E_NO_DEVICE) && (++reconnectAttempts < TIMEOUT_RECONNECT_ATTEMPTS));

	if (reconnectErrorCode == LarsErr::E_NO_DEVICE)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\treconnection failed, no device found -> abort", __FUNCTION__);
		return false;
	}
	else
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\treconnect to adc successful", __FUNCTION__);
		return true;
	}
}


/**
******************************************************************************
* GetPort - get port
*
* @return   void
* @remarks
******************************************************************************
*/
void AdcUsb::GetPort(string &port)
{
	port = m_usbIdentifier;
}


#ifdef USE_LIBUSB
void AdcUsb::ConvertLibUsbErrorCodeToString(char* errorMessage, int errorCode)
{
	switch(errorCode)
	{
		case LIBUSB_SUCCESS: strcpy(errorMessage, "Success"); break;
		case LIBUSB_ERROR_IO: strcpy(errorMessage, "Input/output error"); break;
		case LIBUSB_ERROR_INVALID_PARAM: strcpy(errorMessage, "Invalid parameter"); break;
		case LIBUSB_ERROR_ACCESS: strcpy(errorMessage, "Access denied"); break;
		case LIBUSB_ERROR_NO_DEVICE: strcpy(errorMessage, "No such device (it may have been disconnected)"); break;
		case LIBUSB_ERROR_NOT_FOUND: strcpy(errorMessage, "Entity not found"); break;
		case LIBUSB_ERROR_BUSY: strcpy(errorMessage, "Resource busy"); break;
		case LIBUSB_ERROR_TIMEOUT: strcpy(errorMessage, "Operation timed out"); break;
		case LIBUSB_ERROR_OVERFLOW: strcpy(errorMessage, "Overflow"); break;
		case LIBUSB_ERROR_PIPE: strcpy(errorMessage, "Pipe error"); break;
		case LIBUSB_ERROR_INTERRUPTED: strcpy(errorMessage, "System call interrupted(perhaps due to signal"); break;
		case LIBUSB_ERROR_NO_MEM: strcpy(errorMessage, "Insufficient memory"); break;
		case LIBUSB_ERROR_NOT_SUPPORTED: strcpy(errorMessage, "Operation not supported or unimplemented on this platform"); break;
		default:  strcpy(errorMessage, "Unknown error"); break;
	}
}
#endif
