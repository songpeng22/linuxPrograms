#pragma once
#include <string>
#include <vector>
#include <map>
#include "adctilt.h"
using namespace std;

class CountrySettings
{
public:
	typedef map<string, string> countrySettings;
	typedef map<string, string> registrationParam;
	typedef map<string, string> loadCapacity;
	typedef vector<string> supportedCountries;

    CountrySettings();
    ~CountrySettings();

	void	SetPath(const string &path);
	string  GetPath();
    short   SetCountry(const string &country, const string &path);
    short   SetCountry(const string &country);
    string  GetCountry();
	void	GetSupportedCountries(supportedCountries &countries);
	const map<string, string>* GetRegistrationParameters();
    void    GetCompatibleLoadCapacities(loadCapacity &loadCellMap);
    const map<string, string>* GetSettings();
	void    SetSettings(const map<string, string> &cySettingsMap);
	string	MaxTemplateString();
	string	MinTemplateString();
	string	eTemplateString();
	void	SetTilt(AdcTilt *tilt);

private:
	void    ReadSupportedCountries();
	void    ReadSupportedCountries(const string &path);
	bool	Parse(string &fileContent, const string &country);
    void    RemoveCR(string &str);

    static const  string m_fileName;
	string				m_path;

	AdcTilt				*m_tilt;

    countrySettings     m_cySettings;
	registrationParam	m_regParamMap;
    loadCapacity        m_loadCapacityMap;
    supportedCountries  m_supportedCountries;

	static const string		TCS_LIMIT_FLOAT;

    static const short		UNSUPPORTED = 0;
    static const short		COUNTRY_SETTING = 1;
	static const short		REGISTRATION_PARAMETER = 2;
    static const short		SUPPORTED_LOADCELL = 3;
};

