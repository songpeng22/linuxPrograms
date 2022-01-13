/**
******************************************************************************
* File       : adctcc.cpp

* Project    : BizLars
* Date       : 19.05.2017
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adctcc class
******************************************************************************
*/
#include <sstream>
#include <iomanip>
#include "adcssp.h"
#include "larsErr.h"
#include "crypto.h"
#include "helpers.h"
#include "adctrace.h"

const string AdcSsp::SPIRIT_LEVEL_FLOAT = "spiritLevel|FLOAT";
const string AdcSsp::SPIRIT_LEVEL_FLOAT_TC = "spiritLevelTc|FLOAT";
const string AdcSsp::SPIRIT_LEVEL_ZEROING_RANGE_FLOAT = "spiritLevelZeroingRange|FLOAT";

const string AdcSsp::SCALE_STRING = "[scale]";
const string AdcSsp::TCC_SETTINGS_STRING = "[TCC settings]";
const string AdcSsp::LIN_SETTINGS_STRING = "[LIN settings]";
const string AdcSsp::WDTA_SETTINGS_STRING = "[weight dependent tilt angle]";
const string AdcSsp::GENERAL_SETTINGS_SEALED_STRING = "[general settings sealed]";

const string AdcSsp::WDTA_STRINGS[WDTA_STRINGS_COUNT] = { "x1|FLOAT",
														  "y1|FLOAT",
														  "x2|FLOAT",
														  "y2|FLOAT" };

AdcSsp::AdcSsp()
{
	Clear();
}


AdcSsp::~AdcSsp()
{
}

/**
******************************************************************************
* Clear - clear internal maps
*
* @return   void
* @remarks
******************************************************************************
*/
void AdcSsp::Clear()
{
	m_scaleSettings.clear();
	m_tccSettings.clear();
	m_linSettings.clear();
	m_wdtaSettings.clear();
	m_generalSettingsSealed.clear();
}


/**
******************************************************************************
* LoadFile - load ssp file
*
* @param    directory:in	directory for the ssp settings files
* @param    adcType:in		adc type
* @param    wsType:in		weighing cell type
* @param    scaleModel:in	scale model
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcSsp::LoadFile(const string *directory, const string *adcType, const string *wsType, const string *scaleModel)
{
	short		errorCode;
	Crypto		cryptVar;
	string		sspFile;
	string		fileContent;
	string		tmpAdcType, tmpWsType, tmpScaleModel;
	short		setFlag, actFlag;

	setFlag = 0;
	actFlag = 0;
	sspFile = "ssp";
	if (adcType && adcType->length())
	{
		setFlag |= 0x01;
		sspFile += "_" + *adcType;
	}
	if (wsType && wsType->length())
	{
		setFlag |= 0x02;
		sspFile += "_" + *wsType;
	}
	if (scaleModel && scaleModel->length())
	{
		setFlag |= 0x04;
		sspFile += "_" + *scaleModel;
	}
	sspFile += ".txt";

	if (!directory->empty())
		sspFile = *directory + "/" + sspFile;

	// read and decrypt country settings file
	errorCode = cryptVar.ReadEncryptedFile(sspFile, fileContent);
	if (errorCode != LarsErr::E_SUCCESS)
	{
		return errorCode;
	}

	// parse ssp file
	Parse(fileContent, PARSE_ONLY_HEADER);

	// check for key adc type
	tmpAdcType.clear();
	if (Helpers::KeyExists(m_scaleSettings, "adc", tmpAdcType))
	{
		actFlag |= 0x01;
	}

	// check for key ws type
	tmpWsType.clear();
	if (Helpers::KeyExists(m_scaleSettings, "ws", tmpWsType))
	{
		actFlag |= 0x02;
	}

	// ccheck for key scale model
	tmpScaleModel.clear();
	if (Helpers::KeyExists(m_scaleSettings, "model", tmpScaleModel))
	{
		actFlag |= 0x04;
	}

	if (setFlag != actFlag)
	{
		return LarsErr::E_FILE_CORRUPT;
	}

	// check for content
	if ((setFlag & 0x01) && (*adcType != tmpAdcType)) return LarsErr::E_FILE_CORRUPT;
	if ((setFlag & 0x02) && (*wsType != tmpWsType)) return LarsErr::E_FILE_CORRUPT;
	if ((setFlag & 0x04) && (*scaleModel != tmpScaleModel)) return LarsErr::E_FILE_CORRUPT;

	// parse ssp file
	Parse(fileContent, PARSE_COMPLETE);

	return errorCode;
}


/**
******************************************************************************
* Parse - parse ssp settings
*
* @param    content:in	ssp settings
* @param    type:in		parse only header or complete
*
* @return   errorCode
* @remarks
******************************************************************************
*/
void AdcSsp::Parse(string &content, short type)
{
	short   state = UNSUPPORTED;
	string  oneRow, key, value;
	size_t  startPos = 0, endPos = 0;
	size_t  pos1 = 0, posSeparator = 0;

	// remove all CR from string
	RemoveCR(content);

	while (startPos < content.length())
	{
		endPos = content.find('\n', startPos);
		if (endPos == string::npos) 
			endPos = content.length();
		oneRow = content.substr(startPos, endPos - startPos);

		if (!oneRow.empty())
		{
			if (oneRow == SCALE_STRING)
			{
				m_scaleSettings.clear();
				state = SCALE;
			}
			else if (oneRow == TCC_SETTINGS_STRING)
			{
				if (type == PARSE_ONLY_HEADER)
				{
					state = UNSUPPORTED;
				}
				else
				{
					state = TCC_SETTINGS;
				}
			}
			else if (oneRow == LIN_SETTINGS_STRING)
			{
				if (type == PARSE_ONLY_HEADER)
				{
					state = UNSUPPORTED;
				}
				else
				{
					state = LIN_SETTINGS;
				}
			}
			else if (oneRow == WDTA_SETTINGS_STRING)
			{
				if (type == PARSE_ONLY_HEADER)
				{
					state = UNSUPPORTED;
				}
				else
				{
					state = WDTA_SETTINGS;
				}
			}
			else if (oneRow == GENERAL_SETTINGS_SEALED_STRING)
			{
				if (type == PARSE_ONLY_HEADER)
				{
					state = UNSUPPORTED;
				}
				else
				{
					state = GENERAL_SETTINGS_SEALED;
				}
			}
			else
			{
				switch (state)
				{
				case SCALE:
					pos1 = oneRow.find('=');
					if (pos1 != string::npos)
					{
						key = oneRow.substr(0, pos1);
						value = oneRow.substr(pos1 + 1);
						Helpers::KeyAddOrReplace(m_scaleSettings, key, value);
					}
					else
						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror parsing ssp scale settings -> wrong format", __FUNCTION__);
					break;

				case TCC_SETTINGS:
					pos1 = oneRow.find('=');
					if (pos1 != string::npos)
					{
						key = oneRow.substr(0, pos1);
						value = oneRow.substr(pos1 + 1);
						Helpers::KeyAddOrReplace(m_tccSettings, key, value);
					}
					else
						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror parsing ssp tcc settings -> wrong format", __FUNCTION__);
					break;

				case LIN_SETTINGS:
					pos1 = oneRow.find('=');
					if (pos1 != string::npos)
					{
						key = oneRow.substr(0, pos1);
						value = oneRow.substr(pos1 + 1);
						Helpers::KeyAddOrReplace(m_linSettings, key, value);
					}
					else
						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror parsing ssp lin settings -> wrong format", __FUNCTION__);
					break;

				case WDTA_SETTINGS:
					pos1 = oneRow.find('=');
					if (pos1 != string::npos)
					{
						key = oneRow.substr(0, pos1);
						value = oneRow.substr(pos1 + 1);
						Helpers::KeyAddOrReplace(m_wdtaSettings, key, value);
					}
					else
						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror parsing ssp wdta settings -> wrong format", __FUNCTION__);
					break;

				case GENERAL_SETTINGS_SEALED:
					pos1 = oneRow.find('=');
					if (pos1 != string::npos)
					{
						key = oneRow.substr(0, pos1);
						value = oneRow.substr(pos1 + 1);

						// patch key spiritLevel
						if (key == SPIRIT_LEVEL_FLOAT || 
							key == SPIRIT_LEVEL_FLOAT_TC ||
							key == SPIRIT_LEVEL_ZEROING_RANGE_FLOAT)
						{
							posSeparator = key.find('|');
							if (posSeparator != string::npos) key.replace(posSeparator + 1, pos1 - posSeparator - 1, "INT");

							Helpers::ReplaceDecimalPoint(value);
							value = to_string(m_tilt->GetAngleDigit(stof(value)));
						}

						Helpers::KeyAddOrReplace(m_generalSettingsSealed, key, value);
					}
					else
						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror parsing ssp general settings protected -> wrong format", __FUNCTION__);
					break;
				}
			}
		}
		startPos = endPos + 1;
	}
}


/**
******************************************************************************
* RemoveCR - remove all cr from string
*
* @param    str:in	string
*
* @return   errorCode
* @remarks
******************************************************************************
*/
void AdcSsp::RemoveCR(string &str)
{
	size_t  pos = 0;
	// first remove all \n
	do
	{
		pos = str.find('\r', pos);
		if (pos != string::npos) str.erase(pos, 1);
	} while (pos != string::npos);
}


/**
******************************************************************************
* GetTccSettings - get AdcSsp settings
*
* @return   tcc settings map
* @remarks
******************************************************************************
*/
const AdcSsp::KeyValueMap *AdcSsp::GetTccSettings()
{
	return &m_tccSettings;
}


/**
******************************************************************************
* GetLinSettings - get lin settings
*
* @return   lin settings map
* @remarks
******************************************************************************
*/
const AdcSsp::KeyValueMap *AdcSsp::GetLinSettings()
{
	return &m_linSettings;
}


/**
******************************************************************************
* GetWdtaSettings - get weight dependent tilt angle settings
*

* @return   wdta settings map
* @remarks
******************************************************************************
*/
const AdcSsp::KeyValueMap *AdcSsp::GetWdtaSettings(long multiInterval, long resolution, double maxCapacityKG)
{
	string keyStr;
	string value;

	m_wdtaSettings4Extern.clear();

	keyStr = to_string(multiInterval) + "x" + to_string(resolution) + "|ARRAY";

	if (Helpers::KeyExists(m_wdtaSettings, keyStr, value))
	{
		// create tilt angle settings depending on load capacity
		CalcWdtaSettings(value, maxCapacityKG);
	}
	else
	{
		// copy default tilt angle not depending on load capacity
		for (int i = 0; i < WDTA_STRINGS_COUNT; i++)
		{
			if (Helpers::KeyExists(m_wdtaSettings, WDTA_STRINGS[i], value))
			{
				m_wdtaSettings4Extern[WDTA_STRINGS[i]] = value;
			}
		}
	}

	return &m_wdtaSettings4Extern;
}


/**
******************************************************************************
* GetGeneralSettingsProtected - get general settings sealed
*
* @return   general settings sealed map
* @remarks
******************************************************************************
*/
const AdcSsp::KeyValueMap *AdcSsp::GetGeneralSettingsSealed()
{
	return &m_generalSettingsSealed;
}

/**
******************************************************************************
* SetTilt - set pointer to tilt class
*
* @param    tilt:in		pointer to tilt settings
*
* @return   void
* @remarks
******************************************************************************
*/
void AdcSsp::SetTilt(AdcTilt *tilt)
{
	m_tilt = tilt;
}


/**
******************************************************************************
* SetTilt - set pointer to tilt class
*
* @param    wdtaSettings:in		wdta settings as semi-colon separated string
* @param    maxCapacityKG:in	max Capacity in kg
*
* @return   void
* @remarks
******************************************************************************
*/
void AdcSsp::CalcWdtaSettings(string &wdtaSettings, double maxCapacityKG)
{
	int type;
	double val_x1, val_y1, val_x2, val_y2, tmp_val;
	char tmpBuffer[255];
	string tmpWdtaSettings = wdtaSettings;

	Helpers::ReplaceDecimalPoint(tmpWdtaSettings);
	sscanf(tmpWdtaSettings.c_str(), "%d;%lf;%lf;%lf;%lf", &type, &val_x1, &val_y1, &val_x2, &val_y2);

	if (type == 1)
	{
		// val_x1 and val_x2 contain a divisor and not the weight, so we first have to calculate the weight in kg
		val_x1 = maxCapacityKG / val_x1;
		val_x2 = maxCapacityKG / val_x2;
	}

	for (int i = 0; i < WDTA_STRINGS_COUNT; i++)
	{
		tmp_val = 0;
		if (WDTA_STRINGS[i] == "x1|FLOAT") tmp_val = val_x1;
		if (WDTA_STRINGS[i] == "y1|FLOAT") tmp_val = val_y1;
		if (WDTA_STRINGS[i] == "x2|FLOAT") tmp_val = val_x2;
		if (WDTA_STRINGS[i] == "y2|FLOAT") tmp_val = val_y2;

		sprintf(tmpBuffer, "%.3f", tmp_val);
		m_wdtaSettings4Extern[WDTA_STRINGS[i]] = string(tmpBuffer);
	}
}
