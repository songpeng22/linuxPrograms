/**
******************************************************************************
* File       : adcssp.h
* Project    : BizLars
* Date       : 19.05.2017
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcssp class
******************************************************************************
*/
#pragma once
#include <string>
#include <map>
#include "adctilt.h"
using namespace std;

class AdcSsp
{
	typedef map<string, string> KeyValueMap;

public:
	AdcSsp();
	~AdcSsp();

	short LoadFile(const string *directory, const string *adcType, const string *wsType, const string *scaleModel);
	void Clear(void);
	void SetTilt(AdcTilt *tilt);

	const KeyValueMap* GetTccSettings();
	const KeyValueMap* GetLinSettings();
	const KeyValueMap* GetWdtaSettings(long multiInterval, long resolution, double maxCapacityKG);
	const KeyValueMap* GetGeneralSettingsSealed();
	
private:
	void	Parse(string &content, short part);
	void	RemoveCR(string &str);
	void	CalcWdtaSettings(string &wdtaSettings, double maxCapacityKG);

	AdcTilt *m_tilt;

	KeyValueMap	m_tccSettings;
	KeyValueMap m_linSettings;
	KeyValueMap m_scaleSettings;
	KeyValueMap m_wdtaSettings;
	KeyValueMap m_wdtaSettings4Extern;
	KeyValueMap m_generalSettingsSealed;

	static const string		SPIRIT_LEVEL_FLOAT;
	static const string		SPIRIT_LEVEL_FLOAT_TC;
	static const string		SPIRIT_LEVEL_ZEROING_RANGE_FLOAT;

	static const short		UNSUPPORTED = 0;
	static const short		SCALE = 1;
	static const string		SCALE_STRING;
	static const short		TCC_SETTINGS = 2;
	static const string		TCC_SETTINGS_STRING;
	static const short		LIN_SETTINGS = 3;
	static const string		LIN_SETTINGS_STRING;
	static const short		WDTA_SETTINGS = 4;
	static const string		WDTA_SETTINGS_STRING;
	static const short		GENERAL_SETTINGS_SEALED = 5;
	static const string		GENERAL_SETTINGS_SEALED_STRING;

	static const short		WDTA_STRINGS_COUNT = 4;
	static const string		WDTA_STRINGS[];
	
	static const short		PARSE_ONLY_HEADER = 0;
	static const short		PARSE_COMPLETE = 1;
};
