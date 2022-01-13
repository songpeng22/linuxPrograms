/**
******************************************************************************
* File       : adcfirmware.cpp
* Project    : BizLars
* Date       : 03.05.2017
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcfirmware class 
******************************************************************************
*/
#include <hex.h>
using CryptoPP::HexEncoder;
using CryptoPP::HexDecoder;

#include "larsErr.h"
#include "crypto.h"
#include "adctrace.h"
#include "adcfirmware.h"

const string AdcFirmware::FIRMWARE_FILE = "baf";

AdcFirmware::AdcFirmware()
{
	m_minAddress = 0xFFFFFFFF;
	m_maxAddress = 0;
	m_data = 0;

	m_adcType.clear();
	m_firmwareFileVersion.clear();
	m_usbStackVersion.clear();
	m_welmecStructureVersion.clear();
}

AdcFirmware::~AdcFirmware()
{
	if (m_data)
	{
		delete[] m_data;
		m_data = 0;
	}
}


/**
******************************************************************************
* LoadFile - load firmware file
*
* @param    directory:in	directory for the firmware files
* @param    type:in			adc type
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcFirmware::LoadFile(const string &directory, const string &type)
{
	short		errorCode;
	string		fileContent;
	string		firmwareFile;
	Crypto		cryptVar;
	list<pair<unsigned long, string>> contentList;
	HexDecoder	hexDecoder;
	unsigned long	pos;
	unsigned long	len;
	unsigned char	dataRecord[256];
	
	m_minAddress = 0xFFFFFFFF;
	m_maxAddress = 0;
	if (m_data)
	{
		delete[] m_data;
		m_data = 0;
	}

	m_adcType.clear();
	m_firmwareFileVersion.clear();
	m_usbStackVersion.clear();
	m_welmecStructureVersion.clear();

	if (directory.empty())
		firmwareFile = type + "." + FIRMWARE_FILE;
	else
		firmwareFile = directory + "/" + type + "." + FIRMWARE_FILE;

	// read and decrypt country settings file
	errorCode = cryptVar.ReadEncryptedFile(firmwareFile, fileContent);
	if (errorCode != LarsErr::E_SUCCESS)
	{
		return errorCode;
	}

	// parse firmware file
	errorCode = ParseSRecord(fileContent, &contentList, PARSE_ALL);
	if (errorCode != LarsErr::E_SUCCESS)
	{
		contentList.clear();
		return errorCode;
	}

	// compare adc types
	if (type != m_adcType)
	{
		return LarsErr::E_FILE_CORRUPT;
	}

	// copy file content to internal buffer
	if ((contentList.size() != 0) && (m_maxAddress >= m_minAddress))
	{
		// allocate memory for firmware and copy data from contentMap
		m_data = new unsigned char[m_maxAddress - m_minAddress];
		if (m_data != 0)
		{
			memset(m_data, 0xFF, m_maxAddress - m_minAddress);

			for (list<pair<unsigned long, string>>::iterator it = contentList.begin(); it != contentList.end(); it++)
			{
				// calculate index for m_data
				pos = (*it).first - m_minAddress;

				hexDecoder.Put((byte *)((*it).second.data()), (*it).second.length());
				len = hexDecoder.Get(dataRecord, sizeof(dataRecord));

				memcpy(m_data + pos, dataRecord, len);
			}
		}
		else
		{
			errorCode = LarsErr::E_NOT_ENOUGH_MEMORY;
		}
	}
	else
	{
		errorCode = LarsErr::E_FILE_CORRUPT;
	}

	return errorCode;
}


/**
******************************************************************************
* ParseSRecord - load firmware file
*
* @param    fileContent:in		s file content
* @param    contentList:out		list ofa ll s3 records
* @param    firmwareHeader:out	s0 record content
* @param    parse:in			attribute parse all or only s0 record 
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcFirmware::ParseSRecord(string &fileContent, list<pair<unsigned long, string>> *contentList, ParseSetting parse)
{
	short			errorCode = LarsErr::E_SUCCESS;
	unsigned long	pos;
	unsigned long	lenSRecord, lenData;
	unsigned long	address, minAddress, maxAddress;
	string			content;
	unsigned long	checksum;
	bool			S0Received;

	try
	{
		minAddress = 0xFFFFFFFF;
		maxAddress = 0;
		S0Received = false;
		if (contentList) contentList->clear();

		for (unsigned long idx = 0; idx < fileContent.length(); idx++)
		{
			if (fileContent[idx] == 'S')
			{
				switch (fileContent[idx + 1])
				{
				case '0':
					idx += 2;
					pos = idx;
					// get length of s-record
					lenSRecord = std::stoul(fileContent.substr(idx, 2), 0, 16);
					lenData = lenSRecord - 3;
					idx += 2;
					// get address of s-record
					address = std::stoul(fileContent.substr(idx, 4), 0, 16);
					idx += 4;
					// get data of s-record
					content = fileContent.substr(idx, lenData * 2);
					idx += lenData * 2;
					// get checksum of s-record
					checksum = std::stoul(fileContent.substr(idx, 2), 0, 16);
					idx += 2;

					if (CheckSumOK(fileContent.substr(pos, lenSRecord * 2), checksum))
					{
						ConvertHexAscii2String(content);
						ParseHeader(content);

						minAddress = 0xFFFFFFFF;
						maxAddress = 0;

						S0Received = true;
					}
					else
					{
						throw std::invalid_argument("s-record checksum error");
					}

					S0Received = true;
					break;

				case '3':
					idx += 2;
					pos = idx;
					// get length of s-record
					lenSRecord = std::stoul(fileContent.substr(idx, 2), 0, 16);
					lenData = lenSRecord - 5;
					idx += 2;
					// get address of s-record
					address = std::stoul(fileContent.substr(idx, 8), 0, 16);
					idx += 8;
					// get data of s-record
					content = fileContent.substr(idx, lenData * 2);
					idx += lenData * 2;
					// get checksum of s-record
					checksum = std::stoul(fileContent.substr(idx, 2), 0, 16);
					idx += 2;

					if (CheckSumOK(fileContent.substr(pos, lenSRecord * 2), checksum))
					{
						if (contentList) contentList->push_back(std::pair<unsigned long, string>(address, content));

						if (minAddress >= address) minAddress = address;
						if (maxAddress <= address + (content.length() / 2)) maxAddress = address + (content.length() / 2);
					}
					else
					{
						throw std::invalid_argument("s-record checksum error");
					}
					break;

				case '7':
					idx += 2;
					// get length of s-record
					lenSRecord = std::stoul(fileContent.substr(idx, 2), 0, 16);
					idx += 2;
					lenData = lenSRecord - 5;
					// get address of s-record
					address = std::stoul(fileContent.substr(idx, 4), 0, 16);
					idx += 4;
					// get data of s-record
					idx += lenData * 2;
					// get checksum of s-record
					idx += 2;

					m_minAddress = minAddress;
					m_maxAddress = maxAddress;
					break;
				case '8':
					idx += 2;
					// get length of s-record
					lenSRecord = std::stoul(fileContent.substr(idx, 2), 0, 16);
					idx += 2;
					lenData = lenSRecord - 4;
					// get address of s-record
					address = std::stoul(fileContent.substr(idx, 3), 0, 16);
					idx += 3;
					// get data of s-record
					idx += lenData * 2;
					// get checksum of s-record
					idx += 2;

					m_minAddress = minAddress;
					m_maxAddress = maxAddress;
					break;
				case '9':
					idx += 2;
					// get length of s-record
					lenSRecord = std::stoul(fileContent.substr(idx, 2), 0, 16);
					idx += 2;
					lenData = lenSRecord - 3;
					// get address of s-record
					address = std::stoul(fileContent.substr(idx, 2), 0, 16);
					idx += 2;
					// get data of s-record
					idx += lenData * 2;
					// get checksum of s-record
					idx += 2;

					m_minAddress = minAddress;
					m_maxAddress = maxAddress;
					break;
				}
			}

			if ((parse == PARSE_S0RECORD) && (S0Received == true))
				break;
		}
	}
	catch (std::exception &e)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\texception: %s", __FUNCTION__, e.what());
		errorCode = LarsErr::E_FILE_CORRUPT;
	}

	return errorCode;
}


/**
******************************************************************************
* ChecksumOK - check if S-Record checksum is ok
*
* @param    len:in		data length
* @param    address:in			adc type
*
* @return   errorCode
* @remarks
******************************************************************************
*/
bool AdcFirmware::CheckSumOK(string content, unsigned long checksum)
{
	unsigned long chk = 0;

	try
	{
		for (unsigned long idx = 0; idx < content.length();)
		{
			chk += std::stoul(content.substr(idx, 2), 0, 16);
			idx += 2;
		}

		// make ones complement 
		chk = ~chk & 0xFF;

		return (chk == checksum ? true: false);
	}
	catch (std::exception &e)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\texception: %s", __FUNCTION__, e.what());
		return false;
	}
}


/**
******************************************************************************
* GetData - get one block of the firmware data
*
* @param    pos:in			position in the firmware data
* @param    frmData:out		pointer to struct of firmware data
*
* @return   errorCode
* @remarks
******************************************************************************
*/
bool AdcFirmware::GetData(unsigned long pos, AdcFrmData *frmData)
{
	unsigned long size;

	if ((m_minAddress + pos + 256) <= m_maxAddress)
	{
		size = 256;
	}
	else
	{
		size = m_maxAddress - (m_minAddress + pos);
	}

	frmData->len = (short)size;
	frmData->startAdr = m_minAddress + pos;

	if (size)
	{
		memcpy(frmData->data, &m_data[pos], size);

		return true;
	}
	else
	{
		return false;
	}
}


/**
******************************************************************************
* GetVersion - get the version of the firmware file
*
* @param    directory:in	directory for the firmware files
* @param    type:in			adc type
* @param    version:in		firmware file version
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcFirmware::GetVersion(const string &directory, const string &type, string &version)
{
	short	errorCode;
	string	firmwareFile;
	string	fileContent;
	Crypto	cryptVar;

	version.clear();

	if (directory.empty())
		firmwareFile = type + "." + FIRMWARE_FILE;
	else
		firmwareFile = directory + "/" + type + "." + FIRMWARE_FILE;

	// read and decrypt country settings file
	errorCode = cryptVar.ReadEncryptedFile(firmwareFile, fileContent);
	if (errorCode != LarsErr::E_SUCCESS)
	{
		return errorCode;
	}

	// parse firmware file
	errorCode = ParseSRecord(fileContent, NULL, PARSE_S0RECORD);
	if (errorCode != LarsErr::E_SUCCESS)
	{
		return errorCode;
	}

	// compare adc types
	if (type != m_adcType)
	{
		return LarsErr::E_FILE_CORRUPT;
	}

	if (!m_firmwareFileVersion.empty())
	{
		version = m_firmwareFileVersion;
	}
	else
	{
		errorCode = LarsErr::E_FILE_CORRUPT;
	}

	return errorCode;
}

/**
******************************************************************************
* ParseHeader - parse the S0 record header
*
* @return   errorCode
* @remarks
******************************************************************************
*/
void AdcFirmware::ParseHeader(string &header)
{
	size_t	pos1;
	size_t	pos2;

	// parse for adc type
	pos1 = header.find('_');
	if (pos1 != string::npos)
	{
		m_adcType = header.substr(0, pos1);
	}
	else
	{
		m_adcType.clear();
		m_firmwareFileVersion.clear();
		m_usbStackVersion.clear();
		m_welmecStructureVersion.clear();
		return;
	}


	// parse for file version
	if ((pos2 = header.find('_', pos1 + 1)) == string::npos)
	{
		pos2 = header.length();
	}
	if (pos1 != string::npos)
	{
		m_firmwareFileVersion = header.substr(pos1 + 1, pos2 - (pos1 + 1));
	}
	else
	{
		m_firmwareFileVersion.clear();
		m_usbStackVersion.clear();
		m_welmecStructureVersion.clear();
		return;
	}

	// parse for usb stack version
	pos1 = pos2;
	if ((pos2 = header.find('_', pos1 + 1)) == string::npos)
	{
		pos2 = header.length();
	}
	if (pos1 != string::npos)
	{
		m_usbStackVersion = header.substr(pos1 + 1, pos2 - (pos1 + 1));
	}
	else
	{
		m_usbStackVersion.clear();
		m_welmecStructureVersion.clear();
		return;
	}


	// parse for welmec structure version
	pos1 = pos2;
	if ((pos2 = header.find('_', pos1 + 1)) == string::npos)
	{
		pos2 = header.length();
	}
	if (pos1 != string::npos)
	{
		m_welmecStructureVersion = header.substr(pos1 + 1, pos2 - (pos1 + 1));
	}
	else
	{
		m_welmecStructureVersion.clear();
		return;
	}

}


/**
******************************************************************************
* GetUsbStackVersion - get the usb stack version
*
* @return   string
* @remarks
******************************************************************************
*/
string AdcFirmware::GetUsbStackVersion()
{
	return m_usbStackVersion;
}


/**
******************************************************************************
* GetWelmecStructureVersion - get the welmec structure version
*
* @return   string
* @remarks
******************************************************************************
*/
string AdcFirmware::GetWelmecStructureVersion()
{
	return m_welmecStructureVersion;
}


void AdcFirmware::ConvertHexAscii2String(string &header)
{
	HexDecoder	hexDecoder;
	unsigned long	len;
	unsigned char	dataRecord[256];

	// convert hex ascii to ascii
	hexDecoder.Put((byte *)header.c_str(), header.length());
	len = hexDecoder.Get(dataRecord, sizeof(dataRecord) - 1);
	// add terminating 0
	dataRecord[len] = '\0';
	header = (char *)dataRecord;
}
