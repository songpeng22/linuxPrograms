/**
******************************************************************************
* File       : adcprotocol.h
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcprotocol class (virtual)
******************************************************************************
*/
#pragma once
#include <string>
#include <map>
#include "bizlars.h"
#include "adcinterface.h"
using namespace std;

typedef enum
{
	ADC_FRMUPDATE_START,		// start firmware update
	ADC_FRMUPDATE_END,			// end firmware update
	ADC_FRMUPDATE_CANCEL,		// cancel firmware update
	ADC_FRMUPDATE_WRITE_DATA,	// write firmware data
} AdcFrmCmd;

typedef struct
{
	unsigned long	startAdr;
	short			len;
	unsigned char	data[256];
} AdcFrmData;

class AdcProtocol
{
public:
    AdcProtocol();
	AdcProtocol(AdcInterface *pInterface);
	virtual ~AdcProtocol();

    virtual void SetInterface(AdcInterface *pIinterface);
	virtual short Reset(const AdcResetType type) = 0;
    virtual short ZeroScale(AdcState *adcState) = 0;
    virtual short SetTare(const AdcTare *tare, AdcState *adcState = NULL) = 0;
	virtual short GetTare(AdcTare *tare, AdcState *adcState = NULL) = 0;
    virtual short SetTarePriority(AdcState *adcState, const AdcTarePriority prio) = 0;
    virtual short ClearTare(AdcState *adcState) = 0;
    virtual short ReadWeight(const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcTare *tare, AdcBasePrice *basePrice, AdcPrice *sellPrice) = 0;
    virtual short GetLogger(const short index, char *entry, unsigned long *size) = 0;
    virtual short GetRandomAuthentication(unsigned char *random, unsigned long *size) = 0;
    virtual short SetAuthentication(const AdcAuthentication *authentication) = 0;
    virtual short GetMaxDisplayTextChars(short *number) = 0;
	virtual short GetHighResolution(const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcWeight *weightHighResolution, AdcTare *tare, long *digitValue) = 0;
	virtual short GetGrossWeight(AdcState *adcState, AdcWeight *grossWeight, AdcWeight *grossWeightHighResolution, long *digitValue) = 0;
	virtual short GetDiagnosticData(const char *name, map<string, AdcSensorHealth> &sensorHealth) = 0;
	virtual short ConfigureDiagnosticData(const AdcSensorHealth *sensorHealth) = 0;
    virtual short SetDisplayText(const AdcDisplayText *text) = 0;
    virtual short SetBaseprice(const AdcBasePrice *price, const short frozen = 0) = 0;
    virtual short SetScaleMode(const AdcScaleMode mode) = 0;
    virtual short SetEeprom(const AdcEeprom *eeprom) = 0;
	virtual short SetTiltCompensation(const AdcTiltComp *tiltCompensation, const map<string, string> *tccSettings, const map<string, string> *linSettings, const map<string, string> *wdtaSettings, const AdcSpiritLevelCmd *spiritLevelCmd) = 0;
	virtual short SetTiltAngleConfirm(const AdcTiltAngleConfirmCmd cmd) = 0;
	virtual short SetScaleSpecificSettingsGeneral(const map<string, string> *scaleSpecificSettings, const map<string, string> *scaleSpecificSettingsSealed) = 0;
	virtual short SetFilter(const AdcFilter *filter) = 0;
	virtual short SetGFactor(const short gfactor) = 0;
	virtual short SetProdSettings(const AdcProdSettings *prodSettings) = 0;
	virtual short SetConversionWeightUnit(const long conversionWeightUnitFactor) = 0;
	virtual short SetInterfaceMode(const AdcInterfaceMode *interfaceMode) = 0;
    virtual short GetBaseprice(AdcBasePrice *price) = 0;
    virtual short GetDisplayText(AdcDisplayText *text) = 0;
    virtual short GetScaleMode(AdcScaleMode *mode) = 0;
    virtual short GetEeprom(AdcEeprom *eeprom) = 0;
    virtual short GetTiltCompensation(AdcTiltComp *tiltCompensation) = 0;
	virtual short GetFilter(AdcFilter *filter) = 0;
	virtual short GetCapabilities(map<short, short> &capMap) = 0;
    virtual short SetCountrySettings(const map<string, string> *cySettingsMap) = 0;
	virtual short GetCountrySettings(map < string, string> &cySettingsMap) = 0;
    virtual short SetLoadCapacity(const map<string, string> *lcSettingsMap) = 0;
	virtual short GetLoadCapacity(map<string, string> &cySettingsMap) = 0;
    virtual short GetVersion(map<string, string> &verMap) = 0;
	virtual short GetGFactor(short *gFactor) = 0;
	virtual short GetProdSettings(AdcProdSettings *prodSettings) = 0;
	virtual short GetInterfaceMode(AdcInterfaceMode *interfaceMode) = 0;
	virtual short Calibration(const AdcCalibCmd cmd, AdcState *adcState, long *step, long *calibDigit) = 0;
	virtual short Parameters(const AdcParamMode mode) = 0;
	virtual short GetInternalDataEx(AdcInternalDataEx *internalData, bool sendRequestCmd = true) = 0;
	virtual short GetEepromSize(AdcEepromSize *eepromSize) = 0;
	virtual short SetZeroPointTracking(short mode) = 0;
	virtual short GetZeroPointTracking(short *mode) = 0;
	virtual short SetVerifParamProtected() = 0;
	virtual short GetVerifParamProtected(short *mode) = 0;
	virtual short SetUpdateAllowed(short mode) = 0;
	virtual short GetUpdateAllowed(short *mode) = 0;
	virtual short GetRemainingWarmUpTime(long *seconds) = 0;
	virtual short SetWarmUpTime(long seconds) = 0;
	virtual short GetWarmUpTime(long *seconds) = 0;
	virtual short SetOperatingMode(AdcOperatingMode mode) = 0;
	virtual short GetOperatingMode(AdcOperatingMode *mode) = 0;
	virtual short SetZeroSettingInterval(long seconds) = 0;
	virtual short GetZeroSettingInterval(long *seconds) = 0;
	virtual short SetAutomaticZeroSettingTime(long seconds) = 0;
	virtual short GetAutomaticZeroSettingTime(long *seconds) = 0;
	virtual short SetScaleModel(const string &model) = 0;
	virtual short GetScaleModel(string &model) = 0;
	virtual short SetStateAutomaticTiltSensor(const bool state) = 0;
	virtual short GetStateAutomaticTiltSensor(bool *state) = 0;

	virtual short FirmwareUpdate(const AdcFrmCmd cmd, const AdcFrmData *data, const string *usbStackVersion = NULL, const string *welmecStructureVersion = NULL, const short force = 0) = 0;
	virtual short SetInitialZeroSetting(const AdcInitialZeroSettingParam *initialZeroSettingParam) = 0;

	static const unsigned long RECEIVE_BUFFER_SIZE = 0x10000;
			
protected:
	AdcInterface	*m_interface;
    char            *m_receiveBuffer;
	long			m_major;
	long			m_minor;

private:
	void Init(AdcInterface *Interface);
};

