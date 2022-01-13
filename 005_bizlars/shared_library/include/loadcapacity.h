#pragma once
#include <string>
#include <map>
#include "bizlars.h"
#include "countrysettings.h"

using namespace std;


class LoadCapacity
{
	typedef map<string, string> settings;

public:
    LoadCapacity();
    ~LoadCapacity();

	short Set(const string &loadCapacity, const string &fileName, const string &path, const map<string, string> *regParamMap = NULL);
	string  GetLoadCapacity();
	void SetRegistrationParameters(const map<string, string> *regParamMap);
	void SetCalTemplateStrings(string maxString, string minString, string eString);
    const map<string, string>* GetSettings();
	void SetSettings(const map<string, string> &lcSettingsMap);

	string MaxString();
	string MinString();
	string eString();

	unsigned short	GetDecimalPlaces();
	unsigned short	GetWeightUnit();
	AdcWeight		GetMaxCapacity();
	double			GetMaxCapacityKG();
	short			GetParams(AdcLoadCapacityParams* params);
	short			GetInitialZeroSetting(AdcInitialZeroSettingParam *params);
	long			GetDigitsResolution();
	long			GetMultiInterval();
	long			GetNumberOfIntervalsE();


	static const long DEFAULT_DIGITS_RESOLUTION;

private:
	bool Parse(string &fileContent, const string &loadCapacity, const map<string, string> *regParamMap = NULL);
    void RemoveCR(string &str);
	void BuildCalStrings();
	template <typename T> void GetValue(const string &key, T &value);
	void GetValue(const string &key, string &value);
	unsigned short GetZeroPlaces(const string &value);
	void SetDecimalSeparator(string &value, unsigned short decimalPlaces);
	void CutDecimalPlaces(string &value, unsigned short cutDecimalPlaces);
	void FormatValue(string &value, unsigned short decimalPlaces);
	unsigned short ConvertWeightUnit2MinEWeightUnit(unsigned short weightUnit);

    settings    m_lcSettings;
    string      m_fileName;
	string		m_maxTemplateStr;
	string		m_maxStr;
	string		m_minTemplateStr;
	string		m_minStr;
	string		m_eTemplateStr;
	string		m_eStr;

	#define NUMBER_MAX_WEIGHT_RANGE		3
	static const short IDX_MAX_WEIGHT_RANGE[NUMBER_MAX_WEIGHT_RANGE];
	static const string WEIGHING_RANGE_STR[];

	static const short IDX_E_IN_WEIGHT_RANGE[];
	static const string E_IN_WEIGHT_RANGE_STR[];
	

	#define NUMBER_WEIGHT_UNIT			6
	static const string WEIGHT_UNIT[];

	static const string MAX_WEIGHT_UNIT;
	static const string WEIGHING_MIN;
	static const string MIN_WEIGHT_UNIT;
	static const string E_WEIGHT_UNIT;
};

