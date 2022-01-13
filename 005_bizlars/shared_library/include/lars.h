/**
******************************************************************************
* File       : lars.h
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : lars class
******************************************************************************
*/
#pragma once
#include <string>
#include <mutex>
#include <map>
#include <vector>
#include "bizlars.h"
#include "adcinterface.h"
#include "adcprotocol.h"
#include "countrysettings.h"
#include "loadcapacity.h"
#include "adcssp.h"
using namespace std;

class Lars
{
    typedef map<unsigned short, string> ProductID2AdcType;
    typedef map<short, short> AdcCap;

public:
	Lars();
	Lars(const char *adcName, const char *protocol, const char *port);
    Lars(const Lars &obj);                              // copy contructor
    ~Lars();

    static const char SOH = 0x01;
    static const char ETB = 0x17;
    static const char STX = 0x02;
    static const char ETX = 0x03;
    static const char FS = 0x1C;
    static const char GS = 0x1D;
    static const char ESC = 0x1B;

 	string*			GetAdcName();

	short			Open(const bool performSwReset = true);
    bool            Close();
    short			GetHandle();
    void            SetHandle(short handle);
    short           GetVersion(char *versionStr, unsigned long *size);
    short           GetCapability(AdcCapabilities cap, unsigned char *value);
    short           SetScaleValues(const AdcScaleValues *scaleValues);
    short           GetScaleValues(AdcScaleValues *scaleValues);
    short           ZeroScale(AdcState *adcState);
    short           SetTare(AdcState *adcState, const AdcTare *tare);
	short           GetTare(AdcState *adcState, AdcTare *tare);
    short           SetTarePriority(AdcState *adcState, const AdcTarePriority prio);
    short           ClearTare(AdcState *adcState);
    short           ReadWeight(const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcTare *tare, AdcBasePrice *basePrice, AdcPrice *sellPrice);
    short           GetLogger(const short index, char *entry, unsigned long *size);
    short           GetRandomAuthentication(unsigned char *random, unsigned long *size);
    short           SetAuthentication(const AdcAuthentication *authentication);
	short           GetHighResolution(const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcWeight *weightHighResolution, AdcTare *tare, long *digitValue);
	short			GetGrossWeight(AdcState *adcState, AdcWeight *grossWeight, AdcWeight *grossWeightHighResolution, long *digitValue);
	short           GetFirstDiagnosticData(const char *name, long *sensorHealthID, AdcSensorHealth *sensorHealth, short *state);
	void			GetNextDiagnosticData(const long sensorHealthID, AdcSensorHealth *sensorHealth, short *state);
	short			ConfigureDiagnosticData(const AdcSensorHealth *sensorHealth);
	void			SetCountryFilesPath(const char *path);
	short           GetSupportedCountries(char *countries, unsigned long *size);
	short           SetCountry(const char *name);
    short           GetCountry(char *name, unsigned long size);
    short           GetCompatibleLoadCapacities(char *loadCapacities, unsigned long *size);
	short           GetCompatibleLoadCapacities(const char *country, char *loadCapacities, unsigned long *size);
    short           SetLoadCapacity(const char *loadCapacity);
	short			GetLoadCapacity(char *loadCapacity, unsigned long size);
	void			SetFirmwarePath(const char *path);
	short			Update(const short force);
	short           GetFirmwareFileVersion(char *versionStr, unsigned long *size);
	short			Calibration(const AdcCalibCmd cmd, AdcState *adcState, long *step, long *calibDigit);
	short           Reset(const AdcResetType type);
	short			Parameters(const AdcParamMode mode);
	short			GetInternalDataEx(AdcInternalDataEx* internalData, bool sendRequestCmd = true);
	short			GetEepromSize(AdcEepromSize *eepromSize);
	void			SetSspPath(const char *path = NULL);
	void			LoadSspParam();
	short			GetScaleModel(char *model, unsigned long size);
	short			SetScaleModel(const char *model = NULL);
	short			SetScaleModelIntern(const char *model = NULL);
	short			GetCalStrings4LoadCapacity(const char *loadCapacity, AdcCalStrings *calStrings);
    bool            operator == (const Lars &lars);
	short			GetPortNr(char *portNr, unsigned long *size);

private:
	static const long  INVALID_SENSOR_ID = -1;

    static const ProductID2AdcType  m_productID2adcType;


    void            Init();
    short           MakeAuthentication(bool withIdentityString = false);
	short			CheckDoAuthentication(bool setApplIdentication = false);
	long			CreateNewSensorHealthID();
	void			SetAdcType(map<string, string> &versionMap);
	void			SetWsType(map<string, string> &versionMap);
	void			SetBootLoaderVersion(map<string, string> &versionMap);
	short			SetAdcVariables();
	short			ReadLcSettings();
	void			InitCapabilities();

    mutex           m_mutex;

	string			m_adcName;
	string			m_protocolType;
	string			m_port;
    string          m_adcType;
	string			m_wsType;
    string          m_adcVersion;
	AdcInterface	*m_interface;
	AdcProtocol	    *m_protocol;
    CountrySettings m_cySetting;
    LoadCapacity    m_lc;
	short			m_logicalHandle;
    AdcCap          m_adcCap;
    short           m_maxDisplayTextChars;
	AdcEepromSize	m_eepromSize;
	bool			m_applAuthenticationDone;
	string			m_firmwarePath;
	string			m_sspPath;
	string			m_scaleModel;
	AdcSsp			m_ssp;
	AdcTilt			*m_tilt;
	AdcOperatingMode m_opMode;
	double			m_bootLoaderVersion;

	map<long, map<string, AdcSensorHealth>>	m_sensorHealth;
};

