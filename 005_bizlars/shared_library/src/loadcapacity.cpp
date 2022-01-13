#include <sstream> 
#ifdef  __GNUC__
#include <string.h>
#include <math.h>
#endif
#include "helpers.h"
#include "larsErr.h"
#include "adctrace.h"
#include "crypto.h"
#include "loadcapacity.h"
#include "adcrbs.h"


#define CONST_CONVERT_LB_KG		0.453592
#define CONST_CONVERT_OZ_KG		0.0283495
#define CONST_CONVERT_GR_KG		0.001
#define CONST_CONVERT_MGR_KG	1e-6
#define CONST_CONVERT_UGR_KG	1e-9


const short LoadCapacity::IDX_MAX_WEIGHT_RANGE[NUMBER_MAX_WEIGHT_RANGE] = { AdcRbs::LOAD_CAPACITY_BEREICH, AdcRbs::LOAD_CAPACITY_TEILGRENZE0, AdcRbs::LOAD_CAPACITY_TEILGRENZE1 };
const string LoadCapacity::WEIGHING_RANGE_STR[NUMBER_MAX_WEIGHT_RANGE] = { "{wx}", "{w0}", "{w1}" };

const short LoadCapacity::IDX_E_IN_WEIGHT_RANGE[] = { AdcRbs::LOAD_CAPACITY_TEILSCHRITT0, AdcRbs::LOAD_CAPACITY_TEILSCHRITT1, AdcRbs::LOAD_CAPACITY_TEILSCHRITT2 };
const string LoadCapacity::E_IN_WEIGHT_RANGE_STR[NUMBER_MAX_WEIGHT_RANGE] = { "{t0}", "{t1}", "{t2}" };

// Attention: string array must have the identical instruction as enum AdcWeightUnit
const string LoadCapacity::WEIGHT_UNIT[NUMBER_WEIGHT_UNIT] = { "{kg}", "{lb}", "{oz}", "{g}", "{mg}", "{ug}" };

const string LoadCapacity::MAX_WEIGHT_UNIT = "{ux}";
const string LoadCapacity::WEIGHING_MIN = "{wm}";
const string LoadCapacity::MIN_WEIGHT_UNIT = "{um}";
const string LoadCapacity::E_WEIGHT_UNIT = "{ut}";

const long LoadCapacity::DEFAULT_DIGITS_RESOLUTION = 120000;

LoadCapacity::LoadCapacity()
{
    m_lcSettings.clear();
	m_maxTemplateStr.clear();
	m_maxStr.clear();
	m_minTemplateStr.clear();
	m_minStr.clear();
	m_eTemplateStr.clear();
	m_eStr.clear();
}


LoadCapacity::~LoadCapacity()
{
}

short LoadCapacity::Set(const string &loadCapacity, const string &fileName, const string &path, const map<string, string> *regParamMap)
{
    short   errorCode;
    Crypto  cryptVar;
    string  fileContent;

	if (path.empty())
		m_fileName = fileName;
	else
		m_fileName = path + "/" + fileName;


    // read and decrypt country settings file
	errorCode = cryptVar.ReadEncryptedFile(m_fileName, fileContent);
    if (errorCode != LarsErr::E_SUCCESS)
    {
		if (loadCapacity == GetLoadCapacity()) errorCode = LarsErr::E_SUCCESS;
        return errorCode;
    }

    // parse load capacity file
	Parse(fileContent, loadCapacity, regParamMap);

	// create the calibration strings
	BuildCalStrings();

    return errorCode;
}


const map<string, string> *LoadCapacity::GetSettings()
{
    return &m_lcSettings;
}


void LoadCapacity::SetSettings(const map<string, string> &lcSettingsMap)
{
	// copy content of cySettingsMap to m_cySettings
	m_lcSettings.clear();
	m_lcSettings.insert(lcSettingsMap.begin(), lcSettingsMap.end());
}


bool LoadCapacity::Parse(string &fileContent, const string &loadCapacity, const map<string, string> *regParamMap)
{
    string  oneRow;
    size_t  endPos = 0;
    size_t  pos1 = 0;
	size_t	offset;

	m_lcSettings.clear();

    // remove all CR from string
    RemoveCR(fileContent);

    // always skip first line
    endPos = fileContent.find('\n');
	if (endPos == string::npos)
	{
		endPos = fileContent.size();
	}
    oneRow = fileContent.substr(0, endPos);
    fileContent = fileContent.substr(endPos + 1);

    // parse rest of file
    while (!fileContent.empty())
    {
        endPos = fileContent.find('\n');
		if (endPos == string::npos)
		{
			// no \n in string
			endPos = fileContent.size();
			offset = 0;
		}
		else
		{
			offset = 1;
		}
        oneRow = fileContent.substr(0, endPos);

        if (!oneRow.empty())
        {
            pos1 = oneRow.find('=');
			if (pos1 != string::npos)
			{
				m_lcSettings[oneRow.substr(0, pos1)] = oneRow.substr(pos1 + 1);
				if (oneRow.substr(0, pos1) == "teilSchritt[0]|INT")
				{
					// if we had read e call SetRegistrationParameters to set the parameters
					SetRegistrationParameters(regParamMap);
				}
			}
            else
				g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror parsing load capacity settings for %s -> wrong format", __FUNCTION__, loadCapacity.c_str());
        }
        fileContent = fileContent.substr(endPos + offset);		// add offset because of \n
    }

    return true;
}


void LoadCapacity::RemoveCR(string &str)
{
    size_t  pos = 0;
    // first remove all \n
    do
    {
        pos = str.find('\r', pos);
        if (pos != string::npos) str.erase(pos, 1);
    } while (pos != string::npos);

}


string LoadCapacity::MaxString()
{
	return m_maxStr;
}


string LoadCapacity::MinString()
{
	return m_minStr;
}


string LoadCapacity::eString()
{
	return m_eStr;
}


string LoadCapacity::GetLoadCapacity()
{
	string value;

	value.clear();

	Helpers::KeyExists(m_lcSettings, AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_TRALACODE], value);

	return value;
}


unsigned short LoadCapacity::GetDecimalPlaces()
{
	unsigned short decimalPlaces = 0;

	// get decimalPlaces
	GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_KOMMA], decimalPlaces);

	return decimalPlaces;
}


unsigned short LoadCapacity::GetWeightUnit()
{
	unsigned short weightUnit = 0;

	// get weight Unit
	GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_GEWICHTSEINHEIT], weightUnit);

	return weightUnit;
}


long LoadCapacity::GetDigitsResolution()
{
	long digitsResolution = 0;

	// get digits resolution
	GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_AUFLOESUNG], digitsResolution);

	if (digitsResolution == 0) digitsResolution = DEFAULT_DIGITS_RESOLUTION;		// set default value

	return digitsResolution;
}


void LoadCapacity::SetCalTemplateStrings(string maxString, string minString, string eString)
{
	m_maxTemplateStr = maxString;
	m_minTemplateStr = minString;
	m_eTemplateStr = eString;

	// create the calibration strings
	BuildCalStrings();
}


void LoadCapacity::SetRegistrationParameters(const map<string, string> *regParams)
{
	size_t  pos1 = 0;
	string firstStr, secondStr;
	string value;
	int	eValue, eMul;

	if (regParams)
	{
		if (!regParams->empty())
		{
			for (map<string, string>::const_iterator it = regParams->begin(); it != regParams->end(); it++)
			{
				firstStr = (*it).first;
				secondStr = (*it).second;

				if (firstStr.find("RegGrenze_e") != string::npos)
				{
					// convert registration limit from e to weight
					pos1 = firstStr.find("_");
					if (pos1 != string::npos)
					{
						firstStr.erase(pos1, 2);			// erase character "_e"
					}
					if (Helpers::KeyExists(m_lcSettings, "teilSchritt[0]|INT", value))
					{
						eValue = atoi(value.c_str());
						eMul = atoi(secondStr.c_str());
						secondStr = to_string(eMul * eValue);

						Helpers::KeyAddOrReplace(m_lcSettings, firstStr, secondStr);
					}
				}
				else
				{
					Helpers::KeyAddOrReplace(m_lcSettings, firstStr, secondStr);
				}
			}
		}
	}
}


void LoadCapacity::BuildCalStrings()
{
	string value;
	string value1;
	unsigned short valInt;
	unsigned short decimalPlaces = 0;
	unsigned short multiDivision;
	unsigned short weightUnit;
	unsigned short cutDecimalPlaces;
	size_t pos;

	// get multi division value
	GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_MEHRTEILUNG], multiDivision);
	// get weight unit
	weightUnit = GetWeightUnit();
	// get decimalPlaces
	decimalPlaces = GetDecimalPlaces();

	// max string
	m_maxStr = m_maxTemplateStr;

	for (int idx = 0; idx < NUMBER_MAX_WEIGHT_RANGE; idx++)
	{
		if (idx < multiDivision)
		{
			// get max weight
			GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[IDX_MAX_WEIGHT_RANGE[idx]], value);
			// format value
			FormatValue(value, decimalPlaces);

			// if multiDivision and libs the weight range is decoded in max string
			if ((weightUnit == 1) && (multiDivision > 1))
			{
				if (idx < multiDivision - 1)
				{
					// get max weight
					GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[IDX_MAX_WEIGHT_RANGE[idx + 1]], value1);
					// format value
					FormatValue(value1, decimalPlaces);
				}
				else
				{
					value1 = "0";
				}
				value = value1 + "-" + value;
			}

			// replace value
			pos = m_maxStr.find(WEIGHING_RANGE_STR[idx]);
			if (pos != string::npos)
				m_maxStr.replace(pos, WEIGHING_RANGE_STR[idx].length(), value);
		}
		else
		{
			value.clear();
			pos = m_maxStr.find(WEIGHING_RANGE_STR[idx]);
			if (pos != string::npos)
				m_maxStr.replace(pos, WEIGHING_RANGE_STR[idx].length() + 1, value);
		}
	}

	// replace weight unit in max string
	if (weightUnit < NUMBER_WEIGHT_UNIT)
	{
		pos = m_maxStr.find(MAX_WEIGHT_UNIT);
		if (pos != string::npos)
			m_maxStr.replace(pos, MAX_WEIGHT_UNIT.length(), WEIGHT_UNIT[weightUnit]);
	}


	// min string
	m_minStr = m_minTemplateStr;

	if (m_minStr.length() > 0)
	{
		// get e
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_TEILSCHRITT0], valInt);

		// to get min weight multiple e with 20
		valInt *= 20;

		if ((weightUnit == AdcWeightUnit::ADC_POUND) || (weightUnit == AdcWeightUnit::ADC_OUNCE))
		{
			// handle lb and oz
			value = to_string(valInt);

			SetDecimalSeparator(value, decimalPlaces);

			// cut the last places
			GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_GENGEW0 + multiDivision - 1], cutDecimalPlaces);
			CutDecimalPlaces(value, cutDecimalPlaces);

			// replace value
			pos = m_minStr.find(WEIGHING_MIN);
			if (pos != string::npos)
				m_minStr.replace(pos, WEIGHING_MIN.length(), value);
		}
		else
		{
			// replace value
			pos = m_minStr.find(WEIGHING_MIN);
			if (pos != string::npos)
				m_minStr.replace(pos, WEIGHING_MIN.length(), to_string(valInt));
		}

		// replace weight unit in min string
		pos = m_minStr.find(MIN_WEIGHT_UNIT);
		if (pos != string::npos)
			m_minStr.replace(pos, MIN_WEIGHT_UNIT.length(), WEIGHT_UNIT[ConvertWeightUnit2MinEWeightUnit(weightUnit)]);
	}


	// e string
	m_eStr = m_eTemplateStr;

	for (int idx = 0; idx < NUMBER_MAX_WEIGHT_RANGE; idx++)
	{
		if (idx < multiDivision)
		{
			// get e
			GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[IDX_E_IN_WEIGHT_RANGE[idx]], value);

			if ((weightUnit == AdcWeightUnit::ADC_POUND) || (weightUnit == AdcWeightUnit::ADC_OUNCE))
			{
				// handle lb and oz
				SetDecimalSeparator(value, decimalPlaces);

				// cut the last places
				GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_GENGEW0 + idx], cutDecimalPlaces);
				CutDecimalPlaces(value, cutDecimalPlaces);
			}

			pos = m_eStr.find(E_IN_WEIGHT_RANGE_STR[idx]);
			if (pos != string::npos)
				m_eStr.replace(pos, E_IN_WEIGHT_RANGE_STR[idx].length(), value);
		}
		else
		{
			value.clear();
			pos = m_eStr.find(E_IN_WEIGHT_RANGE_STR[idx]);
			if ((pos != string::npos) && (pos > 1))
			{
				pos--;
				m_eStr.replace(pos, WEIGHING_RANGE_STR[idx].length() + 1, value);
			}
		}
	}

	while (1)
	{
		// replace weight unit in e string
		pos = m_eStr.find(E_WEIGHT_UNIT);
		if (pos != string::npos)
			m_eStr.replace(pos, E_WEIGHT_UNIT.length(), WEIGHT_UNIT[ConvertWeightUnit2MinEWeightUnit(weightUnit)]);
		else
			break;
	}
}


template <typename T>
void LoadCapacity::GetValue(const string &key, T &value)
{
	string valueStr;
	if (Helpers::KeyExists(m_lcSettings, key, valueStr))
	{
		if (!(istringstream(valueStr) >> value)) value = 0;
	}
	else value = 0;
}


void LoadCapacity::GetValue(const string &key, string &value)
{
	if (!Helpers::KeyExists(m_lcSettings, key, value))
	{
		value.clear();
	}
}


unsigned short LoadCapacity::GetZeroPlaces(const string &value)
{
	unsigned short count = 0;

	for (unsigned int idx = 0; idx < value.length(); idx++)
	{
		if (value[idx] != '0') count = 0;
		else count++;
	}

	return count;
}


void LoadCapacity::SetDecimalSeparator(string &value, unsigned short decimalPlaces)
{
	while (value.length() <= decimalPlaces)
	{
		value.insert(0, "0");
	}

	value.insert(value.length() - decimalPlaces, ".");
}


void LoadCapacity::CutDecimalPlaces(string &value, unsigned short cutDecimalPlaces)
{
	if (cutDecimalPlaces)
	{
		value = value.substr(0, value.length() - cutDecimalPlaces);
	}
}


void LoadCapacity::FormatValue(string &value, unsigned short decimalPlaces)
{
	// get count of zero places 
	unsigned short zeroPlaces = GetZeroPlaces(value);

	if (decimalPlaces <= zeroPlaces)
	{
		value = value.substr(0, value.length() - decimalPlaces);
	}
	else
	{
		value = value.substr(0, value.length() - zeroPlaces);
		// set decimal separator
		SetDecimalSeparator(value, decimalPlaces - zeroPlaces);
	}
}


unsigned short LoadCapacity::ConvertWeightUnit2MinEWeightUnit(unsigned short weightUnit)
{
	unsigned short minWeightUnit;

	switch (weightUnit)
	{
	case AdcWeightUnit::ADC_POUND:			// lb
	case AdcWeightUnit::ADC_OUNCE:			// oz
	case AdcWeightUnit::ADC_GRAM:			// g
	case AdcWeightUnit::ADC_MILLIGRAM:		// mg
	case AdcWeightUnit::ADC_MICROGRAM:		// ug
		minWeightUnit = weightUnit;		
		break;

	default:	// kg
		minWeightUnit = AdcWeightUnit::ADC_GRAM;		// gr
	}

	return minWeightUnit;
}


AdcWeight LoadCapacity::GetMaxCapacity()
{
	AdcWeight maxCapacity;

	// get max weight
	GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_BEREICH], maxCapacity.value);

	maxCapacity.weightUnit = (AdcWeightUnit) GetWeightUnit();
	maxCapacity.decimalPlaces = GetDecimalPlaces();

	return maxCapacity;
}


double LoadCapacity::GetMaxCapacityKG()
{
	double maxCapacityKG = 0.0;
	AdcWeight maxCapacityWeight = GetMaxCapacity();

	switch (maxCapacityWeight.weightUnit)
	{
	case ADC_KILOGRAM:
		maxCapacityKG = (double)maxCapacityWeight.value / pow(10, maxCapacityWeight.decimalPlaces);
		break;

	case ADC_POUND:
		maxCapacityKG = ((double)maxCapacityWeight.value / pow(10, maxCapacityWeight.decimalPlaces)) * CONST_CONVERT_LB_KG;
		break;

	case ADC_OUNCE:
		maxCapacityKG = ((double)maxCapacityWeight.value / pow(10, maxCapacityWeight.decimalPlaces)) * CONST_CONVERT_OZ_KG;
		break;

	case ADC_GRAM:
		maxCapacityKG = ((double)maxCapacityWeight.value / pow(10, maxCapacityWeight.decimalPlaces)) * CONST_CONVERT_GR_KG;
		break;

	case ADC_MILLIGRAM:
		maxCapacityKG = ((double)maxCapacityWeight.value / pow(10, maxCapacityWeight.decimalPlaces)) * CONST_CONVERT_MGR_KG;
		break;

	case ADC_MICROGRAM:
		maxCapacityKG = ((double)maxCapacityWeight.value / pow(10, maxCapacityWeight.decimalPlaces)) * CONST_CONVERT_UGR_KG;
		break;
	}

	return maxCapacityKG;
}


short LoadCapacity::GetParams(AdcLoadCapacityParams* params)
{
	short errorCode = LarsErr::E_SUCCESS;
	long tareLimitKnown;
	short decimalPlaces;
	short decimalPlacesInterval;

	if (params)
	{
		string loadCapacityStr = GetLoadCapacity();
		if (loadCapacityStr.size() < LOAD_CAPACITY_SIZE)
			strcpy(params->loadCapacity, loadCapacityStr.c_str());
		else
			params->loadCapacity[0] = 0;

		// get max weight
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_BEREICH], params->maxCapacity);

		// count interval
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_MEHRTEILUNG], params->multiInterval);

		// limit interval 1
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_TEILGRENZE0], params->limitInterval1);

		// limit interval 2
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_TEILGRENZE1], params->limitInterval2);

		// verification scale interval e1
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_TEILSCHRITT0], params->e1);

		// verification scale interval e2
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_TEILSCHRITT1], params->e2);

		// verification scale interval e3
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_TEILSCHRITT2], params->e3);

		// tare limit weighed
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_TARABEREICH], params->tareLimitWeighed);

		// tare limit known
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_TARAHANDBEREICH], tareLimitKnown);
		if (((tareLimitKnown == 0) && (params->multiInterval == 1)) ||
			(tareLimitKnown == 1))
			params->tareLimitKnown = params->maxCapacity;
		else
			params->tareLimitKnown = params->limitInterval1;

		// decimal places
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_KOMMA], decimalPlaces);

		// decimal places interval 1
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_GENGEW0], decimalPlacesInterval);
		params->decimalPlaces1 = decimalPlaces - decimalPlacesInterval;

		// decimal places interval 2
		if (params->multiInterval > 1)
		{
			GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_GENGEW1], decimalPlacesInterval);
			params->decimalPlaces2 = decimalPlaces - decimalPlacesInterval;
		}
		else
			params->decimalPlaces2 = 0;

		// decimal places interval 3
		if (params->multiInterval > 2)
		{
			GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_GENGEW2], decimalPlacesInterval);
			params->decimalPlaces3 = decimalPlaces - decimalPlacesInterval;
		}
		else
			params->decimalPlaces3 = 0;

		params->unit = (AdcWeightUnit)GetWeightUnit();
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


short LoadCapacity::GetInitialZeroSetting(AdcInitialZeroSettingParam *params)
{
	short errorCode = LarsErr::E_SUCCESS;

	if (params)
	{
		// get neg. initial zero setting
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_EINSCHALTNULLBERMI], params->lowerLimit);

		// get pos. initial zero setting
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_EINSCHALTNULLBERPL], params->upperLimit);
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;;
}


long LoadCapacity::GetMultiInterval()
{
	long multiInterval;

	// count interval
	GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_MEHRTEILUNG], multiInterval);

	return multiInterval;
}


long LoadCapacity::GetNumberOfIntervalsE()
{
	long multiInterval;
	long e;
	long numberOfIntervalsE;

	AdcWeight maxCapacity = GetMaxCapacity();

	// count interval
	GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_MEHRTEILUNG], multiInterval);

	switch (multiInterval)
	{
	case 1:
		// verification scale interval e1
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_TEILSCHRITT0], e);
		break;
	case 2:
		// verification scale interval e2
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_TEILSCHRITT1], e);
		break;
	case 3:
		// verification scale interval e3
		GetValue(AdcRbs::LOAD_CAPACITY_STRINGS[AdcRbs::LOAD_CAPACITY_TEILSCHRITT2], e);
		break;
	}

	numberOfIntervalsE = long(maxCapacity.value / e);

	return numberOfIntervalsE;
}
