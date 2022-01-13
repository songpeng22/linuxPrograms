/**
******************************************************************************
* File       : adcfirmware.h
* Project    : BizLars
* Date       : 03.05.2017
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcfirmware class
******************************************************************************
*/
#pragma once
#include <string>
#include <list>
#include "adcprotocol.h"
using namespace std;

class AdcFirmware
{
	typedef enum
	{
		PARSE_ALL = 0,          // parse complete SRecord file
		PARSE_S0RECORD,			// parse only S0 record
	}ParseSetting;

public:
	AdcFirmware();
    ~AdcFirmware();

	short LoadFile(const string &directory, const string &type);
	bool  GetData(unsigned long pos, AdcFrmData *frmData);
	short GetVersion(const string &directory, const string &type, string &version);
	string GetUsbStackVersion();
	string GetWelmecStructureVersion();

private:
	short ParseSRecord(string &content, list<pair<unsigned long, string>> *contentList, ParseSetting parse);
	void ParseHeader(string &header);
	bool CheckSumOK(string content, unsigned long checksum);
	void ConvertHexAscii2String(string &header);
		
	unsigned long m_minAddress;
	unsigned long m_maxAddress;

	unsigned char *m_data;

	string m_adcType;
	string m_firmwareFileVersion;
	string m_usbStackVersion;
	string m_welmecStructureVersion;

	static const string FIRMWARE_FILE;
};

