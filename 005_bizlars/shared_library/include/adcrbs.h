/**
******************************************************************************
* File       : adcrbs.h
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcrbs class: impement the protocol retail bus sensoric
******************************************************************************
*/
#pragma once
#include <string>
#include <map>
#include "adcinterface.h"
#include "adcprotocol.h"
using namespace std;


class AdcRbs : public AdcProtocol
{
    typedef enum
    {
        RBS_FULL_TELEGRAM = 0,          // true: adc support authentication
        RBS_REFERENCE_DATA,				// true: weight display available
    } ProtocolType;

    typedef map<string, string> keyValuePair;

    typedef struct
    {
        unsigned char   *value;
        unsigned long   *size;
    }AdcRandom;

    typedef struct
    {
        short   id;
        union
        {
            AdcBasePrice      *bp;
            AdcPrice          *sp;
            AdcDisplayText    *dt;
            AdcEeprom         *ee;
            AdcTiltComp       *tc;
            AdcTare           *tare;
            AdcWeight         *wt;
            AdcPrice          *pr;
            AdcAuthentication *au;
            AdcState          *adcState;
            AdcTarePriority   tp;
            AdcScaleMode      sm;
            AdcRandom         random;
			AdcCalibCmd		  calibCmd;
			AdcFilter		  *filter;
			AdcParamMode	  paramMode;
			AdcSensorHealth	  *sensorHealth;
			AdcResetType	  resetType;
			AdcOperatingMode  opMode;
			AdcFrmCmd		  frmCmd;
			AdcFrmData		  *frmData;
			AdcSpiritLevelCmd			spiritLevelCmd;
			AdcInitialZeroSettingParam	*initialZeroSettingParam;
			AdcTiltAngleConfirmCmd		tiltAngleConfirmCmd;
			char			  *frmStringData;
			short			  frmForce;
            short             rr;
            short             idx;
            char              *dia;
            short             stat;
            long              raw;
            short             ndt;
			short			  frozen;
			long			  step;
			map<string, string> *csp;
			map<string, string> *lcp;
			map<string, string> *tcc;
			map<string, string> *lin;
			map<string, string> *wdta;
			map<string, string> *ssp;
			map<string, string> *ssps;
			char			  *rawID;
			long			  size;
			short			  gFactor;
			short			  prodSiteID;
			char			  dateStr[10];
			char			  wsc[21];
			long			  conversionWeightUnit;
			short			  interfaceAutoDetection;
			short			  interfaceMode;
			short			  mode;
			short			  verifParamProtected;
			short			  updateAllowed;
			long			  warmUpTime;
			long			  remainingWarmUpTime;
			long			  zeroSettingInterval;
			long			  automaticZeroSettingTime;
			long			  digitValue;
			long			  calibDigit;
			string			  *scaleModel;
			bool			  state;
		}u;
    }RefDataStruct;



public:
    AdcRbs();
	AdcRbs(AdcInterface *pInterface);
	~AdcRbs();

	short Reset(const AdcResetType type);
    short SetTare(const AdcTare *tare, AdcState *adcState = NULL);
	short GetTare(AdcTare *tare, AdcState *adcState = NULL);
    short SetTarePriority(AdcState *adcState, const AdcTarePriority prio);
    short ClearTare(AdcState *adcState);
    short SetDisplayText(const AdcDisplayText *text);
    short SetBaseprice(const AdcBasePrice *price, const short frozen = 0);
    short SetScaleMode(const AdcScaleMode mode);
    short SetEeprom(const AdcEeprom *eeprom);
	short SetTiltCompensation(const AdcTiltComp *tiltCompensation, const map<string, string> *tccSettings, const map<string, string> *linSettings, const map<string, string> *wdtaSettings, const AdcSpiritLevelCmd *spiritLevelCmd);
	short SetTiltAngleConfirm(const AdcTiltAngleConfirmCmd cmd);
	short SetScaleSpecificSettingsGeneral(const map<string, string> *scaleSpecificSettings, const map<string, string> *scaleSpecificSettingsSealed);
	short SetFilter(const AdcFilter *filter);
	short SetGFactor(const short gFactor);
	short SetProdSettings(const AdcProdSettings *prodSettings);
	short SetConversionWeightUnit(const long conversionWeightUnitFactor);
	short SetInterfaceMode(const AdcInterfaceMode *interfaceMode);
    short ZeroScale(AdcState *adcState);
    short ReadWeight(const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcTare *tare, AdcBasePrice *basePrice, AdcPrice *sellPrice);
    short GetLogger(const short index, char *entry, unsigned long *size);
    short GetRandomAuthentication(unsigned char *random, unsigned long *size);
    short SetAuthentication(const AdcAuthentication *authentication);
    short GetMaxDisplayTextChars(short *number);
	short GetHighResolution(const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcWeight *weightHighResolution, AdcTare *tare, long *digitValue);
	short GetGrossWeight(AdcState *adcState, AdcWeight *grossWeight, AdcWeight *grossWeightHighResolution, long *digitValue);
	short GetDiagnosticData(const char *name, map<string, AdcSensorHealth> &sensorHealth);
	short ConfigureDiagnosticData(const AdcSensorHealth *sensorHealth);
    short GetBaseprice(AdcBasePrice *price);
    short GetDisplayText(AdcDisplayText *text);
    short GetScaleMode(AdcScaleMode *mode);
    short GetEeprom(AdcEeprom *eeprom);
    short GetTiltCompensation(AdcTiltComp *tiltCompensation);
	short GetFilter(AdcFilter *filter);
    short GetCapabilities(map<short, short> &capMap);
    short SetCountrySettings(const map<string, string> *cySettingsMap);
	short GetCountrySettings(map<string, string> &cySettingsMap);
    short SetLoadCapacity(const map<string, string> *lcSettingsMap);
	short GetLoadCapacity(map<string, string> &cySettingsMap);
    short GetVersion(map<string, string> &verMap);
	short GetGFactor(short *gFactor);
	short GetProdSettings(AdcProdSettings *prodSettings);
	short GetInterfaceMode(AdcInterfaceMode *interfaceMode);
	short Calibration(const AdcCalibCmd cmd, AdcState *adcState, long *step, long *calibDigit);
	short Parameters(const AdcParamMode mode);
	short GetInternalDataEx(AdcInternalDataEx *internalData, bool sendRequestCmd = true);
	short GetEepromSize(AdcEepromSize *eepromSize);
	short SetZeroPointTracking(short mode);
	short GetZeroPointTracking(short *mode);
	short SetVerifParamProtected();
	short GetVerifParamProtected(short *mode);
	short SetUpdateAllowed(short mode);
	short GetUpdateAllowed(short *mode);
	short GetRemainingWarmUpTime(long *seconds);
	short GetWarmUpTime(long *seconds);
	short SetWarmUpTime(long seconds);
	short SetOperatingMode(AdcOperatingMode mode);
	short GetOperatingMode(AdcOperatingMode *mode);
	short SetZeroSettingInterval(long seconds);
	short GetZeroSettingInterval(long *seconds);
	short SetAutomaticZeroSettingTime(long seconds);
	short GetAutomaticZeroSettingTime(long *seconds);
	short SetScaleModel(const string &model);
	short GetScaleModel(string &model);
	short SetStateAutomaticTiltSensor(const bool state);
	short GetStateAutomaticTiltSensor(bool *state);
	short FirmwareUpdate(const AdcFrmCmd cmd, const AdcFrmData *data, const string *usbStackVersion = NULL, const string *welmecStructureVersion = NULL, const short force = 0);
	short SetInitialZeroSetting(const AdcInitialZeroSettingParam *initialZeroSettingParam);

	// country specific strings
	static const short COUNTRY_SETTING_UPDATE;
	static const short COUNTRY_SETTING_CURRENCYSIGN;
	static const short COUNTRY_SETTING_DECIMALSIGN;
	static const short COUNTRY_SETTING_DECIMALPLACES;
	static const short COUNTRY_SETTING_MAXSELLPRICE;
	static const short COUNTRY_SETTING_TAREMODE;
	static const short COUNTRY_SETTING_SPRICEROUND;
	static const short COUNTRY_SETTING_STAPTARA;
	static const short COUNTRY_SETTING_PROZENTTARA;
	static const short COUNTRY_SETTING_ALLGWAAEINST;
	static const short COUNTRY_SETTING_GFAKTOR;
	static const short COUNTRY_SETTING_COUNTRY;
	static const short COUNTRY_SETTING_MAXSTR;
	static const short COUNTRY_SETTING_MINSTR;
	static const short COUNTRY_SETTING_ESTR;
	static const short COUNTRY_SETTING_TCLIMITCS;						// optional
	static const short COUNTRY_SPECIFIC_STRINGS_MANDATORY_COUNT;
	static const short COUNTRY_SPECIFIC_STRINGS_COUNT;
	static const string COUNTRY_SPECIFIC_STRINGS[];

	// load capacity strings
	static const short LOAD_CAPACITY_BEREICH;
	static const short LOAD_CAPACITY_BERUEBERLAUF;
	static const short LOAD_CAPACITY_KOMMA;
	static const short LOAD_CAPACITY_MEHRTEILUNG;
	static const short LOAD_CAPACITY_TEILGRENZE0;
	static const short LOAD_CAPACITY_TEILGRENZE1;
	static const short LOAD_CAPACITY_TEILSCHRITT0;
	static const short LOAD_CAPACITY_TEILSCHRITT1;
	static const short LOAD_CAPACITY_TEILSCHRITT2;
	static const short LOAD_CAPACITY_EINSCHALTNULLBERMI;
	static const short LOAD_CAPACITY_EINSCHALTNULLBERPL;
	static const short LOAD_CAPACITY_NULLSTELLBERMI;
	static const short LOAD_CAPACITY_NULLSTELLBERPL;
	static const short LOAD_CAPACITY_TARABEREICH;
	static const short LOAD_CAPACITY_TARAHANDBEREICH;
	static const short LOAD_CAPACITY_AUTNULLPUNKTKOR;
	static const short LOAD_CAPACITY_TRALACODE;
	static const short LOAD_CAPACITY_GRPRTALOEGREN;
	static const short LOAD_CAPACITY_GENGEW0;
	static const short LOAD_CAPACITY_GENGEW1;
	static const short LOAD_CAPACITY_GENGEW2;
	static const short LOAD_CAPACITY_ANZNEGBRU;
	static const short LOAD_CAPACITY_GEWICHTSEINHEIT;
	static const short LOAD_CAPACITY_ASREGWDHSPERRE;
	static const short LOAD_CAPACITY_ASREGGRENZE;
	static const short LOAD_CAPACITY_ASVERRIEGELWERT;
	static const short LOAD_CAPACITY_SBREGWDHSPERRE;
	static const short LOAD_CAPACITY_SBREGGRENZE;
	static const short LOAD_CAPACITY_SBVERRIEGELWERT;
	static const short LOAD_CAPACITY_PAREGWDHSPERRE;
	static const short LOAD_CAPACITY_PAREGGRENZE;
	static const short LOAD_CAPACITY_PAVERRIEGELWERT;
	static const short LOAD_CAPACITY_AUFLOESUNG;
	static const short LOAD_CAPACITY_STRINGS_MANDATORY_COUNT;
	static const short  LOAD_CAPACITY_STRINGS_COUNT;
	static const string LOAD_CAPACITY_STRINGS[];

	static const short TCC_STRINGS_COUNT;
	static const string TCC_STRINGS[];
	static const short LIN_STRINGS_COUNT;
	static const string LIN_STRINGS[];
	static const short WDTA_STRINGS_COUNT;
	static const string WDTA_STRINGS[];
	static const short WDTA_STRINGS_TO_CONVERT_COUNT;
	static const string WDTA_STRINGS_TO_CONVERT[];
	static const short SSP_STRINGS_COUNT;
	static const string SSP_STRINGS[];
	static const short SSPS_STRINGS_COUNT;
	static const string SSPS_STRINGS[];

private:
    // command identifier
    static const string CMD_SET_TARE;
    static const string CMD_SET_TARE_PRIO;
    static const string CMD_CLEAR_TARE;
    static const string CMD_SET_ZERO;
    static const string CMD_ADC_RESET;
    static const string CMD_READ_WEIGHT;
    static const string CMD_LOGBOOK;
    static const string CMD_GET_RANDOM_AUTH;
    static const string CMD_SET_AUTH;
    static const string CMD_GET_HIGH_RESOLUTION;
	static const string CMD_GET_GROSS_WEIGHT;
    static const string CMD_DIA;
	static const string CMD_GET_TARE;
    static const string CMD_GET_CAPABILITIES;
    static const string CMD_SET_COUNTRY_SETTINGS;
	static const string CMD_GET_COUNTRY_SETTINGS;
    static const string CMD_SET_LC_SETTINGS;
	static const string CMD_GET_LC_SETTINGS;
    static const string CMD_GET_VERSION;
	static const string CMD_CALIB;
	static const string CMD_PARAMETER_MODE;
	static const string CMD_GET_INTERNAL_DATA;
	static const string CMD_CONFIGURE_DIAGNOSTIC;
	static const string CMD_FRM_UPDATE;
    
	static const string CMD_SET_SCALE_VALUE;
    static const string CMD_SET_SCALE_VALUE_BP;
    static const string CMD_SET_SCALE_VALUE_DT;
    static const string CMD_SET_SCALE_VALUE_SM;
    static const string CMD_SET_SCALE_VALUE_EE;
    static const string CMD_SET_SCALE_VALUE_TC;
	static const string CMD_SET_SCALE_VALUE_FILTER;
	static const string CMD_SET_SCALE_VALUE_GFACTOR;
	static const string CMD_SET_SCALE_VALUE_PROD_SETTINGS;
	static const string CMD_SET_SCALE_VALUE_CONVERSION_WEIGHT_UNIT;
	static const string CMD_SET_SCALE_VALUE_INTERFACE_MODE;
	static const string CMD_SET_ZEROSETTING_ZEROTRACKING_CONFIG;

	static const string CMD_GET_SCALE_VALUE;
	static const string CMD_GET_SCALE_VALUE_BP;
    static const string CMD_GET_SCALE_VALUE_DT;
    static const string CMD_GET_SCALE_VALUE_SM;
    static const string CMD_GET_SCALE_VALUE_EE;
    static const string CMD_GET_SCALE_VALUE_TC;
    static const string CMD_GET_SCALE_VALUE_MAX_DT;
	static const string CMD_GET_SCALE_VALUE_FILTER;
	static const string CMD_GET_SCALE_VALUE_EE_SIZE;
	static const string CMD_GET_SCALE_VALUE_GFACTOR;
	static const string CMD_GET_SCALE_VALUE_PROD_SETTINGS;
	static const string CMD_GET_SCALE_VALUE_INTERFACE_MODE;
	static const string CMD_GET_ZEROSETTING_ZEROTRACKING_CONFIG;

    // reference data identifier
    static const short  REF_DATA_ID_TARE;
    static const string REF_DATA_ID_TARE_STR;

    static const short  REF_DATA_ID_TARE_PRIO;
    static const string REF_DATA_ID_TARE_PRIO_STR;

    static const short  REF_DATA_ID_BASEPRICE;
    static const string REF_DATA_ID_BASEPRICE_STR;

    static const short  REF_DATA_ID_DISPLAYTEXT;
    static const string REF_DATA_ID_DISPLAYTEXT_STR;

    static const short  REF_DATA_ID_SCALEMODE;
    static const string REF_DATA_ID_SCALEMODE_STR;

    static const short  REF_DATA_ID_EEPROM;
    static const string REF_DATA_ID_EEPROM_STR;

	static const short  REF_DATA_ID_EEPROM_WELMEC_SIZE;
	static const string REF_DATA_ID_EEPROM_WELMEC_SIZE_STR;

	static const short  REF_DATA_ID_EEPROM_OPEN_SIZE;
	static const string REF_DATA_ID_EEPROM_OPEN_SIZE_STR;

	static const short  REF_DATA_ID_EEPROM_PROD_SIZE;
	static const string REF_DATA_ID_EEPROM_PROD_SIZE_STR;

	static const short  REF_DATA_ID_EEPROM_PROD_SENSORS_SIZE;
	static const string REF_DATA_ID_EEPROM_PROD_SENSORS_SIZE_STR;

    static const short  REF_DATA_ID_TILT_COMP;
    static const string REF_DATA_ID_TILT_COMP_STR;

    static const short  REF_DATA_ID_REGISTRATION_REQ;
    static const string REF_DATA_ID_REGISTRATION_REQ_STR;

    static const short  REF_DATA_ID_LOGBOOK;
    static const string REF_DATA_ID_LOGBOOK_STR;

    static const short  REF_DATA_ID_AUTHENTICATION;
    static const string REF_DATA_ID_AUTHENTICATION_STR;

    static const short  REF_DATA_ID_DIAGNOSTIC;

    static const short  REF_DATA_ID_STAT;
    static const string REF_DATA_ID_STAT_STR;

    static const short  REF_DATA_ID_LCSTATE;
    static const string REF_DATA_ID_LCSTATE_STR;

    static const short  REF_DATA_ID_WEIGHT;
    static const string REF_DATA_ID_WEIGHT_STR;
    static const string REF_DATA_ID_HIGHWEIGHT_STR;
	static const string REF_DATA_ID_GROSSWEIGHT_STR;
	static const string REF_DATA_ID_HIGHGROSSWEIGHT_STR;

    static const short  REF_DATA_ID_SELLPRICE;
    static const string REF_DATA_ID_SELLPRICE_STR;

    static const short  REF_DATA_ID_RAW_VALUE;
    static const string REF_DATA_ID_RAW_VALUE_STR;

    static const short  REF_DATA_ID_MAX_DISPL_CHAR;
    static const string REF_DATA_ID_MAX_DISPL_CHAR_STR;

    static const short  REF_DATA_ID_RANDOM_AUTH;
    static const string REF_DATA_ID_RANDOM_AUTH_STR;

	static const short  REF_DATA_ID_CONST_BASEPRICE;
	static const string REF_DATA_ID_CONST_BASEPRICE_STR;

	static const short  REF_DATA_ID_CONST_TARE;
	static const string REF_DATA_ID_CONST_TARE_STR;

	static const short  REF_DATA_ID_COUNTRY_SETTINGS;
	static const string REF_DATA_ID_COUNTRY_SETTINGS_STR;

	static const short  REF_DATA_ID_LOAD_CAPACITY_SETTINGS;
	static const string REF_DATA_ID_LOAD_CAPACITY_SETTINGS_STR;

	static const short  REF_DATA_ID_CALIBRATION;
	static const string REF_DATA_ID_CALIBRATION_STR;

	static const short  REF_DATA_ID_CALIB_STEP;
	static const string REF_DATA_ID_CALIB_STEP_STR;

	static const short  REF_DATA_ID_FILTER_IDX;
	static const string REF_DATA_ID_FILTER_IDX_STR;

	static const short  REF_DATA_ID_OFFSET_STABLE_TIME;
	static const string REF_DATA_ID_OFFSET_STABLE_TIME_STR;

	static const short  REF_DATA_ID_STABLE_RANGE;
	static const string REF_DATA_ID_STABLE_RANGE_STR;

	static const short  REF_DATA_ID_PARAM_MODE;
	static const string REF_DATA_ID_PARAM_MODE_STR;

	static const short	 REF_DATA_ID_SENSOR_HEALTH;

	static const short  REF_DATA_ID_RESET_TYPE;
	static const string REF_DATA_ID_RESET_TYPE_STR;

	static const short	REF_DATA_ID_CONFIG_DIAGNOSTIC;

	static const short  REF_DATA_ID_GFACTOR;
	static const string REF_DATA_ID_GFACTOR_STR;

	static const short  REF_DATA_ID_PROD_SITE;
	static const string REF_DATA_ID_PROD_SITE_STR;

	static const short  REF_DATA_ID_PROD_GFACTOR_OFFSET;
	static const string REF_DATA_ID_PROD_GFACTOR_OFFSET_STR;
	
	static const short  REF_DATA_ID_PROD_DATE;
	static const string REF_DATA_ID_PROD_DATE_STR;

	static const short  REF_DATA_ID_PROD_WSC;
	static const string REF_DATA_ID_PROD_WSC_STR;

	static const short  REF_DATA_ID_CONVERSION_WEIGHT_UNIT;
	static const string REF_DATA_ID_CONVERSION_WEIGHT_UNIT_STR;

	static const short  REF_DATA_ID_INTERFACE_AUTO_DETECTION;
	static const string REF_DATA_ID_INTERFACE_AUTO_DETECTION_STR;

	static const short  REF_DATA_ID_INTERFACE_MODE;
	static const string REF_DATA_ID_INTERFACE_MODE_STR;

	static const short  REF_DATA_ID_ZERO_POINT_TRACKING;
	static const string REF_DATA_ID_ZERO_POINT_TRACKING_STR;

	static const short  REF_DATA_ID_ZERO_SETTING_INTERVAL;
	static const string REF_DATA_ID_ZERO_SETTING_INTERVAL_STR;

	static const short  REF_DATA_ID_AUTOMATIC_ZERO_SETTING_TIME;
	static const string REF_DATA_ID_AUTOMATIC_ZERO_SETTING_TIME_STR;

	static const short  REF_DATA_ID_VERIF_PARAM_PROTECTED;
	static const string REF_DATA_ID_VERIF_PARAM_PROTECTED_STR;

	static const short  REF_DATA_ID_UPDATE_ALLOWED;
	static const string REF_DATA_ID_UPDATE_ALLOWED_STR;

	static const short  REF_DATA_ID_REMAINING_WARM_UP_TIME;
	static const string REF_DATA_ID_REMAINING_WARM_UP_TIME_STR;

	static const short  REF_DATA_ID_WARM_UP_TIME;
	static const string REF_DATA_ID_WARM_UP_TIME_STR;

	static const short  REF_DATA_ID_OPERATING_MODE;
	static const string REF_DATA_ID_OPERATING_MODE_STR;

	static const short  REF_DATA_ID_FRM_COMMAND;
	static const string REF_DATA_ID_FRM_COMMAND_STR;

	static const short  REF_DATA_ID_FRM_DATA;
	static const string REF_DATA_ID_FRM_DATA_STR;

	static const short  REF_DATA_ID_FRM_USBSTACK_VERS;
	static const string REF_DATA_ID_FRM_USBSTACK_VERS_STR;

	static const short  REF_DATA_ID_FRM_WELMECSTRUCT_VERS;
	static const string REF_DATA_ID_FRM_WELMECSTRUCT_VERS_STR;

	static const short  REF_DATA_ID_FRM_FORCE;
	static const string REF_DATA_ID_FRM_FORCE_STR;

	static const short  REF_DATA_ID_FRM_DATE;
	static const string REF_DATA_ID_FRM_DATE_STR;

	static const short  REF_DATA_ID_TILT_TCC;
	static const string REF_DATA_ID_TILT_TCC_STR;

	static const short  REF_DATA_ID_TILT_LIN;
	static const string REF_DATA_ID_TILT_LIN_STR;

	static const short  REF_DATA_ID_TILT_WDTA;
	static const string REF_DATA_ID_TILT_WDTA_STR;

	static const short	REF_DATA_ID_SPIRIT_LEVEL;
	static const string REF_DATA_ID_SPIRIT_LEVEL_STR;

	static const short  REF_DATA_ID_TARE_TYPE;
	static const string REF_DATA_ID_TARE_TYPE_STR;

	static const short  REF_DATA_ID_DIGIT_VALUE;
	static const string REF_DATA_ID_DIGIT_VALUE_STR;

	static const short  REF_DATA_ID_CALIB_DIGIT_VALUE;
	static const string REF_DATA_ID_CALIB_DIGIT_VALUE_STR;

	static const short  REF_DATA_ID_INITIAL_ZERO_SETTING;
	static const string REF_DATA_ID_INITIAL_ZERO_SETTING_STR;

	static const short  REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL;
	static const string REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL_STR;

	static const short  REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL_SEALED;
	static const string REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL_SEALED_STR;

	static const short  REF_DATA_ID_SCALE_MODEL;
	static const string REF_DATA_ID_SCALE_MODEL_STR;

	static const short  REF_DATA_ID_STATE_AUTOMATIC_TILT_SENSOR;
	static const string REF_DATA_ID_STATE_AUTOMATIC_TILT_SENSOR_STR;

	static const short  REF_DATA_ID_TILT_COMP_CHECKED;
	static const string REF_DATA_ID_TILT_COMP_CHECKED_STR;
	
    // header identifier
    static const string HEADER_LENGTH;
    static const string HEADER_ORDERID;
    static const string HEADER_CMD;
	static const string HEADER_CRC16;

    // defines for parsing a structure
    static const string STRUCT_POS;


    // defines for state machine receive
    static const short  WAIT_FOR_SOH					= 0;
    static const short  WAIT_FOR_LENGTH					= 1;
    static const short  READ_REMAINING_DATA				= 2;
	
    static const short  TIMEOUT_RECEIVE     = 2;
	static const short  TIMEOUT_RETRIES_ATTEMPTS = 2;
	
    // defines for state machine
    static const short  CHECK_FOR_SOH   = 0;
    static const short  PARSE_HEADER    = 1;
    static const short  PARSE_COMMAND   = 2;
    static const short  CHECK_FOR_STX   = 3;
    static const short  PARSE_KEY       = 4;
    static const short  PARSE_VALUE     = 5;
	static const short  PARSE_CHECKSUM  = 6;
    static const short  CHECK_FOR_ETB   = 7;

	// defines for adc stat error
	static const short	ADC_CMD_SUCCESSFUL = 0;
	static const short	ADC_CMD_NOT_EXECUTED = 1;
	static const short	ADC_INVALID_PARAMETER = 2;
	static const short	ADC_FUNCTION_NOT_IMPLEMENTED = 3;
	static const short	ADC_ERROR_AUTHENTICATION = 4;
	static const short	ADC_COMMAND_NOT_SUPPORTED_IN_THIS_OPERATING_MODE = 5;
	static const short	ADC_LOGBOOK_NO_FURTHER_ENTRY = 6;
	static const short	ADC_LOGBOOK_FULL = 7;
	static const short	ADC_ERROR_TILT_COMPENSATION_SWITCH_ON = 8;
	static const short	ADC_ERROR_CHKSUM_BLOCK_3_4 = 9;
	static const short	ADC_ERROR_PROTOCOL_CRC = 10;
	static const short	ADC_ERROR_LINEAR_CALIBRATION = 11;
	static const short	ADC_ERROR_EEPROM_ACCESS_VIOLATION = 12;
	static const short	ADC_ERROR_CMD_ONLY_FOR_SEPARATE_LOADCELL = 13;
	static const short	ADC_ERROR_UPDATE_NOT_ALLOWED = 14;
	static const short	ADC_ERROR_TCC_SWITCH_ON = 15;
	static const short	ADC_ERROR_INCOMPATIBLE_PROD_DATA = 16;
	static const short	ADC_ERROR_TARE_OCCUPIED = 17;
	static const short	ADC_ERROR_KNOWN_TARE_LESS_E = 18;			// known tare is less than the verification scale interval e
	static const short	ADC_ERROR_BATCH_TARE_NOT_ALLOWED = 19;
	static const short	ADC_ERROR_TARE_OUT_OF_RANGE = 20;			// tare could not be set because greater limit or gross weight < 0
	static const short	ADC_ERROR_TARE_OUTSIDE_CLEARING_AREA = 21;	// tare could not be cleared because tare value is not identical to the negative weight value
	static const short	ADC_ERROR_WS_NOT_SUPPORT_LOAD_CAPACITY = 22;	// weighing system does not support the load capacity
	static const short  ADC_ERROR_CHKSUM_FIRMWARE = 23;				// wrong firmware checksum

	// defines for flag
	static const unsigned long FLAG_TELEGRAM_REPEAT = 0x00000001;

	void	Init();
    void    CreateReferenceData(const RefDataStruct &refData, keyValuePair &output);
    short   GetReferenceData(const string &masterKey, const keyValuePair &refDataMap, RefDataStruct &refData, short sollDefDataReceived);
	short   CreateTelegram(const string &cmd, const keyValuePair &refDataMap, string &telegram, bool crc16, bool createNewOrderID, short previousOrderID, unsigned long flag);
    short   GetOrderID();
	short   SendRequestReceiveResponse(const string &cmd, keyValuePair &keyValueMap, bool sendRequestCmd = true);
	short   ReceiveResponse(string &response, short timeoutOffset = 0);
    short   GetKeyValuePair(ProtocolType type, const string &keyValueStr, keyValuePair &headerMap, keyValuePair &refDataMap);
    void    ParseStructure(const string &structStr, keyValuePair &structMap);
    short   GetDebugResponse(const string &cmd, short orderID, string &response);
	unsigned short CalculateChecksum(string str);
	short	CheckChecksum(string str, string crc);
	bool	CheckErrorCode(short errorCode, bool ignoreADCError = false);
	string  ConvertFloatIEEToInt(string value);
	bool	ConvertDegreeToDigits(keyValuePair *wdtaSettings);

    short               m_orderID;
    static const short  m_base = 10;         // Kodierung Dezimal
};

