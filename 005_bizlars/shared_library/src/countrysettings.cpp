#ifdef  _MSC_VER
#include <Windows.h>
#endif
#ifdef __GNUC__
#include <dirent.h>
#endif

#include "helpers.h"
#include "larsErr.h"
#include "adctrace.h"
#include "crypto.h"
#include "adcrbs.h"
#include "adctilt.h"
#include "countrysettings.h"

const string CountrySettings::TCS_LIMIT_FLOAT = "tclimitcs|FLOAT";
const string CountrySettings::m_fileName = "countrysettings_";

CountrySettings::CountrySettings()
{
	m_tilt = NULL;

    m_cySettings.clear();
	m_regParamMap.clear();
    m_loadCapacityMap.clear();
    m_supportedCountries.clear();

	m_path.clear();

    ReadSupportedCountries();
}


CountrySettings::~CountrySettings()
{
}


void CountrySettings::SetPath(const string&path)
{
	m_path = path;
	// path changed, so re-read all supported countries
	ReadSupportedCountries();
}


string CountrySettings::GetPath()
{
	return m_path;
}


string CountrySettings::GetCountry()
{
	string value;

	value.clear();

	Helpers::KeyExists(m_cySettings, AdcRbs::COUNTRY_SPECIFIC_STRINGS[AdcRbs::COUNTRY_SETTING_COUNTRY], value);

	return value;
}


short CountrySettings::SetCountry(const string& country)
{
    return (SetCountry(country, m_path));
}


short CountrySettings::SetCountry(const string& country, const string& path)
{
    string fileContent;
    string ctyFileName;
    string saveCountryISO;
    Crypto cryptVar;
    short  errorCode;

    if (path.empty())
		ctyFileName = m_fileName + country + ".txt";
    else
		ctyFileName = path + "/" + m_fileName + country + ".txt";

    // read and decrypt country settings file
    errorCode = cryptVar.ReadEncryptedFile(ctyFileName, fileContent);
    if (errorCode != LarsErr::E_SUCCESS)
    {
		// check if country is already set
		if (country == GetCountry()) errorCode = LarsErr::E_SUCCESS;
        return errorCode;
    }

	m_cySettings.clear();
	m_regParamMap.clear();
	m_loadCapacityMap.clear();

    // parse country settings file
    Parse(fileContent, country);

    return errorCode;
}


const map<string, string> *CountrySettings::GetSettings()
{
    return &m_cySettings;
}


void CountrySettings::SetSettings(const map<string, string> &cySettingsMap)
{
	string value;

	// copy content of cySettingsMap to m_cySettings
	m_cySettings.clear();
	m_cySettings.insert(cySettingsMap.begin(), cySettingsMap.end());
}


bool CountrySettings::Parse(string &fileContent, const string &country)
{
    short   state = UNSUPPORTED;
    string  oneRow, key, value;
    size_t  endPos = 0;
    size_t  pos1 = 0;
	size_t	offset;
	size_t	posSeparator = 0;
	string  areaCountrySettings = "[country settings " + country + "]";
	string	areaRegParams = "[registration parameters]";
	string  areaLoadCapacity = "[load capacities]";
	
    // remove all CR from string
    RemoveCR(fileContent);

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
			if (oneRow == areaCountrySettings)
				state = COUNTRY_SETTING;
			else if (oneRow == areaRegParams)
				state = REGISTRATION_PARAMETER;
            else if (oneRow == areaLoadCapacity)
                state = SUPPORTED_LOADCELL;
            else
            {
                switch (state)
                {
                case COUNTRY_SETTING:
                    pos1 = oneRow.find('=');
					if (pos1 != string::npos)
					{
						key = oneRow.substr(0, pos1);
						value = oneRow.substr(pos1 + 1);

						// patch tclimitcs key
						if (key == TCS_LIMIT_FLOAT)
						{
							posSeparator = key.find('|');
							if (posSeparator != string::npos) key.replace(posSeparator + 1, pos1 - posSeparator - 1, "INT");

							if (m_tilt)
							{
								Helpers::ReplaceDecimalPoint(value);
								value = to_string(m_tilt->GetAngleDigit(stof(value)));
							}
						}

						m_cySettings[key] = value;
					}
                    else
						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror parsing country settings for %s -> wrong format", __FUNCTION__, country.c_str());
                    break;

				case REGISTRATION_PARAMETER:
					pos1 = oneRow.find('=');
					if (pos1 != string::npos)
						m_regParamMap[oneRow.substr(0, pos1)] = oneRow.substr(pos1 + 1);
					else
						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror parsing country settings for %s -> wrong format", __FUNCTION__, country.c_str());
					break;

                case SUPPORTED_LOADCELL:
                    pos1 = oneRow.find('=');
                    if (pos1 != string::npos)
                        m_loadCapacityMap[oneRow.substr(0, pos1)] = oneRow.substr(pos1 + 1);
                    else
						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror parsing country settings for %s -> wrong format", __FUNCTION__, country.c_str());
                    break;
                }
            }
        }
        fileContent = fileContent.substr(endPos + offset);			// add offset because of \n
    }

    return true;
}


void CountrySettings::GetSupportedCountries(supportedCountries& countries)
{
	countries = m_supportedCountries;
}


void CountrySettings::ReadSupportedCountries()
{
    ReadSupportedCountries(m_path);
}


void CountrySettings::ReadSupportedCountries(const string& path)
{
    string countryISO;
    size_t pos1, pos2;

#ifdef _MSC_VER
    WIN32_FIND_DATAA fileData;
    HANDLE hFind;
    string filePattern;

	m_supportedCountries.clear();

    if (path.empty())
    {
        filePattern = "./countrysettings_*.txt";
    }
    else
    {
        filePattern = path + "/countrysettings_*.txt";
    }

    if (!((hFind = FindFirstFileA(filePattern.c_str(), &fileData)) == INVALID_HANDLE_VALUE))
    {
        do
        {
            // check for file
            if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                countryISO = fileData.cFileName;
                pos1 = countryISO.find_last_of('_');
                pos2 = countryISO.find_last_of('.');
                if ((pos1 != string::npos) && (pos2 != string::npos))
                {
                    countryISO = countryISO.substr(pos1 + 1, pos2 - pos1 - 1);
                    m_supportedCountries.push_back(countryISO);
                }
            }
        } while (FindNextFileA(hFind, &fileData));
    }

    FindClose(hFind);
#endif

#ifdef __GNUC__
    DIR 			*dp;
    struct dirent 	*dirp;
    string 			dir;
    string			filePattern = "countrysettings_";

	m_supportedCountries.clear();

    if (path.empty())
    {
        dir = "./";
    }
    else
    {
        dir = path;
    }

    //if ((dp = opendir(filePattern.c_str())) != NULL)
    if ((dp = opendir(dir.c_str())) != NULL)
    {
        while ((dirp = readdir(dp)) != NULL)
        {
			countryISO = dirp->d_name;
			if (countryISO.find(filePattern) != string::npos)
			{
				pos1 = countryISO.find_last_of('_');
				pos2 = countryISO.find_last_of('.');
				if ((pos1 != string::npos) && (pos2 != string::npos))
				{
					countryISO = countryISO.substr(pos1 + 1, pos2 - pos1 - 1);
					m_supportedCountries.push_back(countryISO);
				}
			}
        }
    }

    closedir(dp);
#endif
}


void CountrySettings::GetCompatibleLoadCapacities(loadCapacity &loadCellMap)
{
    loadCellMap = m_loadCapacityMap;
}


const map<string, string> *CountrySettings::GetRegistrationParameters()
{
	return &m_regParamMap;
}


void CountrySettings::RemoveCR(string &str)
{
    size_t  pos = 0;
    // first remove all \n
    do
    {
        pos = str.find('\r', pos);
        if (pos != string::npos) str.erase(pos, 1);
    } while (pos != string::npos);
}


string CountrySettings::MaxTemplateString()
{
	string value;

	value.clear();

	Helpers::KeyExists(m_cySettings, AdcRbs::COUNTRY_SPECIFIC_STRINGS[AdcRbs::COUNTRY_SETTING_MAXSTR], value);

	return value;
}


string CountrySettings::MinTemplateString()
{
	string value;

	value.clear();

	Helpers::KeyExists(m_cySettings, AdcRbs::COUNTRY_SPECIFIC_STRINGS[AdcRbs::COUNTRY_SETTING_MINSTR], value);

	return value;
}


string CountrySettings::eTemplateString()
{
	string value;

	value.clear();

	Helpers::KeyExists(m_cySettings, AdcRbs::COUNTRY_SPECIFIC_STRINGS[AdcRbs::COUNTRY_SETTING_ESTR], value);

	return value;
}


void CountrySettings::SetTilt(AdcTilt *tilt)
{
	m_tilt = tilt;
}
