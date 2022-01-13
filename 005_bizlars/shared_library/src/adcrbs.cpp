/**
******************************************************************************
* File       : adcrbs.cpp
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcrbs class: impement the protocol retail bus sensoric
******************************************************************************
*/
#include <sstream>
#include <iomanip>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <math.h>		  // sin()
#include "helpers.h"

// header for cryptopp
#include <filters.h>
using CryptoPP::StringSink;
using CryptoPP::StringSource;

#include <hex.h>
using CryptoPP::HexEncoder;
using CryptoPP::HexDecoder;

#ifdef  __GNUC__
#include <string.h>
#endif
#include "larsErr.h"
#include "helpers.h"
#include "adcrbs.h"
#include "lars.h"
#include "adctrace.h"
#include "authentication.h"

#define PI 3.14159265

// defines for RBS CRC16
#define RBS_CRC_START      0x6621      /* start value for the Polynomial */
#define RBS_CRC_POLY       0x7102      /* Polynomial-coeffizient */
#define RBS_CRC_MASK       0x0000      /* Mask for the Polynomial */


// command identifier
const string AdcRbs::CMD_CALIB					= "CAL";
const string AdcRbs::CMD_GET_CAPABILITIES		= "CAP";
const string AdcRbs::CMD_CLEAR_TARE				= "CT";
const string AdcRbs::CMD_DIA					= "DIA";
const string AdcRbs::CMD_GET_RANDOM_AUTH		= "GA";
const string AdcRbs::CMD_GET_HIGH_RESOLUTION	= "GHR";
const string AdcRbs::CMD_GET_GROSS_WEIGHT		= "GGW";
const string AdcRbs::CMD_LOGBOOK				= "LOG";
const string AdcRbs::CMD_SET_TARE				= "ST";
const string AdcRbs::CMD_GET_TARE				= "GT";
const string AdcRbs::CMD_ADC_RESET				= "RST";
const string AdcRbs::CMD_READ_WEIGHT			= "RW";
const string AdcRbs::CMD_SET_AUTH				= "SA";
const string AdcRbs::CMD_SET_COUNTRY_SETTINGS	= "SCTR";
const string AdcRbs::CMD_GET_COUNTRY_SETTINGS   = "GCTR";
const string AdcRbs::CMD_SET_LC_SETTINGS		= "SLC";
const string AdcRbs::CMD_GET_LC_SETTINGS		= "GLC";
const string AdcRbs::CMD_PARAMETER_MODE			= "AP";
const string AdcRbs::CMD_SET_TARE_PRIO			= "STP";
const string AdcRbs::CMD_SET_ZERO				= "SZ";
const string AdcRbs::CMD_GET_VERSION			= "VERS";
const string AdcRbs::CMD_GET_INTERNAL_DATA		= "INTD";
const string AdcRbs::CMD_CONFIGURE_DIAGNOSTIC	= "DIAC";
const string AdcRbs::CMD_FRM_UPDATE				= "FRM";

const string AdcRbs::CMD_SET_SCALE_VALUE = "SSV";
const string AdcRbs::CMD_SET_SCALE_VALUE_CONVERSION_WEIGHT_UNIT = "SSVb";
const string AdcRbs::CMD_SET_SCALE_VALUE_TC = "SSVc";
const string AdcRbs::CMD_SET_ZEROSETTING_ZEROTRACKING_CONFIG = "SSVd";
const string AdcRbs::CMD_SET_SCALE_VALUE_EE = "SSVe";
const string AdcRbs::CMD_SET_SCALE_VALUE_FILTER = "SSVf";
const string AdcRbs::CMD_SET_SCALE_VALUE_GFACTOR = "SSVg";
const string AdcRbs::CMD_SET_SCALE_VALUE_INTERFACE_MODE = "SSVi";
const string AdcRbs::CMD_SET_SCALE_VALUE_PROD_SETTINGS = "SSVl";
const string AdcRbs::CMD_SET_SCALE_VALUE_SM = "SSVm";
const string AdcRbs::CMD_SET_SCALE_VALUE_BP = "SSVp";
const string AdcRbs::CMD_SET_SCALE_VALUE_DT = "SSVt";

const string AdcRbs::CMD_GET_SCALE_VALUE = "GSV";
const string AdcRbs::CMD_GET_SCALE_VALUE_TC = "GSVc";
const string AdcRbs::CMD_GET_ZEROSETTING_ZEROTRACKING_CONFIG = "GSVd";
const string AdcRbs::CMD_GET_SCALE_VALUE_EE = "GSVe";
const string AdcRbs::CMD_GET_SCALE_VALUE_FILTER = "GSVf";
const string AdcRbs::CMD_GET_SCALE_VALUE_GFACTOR = "GSVg";
const string AdcRbs::CMD_GET_SCALE_VALUE_INTERFACE_MODE = "GSVi";
const string AdcRbs::CMD_GET_SCALE_VALUE_PROD_SETTINGS = "GSVl";
const string AdcRbs::CMD_GET_SCALE_VALUE_SM = "GSVm";
const string AdcRbs::CMD_GET_SCALE_VALUE_MAX_DT = "GSVn";
const string AdcRbs::CMD_GET_SCALE_VALUE_BP = "GSVp";
const string AdcRbs::CMD_GET_SCALE_VALUE_EE_SIZE = "GSVs";
const string AdcRbs::CMD_GET_SCALE_VALUE_DT = "GSVt";

// referenc data identifier
const short  AdcRbs::REF_DATA_ID_TARE                   = 0;
const string AdcRbs::REF_DATA_ID_TARE_STR               = "tare";

const short  AdcRbs::REF_DATA_ID_TARE_PRIO              = 1;
const string AdcRbs::REF_DATA_ID_TARE_PRIO_STR          = "tp";

const short  AdcRbs::REF_DATA_ID_BASEPRICE              = 2;
const string AdcRbs::REF_DATA_ID_BASEPRICE_STR          = "bp";

const short  AdcRbs::REF_DATA_ID_DISPLAYTEXT            = 3;
const string AdcRbs::REF_DATA_ID_DISPLAYTEXT_STR        = "dt";

const short  AdcRbs::REF_DATA_ID_SCALEMODE              = 4;
const string AdcRbs::REF_DATA_ID_SCALEMODE_STR          = "m";

const short  AdcRbs::REF_DATA_ID_EEPROM                 = 5;
const string AdcRbs::REF_DATA_ID_EEPROM_STR             = "ee";

const short  AdcRbs::REF_DATA_ID_EEPROM_WELMEC_SIZE		= 6;
const string AdcRbs::REF_DATA_ID_EEPROM_WELMEC_SIZE_STR	= "eew";

const short  AdcRbs::REF_DATA_ID_EEPROM_OPEN_SIZE		= 7;
const string AdcRbs::REF_DATA_ID_EEPROM_OPEN_SIZE_STR	= "ees";

const short  AdcRbs::REF_DATA_ID_TILT_COMP              = 8;
const string AdcRbs::REF_DATA_ID_TILT_COMP_STR          = "tc";

const short  AdcRbs::REF_DATA_ID_REGISTRATION_REQ       = 9;
const string AdcRbs::REF_DATA_ID_REGISTRATION_REQ_STR   = "rr";

const short  AdcRbs::REF_DATA_ID_LOGBOOK                = 10;
const string AdcRbs::REF_DATA_ID_LOGBOOK_STR            = "idx";

const short  AdcRbs::REF_DATA_ID_AUTHENTICATION         = 11;
const string AdcRbs::REF_DATA_ID_AUTHENTICATION_STR     = "chk";

const short  AdcRbs::REF_DATA_ID_DIAGNOSTIC             = 12;

const short  AdcRbs::REF_DATA_ID_STAT                   = 13;
const string AdcRbs::REF_DATA_ID_STAT_STR               = "stat";

const short  AdcRbs::REF_DATA_ID_LCSTATE                = 14;
const string AdcRbs::REF_DATA_ID_LCSTATE_STR            = "lcs";

const short  AdcRbs::REF_DATA_ID_WEIGHT                 = 15;
const string AdcRbs::REF_DATA_ID_WEIGHT_STR             = "wt";
const string AdcRbs::REF_DATA_ID_HIGHWEIGHT_STR         = "wth";
const string AdcRbs::REF_DATA_ID_GROSSWEIGHT_STR		= "gw";
const string AdcRbs::REF_DATA_ID_HIGHGROSSWEIGHT_STR	= "gwh";

const short  AdcRbs::REF_DATA_ID_SELLPRICE              = 16;
const string AdcRbs::REF_DATA_ID_SELLPRICE_STR          = "sp";

const short  AdcRbs::REF_DATA_ID_RAW_VALUE				= 17;
const string AdcRbs::REF_DATA_ID_RAW_VALUE_STR			= "raw";

const short  AdcRbs::REF_DATA_ID_MAX_DISPL_CHAR         = 18;
const string AdcRbs::REF_DATA_ID_MAX_DISPL_CHAR_STR     = "ndt";

const short  AdcRbs::REF_DATA_ID_RANDOM_AUTH            = 19;
const string AdcRbs::REF_DATA_ID_RANDOM_AUTH_STR        = "no";

const short  AdcRbs::REF_DATA_ID_CONST_BASEPRICE		= 20;
const string AdcRbs::REF_DATA_ID_CONST_BASEPRICE_STR	= "conb";

const short  AdcRbs::REF_DATA_ID_CONST_TARE				= 21;
const string AdcRbs::REF_DATA_ID_CONST_TARE_STR			= "cont";

const short  AdcRbs::REF_DATA_ID_COUNTRY_SETTINGS		= 22;
const string AdcRbs::REF_DATA_ID_COUNTRY_SETTINGS_STR	= "css";

const short  AdcRbs::REF_DATA_ID_LOAD_CAPACITY_SETTINGS = 23;
const string AdcRbs::REF_DATA_ID_LOAD_CAPACITY_SETTINGS_STR = "lcp";

const short  AdcRbs::REF_DATA_ID_CALIBRATION			= 24;
const string AdcRbs::REF_DATA_ID_CALIBRATION_STR		= "cmd";

const short  AdcRbs::REF_DATA_ID_CALIB_STEP				= 25;
const string AdcRbs::REF_DATA_ID_CALIB_STEP_STR			= "step";

const short  AdcRbs::REF_DATA_ID_FILTER_IDX				= 26;
const string AdcRbs::REF_DATA_ID_FILTER_IDX_STR			= "idx";

const short  AdcRbs::REF_DATA_ID_OFFSET_STABLE_TIME		= 27;
const string AdcRbs::REF_DATA_ID_OFFSET_STABLE_TIME_STR = "rstt";

const short  AdcRbs::REF_DATA_ID_STABLE_RANGE			= 28;
const string AdcRbs::REF_DATA_ID_STABLE_RANGE_STR		= "rstr";

const short  AdcRbs::REF_DATA_ID_PARAM_MODE				= 29;
const string AdcRbs::REF_DATA_ID_PARAM_MODE_STR			= "mode";

const short	 AdcRbs::REF_DATA_ID_SENSOR_HEALTH			= 30;

const short  AdcRbs::REF_DATA_ID_RESET_TYPE				= 31;
const string AdcRbs::REF_DATA_ID_RESET_TYPE_STR			= "type";

const short AdcRbs::REF_DATA_ID_CONFIG_DIAGNOSTIC		= 32;

const short  AdcRbs::REF_DATA_ID_GFACTOR				= 33;
const string AdcRbs::REF_DATA_ID_GFACTOR_STR			= "gfac";

const short  AdcRbs::REF_DATA_ID_PROD_SITE				= 34;
const string AdcRbs::REF_DATA_ID_PROD_SITE_STR			= "pdp";

const short  AdcRbs::REF_DATA_ID_PROD_GFACTOR_OFFSET	= 35;
const string AdcRbs::REF_DATA_ID_PROD_GFACTOR_OFFSET_STR = "gofs";

const short  AdcRbs::REF_DATA_ID_PROD_DATE				= 36;
const string AdcRbs::REF_DATA_ID_PROD_DATE_STR			= "date";

const short  AdcRbs::REF_DATA_ID_PROD_WSC				= 37;
const string AdcRbs::REF_DATA_ID_PROD_WSC_STR			= "wsc";

const short  AdcRbs::REF_DATA_ID_CONVERSION_WEIGHT_UNIT = 38;
const string AdcRbs::REF_DATA_ID_CONVERSION_WEIGHT_UNIT_STR = "fac";

const short  AdcRbs::REF_DATA_ID_INTERFACE_AUTO_DETECTION		= 39;
const string AdcRbs::REF_DATA_ID_INTERFACE_AUTO_DETECTION_STR	= "ad";

const short  AdcRbs::REF_DATA_ID_INTERFACE_MODE			= 40;
const string AdcRbs::REF_DATA_ID_INTERFACE_MODE_STR		= "mode";

const short  AdcRbs::REF_DATA_ID_ZERO_POINT_TRACKING    = 41;
const string AdcRbs::REF_DATA_ID_ZERO_POINT_TRACKING_STR = "zpt";

const short  AdcRbs::REF_DATA_ID_ZERO_SETTING_INTERVAL	= 42;
const string AdcRbs::REF_DATA_ID_ZERO_SETTING_INTERVAL_STR = "zsi";

const short  AdcRbs::REF_DATA_ID_AUTOMATIC_ZERO_SETTING_TIME		= 43;
const string AdcRbs::REF_DATA_ID_AUTOMATIC_ZERO_SETTING_TIME_STR	= "azst";

const short  AdcRbs::REF_DATA_ID_VERIF_PARAM_PROTECTED	= 44;
const string AdcRbs::REF_DATA_ID_VERIF_PARAM_PROTECTED_STR = "vpp";

const short  AdcRbs::REF_DATA_ID_UPDATE_ALLOWED			= 45;
const string AdcRbs::REF_DATA_ID_UPDATE_ALLOWED_STR		= "upd";

const short  AdcRbs::REF_DATA_ID_REMAINING_WARM_UP_TIME		= 46;
const string AdcRbs::REF_DATA_ID_REMAINING_WARM_UP_TIME_STR	= "rwut";

const short  AdcRbs::REF_DATA_ID_WARM_UP_TIME			= 47;
const string AdcRbs::REF_DATA_ID_WARM_UP_TIME_STR		= "wut";

const short  AdcRbs::REF_DATA_ID_OPERATING_MODE			= 48;
const string AdcRbs::REF_DATA_ID_OPERATING_MODE_STR		= "bm";

const short  AdcRbs::REF_DATA_ID_FRM_COMMAND			= 49;
const string AdcRbs::REF_DATA_ID_FRM_COMMAND_STR		= "cmd";

const short  AdcRbs::REF_DATA_ID_FRM_DATA				= 50;
const string AdcRbs::REF_DATA_ID_FRM_DATA_STR			= "data";

const short  AdcRbs::REF_DATA_ID_FRM_USBSTACK_VERS		= 51;
const string AdcRbs::REF_DATA_ID_FRM_USBSTACK_VERS_STR = "usbv";

const short  AdcRbs::REF_DATA_ID_FRM_WELMECSTRUCT_VERS	= 52;
const string AdcRbs::REF_DATA_ID_FRM_WELMECSTRUCT_VERS_STR = "welv";

const short  AdcRbs::REF_DATA_ID_FRM_FORCE				= 53;
const string AdcRbs::REF_DATA_ID_FRM_FORCE_STR			= "fcon";

const short  AdcRbs::REF_DATA_ID_FRM_DATE				= 54;
const string AdcRbs::REF_DATA_ID_FRM_DATE_STR			= "date";

const short  AdcRbs::REF_DATA_ID_TILT_TCC				= 55;
const string AdcRbs::REF_DATA_ID_TILT_TCC_STR			= "tcc";

const short  AdcRbs::REF_DATA_ID_TILT_LIN				= 56;
const string AdcRbs::REF_DATA_ID_TILT_LIN_STR			= "lin";

const short  AdcRbs::REF_DATA_ID_TILT_WDTA				= 57;
const string AdcRbs::REF_DATA_ID_TILT_WDTA_STR			= "wdta";

const short  AdcRbs::REF_DATA_ID_SPIRIT_LEVEL			= 58;
const string AdcRbs::REF_DATA_ID_SPIRIT_LEVEL_STR		= "slc";

const short  AdcRbs::REF_DATA_ID_EEPROM_PROD_SIZE		= 59;
const string AdcRbs::REF_DATA_ID_EEPROM_PROD_SIZE_STR	= "eep";

const short  AdcRbs::REF_DATA_ID_TARE_TYPE				= 60;
const string AdcRbs::REF_DATA_ID_TARE_TYPE_STR			= "type";

const short  AdcRbs::REF_DATA_ID_DIGIT_VALUE			= 61;
const string AdcRbs::REF_DATA_ID_DIGIT_VALUE_STR		= "digi";

const short  AdcRbs::REF_DATA_ID_CALIB_DIGIT_VALUE		= 62;
const string AdcRbs::REF_DATA_ID_CALIB_DIGIT_VALUE_STR = "cdig";

const short  AdcRbs::REF_DATA_ID_INITIAL_ZERO_SETTING		= 63;
const string AdcRbs::REF_DATA_ID_INITIAL_ZERO_SETTING_STR	= "izs";

const short  AdcRbs::REF_DATA_ID_EEPROM_PROD_SENSORS_SIZE		= 64;
const string AdcRbs::REF_DATA_ID_EEPROM_PROD_SENSORS_SIZE_STR	= "eeps";

const short  AdcRbs::REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL		= 65;
const string AdcRbs::REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL_STR	= "ssp";

const short  AdcRbs::REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL_SEALED		= 66;
const string AdcRbs::REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL_SEALED_STR = "ssps";

const short  AdcRbs::REF_DATA_ID_SCALE_MODEL			= 67;
const string AdcRbs::REF_DATA_ID_SCALE_MODEL_STR		= "sm";

const short  AdcRbs::REF_DATA_ID_STATE_AUTOMATIC_TILT_SENSOR = 68;
const string AdcRbs::REF_DATA_ID_STATE_AUTOMATIC_TILT_SENSOR_STR = "ats";

const short  AdcRbs::REF_DATA_ID_TILT_COMP_CHECKED		= 69;
const string AdcRbs::REF_DATA_ID_TILT_COMP_CHECKED_STR	= "tcp";

// header identifier
const string AdcRbs::HEADER_LENGTH                      = "hdr0";
const string AdcRbs::HEADER_ORDERID                     = "hdr1";
const string AdcRbs::HEADER_CMD                         = "cmd";
const string AdcRbs::HEADER_CRC16						= "crc16";

const string AdcRbs::STRUCT_POS                         = "pos";

// country specific strings, add new strings always on end
const short AdcRbs::COUNTRY_SETTING_TAREMODE = 0;
const short AdcRbs::COUNTRY_SETTING_STAPTARA = 1;
const short AdcRbs::COUNTRY_SETTING_PROZENTTARA = 2;
const short AdcRbs::COUNTRY_SETTING_ALLGWAAEINST = 3;
const short AdcRbs::COUNTRY_SETTING_GFAKTOR = 4;
const short AdcRbs::COUNTRY_SETTING_COUNTRY = 5;
const short AdcRbs::COUNTRY_SETTING_MAXSTR = 6;
const short AdcRbs::COUNTRY_SETTING_MINSTR = 7;
const short AdcRbs::COUNTRY_SETTING_ESTR = 8;
const short AdcRbs::COUNTRY_SETTING_TCLIMITCS = 9;					// optional
const short AdcRbs::COUNTRY_SPECIFIC_STRINGS_MANDATORY_COUNT = 9;
const short AdcRbs::COUNTRY_SPECIFIC_STRINGS_COUNT = 10;
const string AdcRbs::COUNTRY_SPECIFIC_STRINGS[COUNTRY_SPECIFIC_STRINGS_COUNT] = {	"taremode|INT",
																					"stapTara|INT",
																					"prozentTara|INT",
																					"allgWaaEinst|INT",
																					"gFaktor|INT",
																					"country|STR",
																					"MaxStr|STR",
																					"MinStr|STR",
																					"eStr|STR",
																					"tclimitcs|INT" };

// load capacity, add new strings always on end
const short AdcRbs::LOAD_CAPACITY_BEREICH = 0;
const short AdcRbs::LOAD_CAPACITY_BERUEBERLAUF = 1;
const short AdcRbs::LOAD_CAPACITY_KOMMA = 2;
const short AdcRbs::LOAD_CAPACITY_MEHRTEILUNG = 3;
const short AdcRbs::LOAD_CAPACITY_TEILGRENZE0 = 4;
const short AdcRbs::LOAD_CAPACITY_TEILGRENZE1 = 5;
const short AdcRbs::LOAD_CAPACITY_TEILSCHRITT0 = 6;
const short AdcRbs::LOAD_CAPACITY_TEILSCHRITT1 = 7;
const short AdcRbs::LOAD_CAPACITY_TEILSCHRITT2 = 8;
const short AdcRbs::LOAD_CAPACITY_EINSCHALTNULLBERMI = 9;
const short AdcRbs::LOAD_CAPACITY_EINSCHALTNULLBERPL = 10;
const short AdcRbs::LOAD_CAPACITY_NULLSTELLBERMI = 11;
const short AdcRbs::LOAD_CAPACITY_NULLSTELLBERPL = 12;
const short AdcRbs::LOAD_CAPACITY_TARABEREICH = 13;
const short AdcRbs::LOAD_CAPACITY_TARAHANDBEREICH = 14;
const short AdcRbs::LOAD_CAPACITY_AUTNULLPUNKTKOR = 15;
const short AdcRbs::LOAD_CAPACITY_TRALACODE = 16;
const short AdcRbs::LOAD_CAPACITY_GENGEW0 = 17;
const short AdcRbs::LOAD_CAPACITY_GENGEW1 = 18;
const short AdcRbs::LOAD_CAPACITY_GENGEW2 = 19;
const short AdcRbs::LOAD_CAPACITY_ANZNEGBRU = 20;
const short AdcRbs::LOAD_CAPACITY_GEWICHTSEINHEIT = 21;
const short AdcRbs::LOAD_CAPACITY_ASREGWDHSPERRE = 22;
const short AdcRbs::LOAD_CAPACITY_ASREGGRENZE = 23;
const short AdcRbs::LOAD_CAPACITY_ASVERRIEGELWERT = 24;
const short AdcRbs::LOAD_CAPACITY_SBREGWDHSPERRE = 25;
const short AdcRbs::LOAD_CAPACITY_SBREGGRENZE = 26;
const short AdcRbs::LOAD_CAPACITY_SBVERRIEGELWERT = 27;
const short AdcRbs::LOAD_CAPACITY_PAREGWDHSPERRE = 28;
const short AdcRbs::LOAD_CAPACITY_PAREGGRENZE = 29;
const short AdcRbs::LOAD_CAPACITY_PAVERRIEGELWERT = 30;
const short AdcRbs::LOAD_CAPACITY_AUFLOESUNG = 31;				// optional
const short AdcRbs::LOAD_CAPACITY_STRINGS_MANDATORY_COUNT = 31;
const short AdcRbs::LOAD_CAPACITY_STRINGS_COUNT = 32;
const string AdcRbs::LOAD_CAPACITY_STRINGS[LOAD_CAPACITY_STRINGS_COUNT] = {	"bereich|INT",
																			"berUeberlauf|INT",
																			"komma|INT",
																			"mehrteilung|INT",
																			"teilGrenze[0]|INT",
																			"teilGrenze[1]|INT",
																			"teilSchritt[0]|INT",
																			"teilSchritt[1]|INT",
																			"teilSchritt[2]|INT",
																			"einschaltNullBerMi|INT",
																			"einschaltNullBerPl|INT",
																			"nullstellBerMi|INT",
																			"nullstellBerPl|INT",
																			"taraBereich|INT",
																			"taraHandBereich|INT",
																			"autNullpunktkor|INT",
																			"tralaCode|INT",
																			"genGew[0]|INT",
																			"genGew[1]|INT",
																			"genGew[2]|INT",
																			"anzNegBru|INT",
																			"gewichtsEinheit|INT",
																			"asRegWdhSperre|INT",
																			"asRegGrenze|INT",
																			"asVerriegelWert|INT",
																			"sbRegWdhSperre|INT",
																			"sbRegGrenze|INT",
																			"sbVerriegelWert|INT",
																			"paRegWdhSperre|INT",
																			"paRegGrenze|INT",
																			"paVerriegelWert|INT",
																			"aufloesung|INT" };

const short AdcRbs::TCC_STRINGS_COUNT = 15;
const string AdcRbs::TCC_STRINGS[TCC_STRINGS_COUNT] = { "angleLimit|INT",
														"weightLimit|INT",
														"dyLimit_a_xTilt|FLOAT",
														"dyLimit_b_xTilt|FLOAT",
														"xLimit_xTilt|INT",
														"corr_a_xTilt|FLOAT",
														"corr_b_xTilt|FLOAT",
														"dyLimit_a_yTilt|FLOAT",
														"xLimit_yTilt|INT",
														"corr_a_yTilt|FLOAT",
														"corrLimit_a_xTilt|FLOAT",
														"corrLimit_b_xTilt|FLOAT",
														"corrLimit_a_yTilt|FLOAT",
														"corrLimit_b_yTilt|FLOAT",
														"kippLimit|INT" };

const short AdcRbs::LIN_STRINGS_COUNT = 6;
const string AdcRbs::LIN_STRINGS[TCC_STRINGS_COUNT] = { "lin_a|FLOAT",
														"lin_b|FLOAT",
														"lin_c|FLOAT",
														"lin_d|FLOAT",
														"lin_e|FLOAT",
														"lin_f|FLOAT" };

const short AdcRbs::WDTA_STRINGS_COUNT = 4;
const string AdcRbs::WDTA_STRINGS[WDTA_STRINGS_COUNT] = { "x1|FLOAT",
														  "y1|INT",
														  "x2|FLOAT",
														  "y2|INT" };

const short AdcRbs::WDTA_STRINGS_TO_CONVERT_COUNT = 2;
const string AdcRbs::WDTA_STRINGS_TO_CONVERT[WDTA_STRINGS_TO_CONVERT_COUNT] = { "y1|FLOAT",
																				"y2|FLOAT" };
const short AdcRbs::SSP_STRINGS_COUNT = 1;
const string AdcRbs::SSP_STRINGS[SSP_STRINGS_COUNT] = { "" };

const short AdcRbs::SSPS_STRINGS_COUNT = 4;
const string AdcRbs::SSPS_STRINGS[SSPS_STRINGS_COUNT] = { "spiritLevel|INT",
														  "spiritLevelZeroingRange|INT",
														  "spiritLevelTc|INT",
														  "tiltCompensationAlwaysOn|INT" };

AdcRbs::AdcRbs() : AdcProtocol()
{
	Init();
}

AdcRbs::AdcRbs(AdcInterface *pInterface) : AdcProtocol(pInterface)
{
	Init();
}

AdcRbs::~AdcRbs()
{

}

void AdcRbs::Init()
{
	m_orderID = 0;
}

/**
******************************************************************************
* SetTare - tare scale
*
* @param    tare:in         tare structure
* @param    adcState:out    state of the adc
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetTare(const AdcTare *tare, AdcState *adcState)
{
    short           errorCode;
	short           errorCodeRefData;
    keyValuePair    refDataMap;
    RefDataStruct   refData;
    string          key;
    string          value;

    if (tare)
    {
        refDataMap.clear();

		if (tare->type != ADC_TARE_ATTRIBUTE_CONST)
		{
			// copy reference data to map
			refData.id = REF_DATA_ID_TARE;
			refData.u.tare = (AdcTare *)tare;
			CreateReferenceData(refData, refDataMap);
		}

		// copy reference data to map
		refData.id = REF_DATA_ID_CONST_TARE;
		refData.u.tare = (AdcTare *)tare;
		CreateReferenceData(refData, refDataMap);

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_SET_TARE, refDataMap);
       
        // get adcState
		if (CheckErrorCode(errorCode, true) && adcState)
        {
            refData.id = REF_DATA_ID_LCSTATE;
            refData.u.adcState = adcState;
			errorCodeRefData = GetReferenceData(REF_DATA_ID_LCSTATE_STR, refDataMap, refData, 1);
			if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
        }
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* GetTare - get tare limit weighed and known
*
* @param    tare:in/out     tare structure
* @param    adcState:out    state of the adc
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetTare(AdcTare *tare, AdcState *adcState)
{
	short           errorCode;
	short           errorCodeRefData;
	keyValuePair    refDataMap;
	RefDataStruct   refData;
	string          key;
	string          value;

	if (tare)
	{
		refDataMap.clear();

		// copy reference data to map
		refData.id = REF_DATA_ID_TARE_TYPE;
		refData.u.tare = (AdcTare *)tare;
		CreateReferenceData(refData, refDataMap);

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_TARE, refDataMap);

		// get adcState
		if (CheckErrorCode(errorCode, true) && adcState)
		{
			refData.id = REF_DATA_ID_LCSTATE;
			refData.u.adcState = adcState;
			errorCodeRefData = GetReferenceData(REF_DATA_ID_LCSTATE_STR, refDataMap, refData, 1);
			if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
		}

		// get tare limit
		if (CheckErrorCode(errorCode) && tare)
		{
			refData.id = REF_DATA_ID_TARE;
			tare->frozen = 0;
			refData.u.tare = tare;
			errorCodeRefData = GetReferenceData(REF_DATA_ID_TARE_STR, refDataMap, refData, 4);
			if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* SetTarePriority - tare prio
*
* @param    adcState:out    state of the adc
* @param    prio:in         tare prio
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetTarePriority(AdcState *adcState, const AdcTarePriority prio)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;
    string          key;
    string          value;

    refDataMap.clear();

    // copy reference data to map
    refData.id = REF_DATA_ID_TARE_PRIO;
    refData.u.tp = prio;
    CreateReferenceData(refData, refDataMap);

    // send request receive respnse
    errorCode = SendRequestReceiveResponse(CMD_SET_TARE_PRIO, refDataMap);

    // get adcState
	if (CheckErrorCode(errorCode) && adcState)
    {
        refData.id = REF_DATA_ID_LCSTATE;
        refData.u.adcState = adcState;
        errorCode = GetReferenceData(REF_DATA_ID_LCSTATE_STR, refDataMap, refData, 1);
    }

    return errorCode;
}

/**
******************************************************************************
* ClearTare - clear tare of the scale
*
* @param    adcState:out  state of the adc
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::ClearTare(AdcState *adcState)
{
    short           errorCode;
	short           errorCodeRefData;
    keyValuePair    refDataMap;
    RefDataStruct   refData;
    string          key;
    string          value;

    refDataMap.clear();

    // send request receive respnse
    errorCode = SendRequestReceiveResponse(CMD_CLEAR_TARE, refDataMap);

    // get adcState
    if (CheckErrorCode(errorCode, true) && adcState)
    {
        refData.id = REF_DATA_ID_LCSTATE;
        refData.u.adcState = adcState;
		errorCodeRefData = GetReferenceData(REF_DATA_ID_LCSTATE_STR, refDataMap, refData, 1);
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
    }

    return errorCode;
}


/**
******************************************************************************
* SetBaseprice - set baseprice
*
* @param    price:in  pointer to structure to set price
* @param    frozen:in constant
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetBaseprice(const AdcBasePrice *price, const short frozen)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

    if (price)
    {
        refDataMap.clear();

        // copy reference data to map
        refData.id = REF_DATA_ID_BASEPRICE;
        refData.u.bp = (AdcBasePrice *)price;
        CreateReferenceData(refData, refDataMap);

		// copy reference data to map
		refData.id = REF_DATA_ID_CONST_BASEPRICE;
		refData.u.frozen = frozen;
		CreateReferenceData(refData, refDataMap);

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE_BP, refDataMap);
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* SetDisplayText - set display text
*
* @param    text:in  pointer to structure to set display text
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetDisplayText(const AdcDisplayText *text)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

    if (text)
    {
        refDataMap.clear();

        // copy reference data to map
        refData.id = REF_DATA_ID_DISPLAYTEXT;
        refData.u.dt = (AdcDisplayText *)text;
        CreateReferenceData(refData, refDataMap);

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE_DT, refDataMap);
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* SetScaleMode - set scale mode
*
* @param    mode:in  scale mode
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetScaleMode(const AdcScaleMode mode)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

    refDataMap.clear();

    // copy reference data to map
    refData.id = REF_DATA_ID_SCALEMODE;
    refData.u.sm = mode;
    CreateReferenceData(refData, refDataMap);

    // send request receive respnse
    errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE_SM, refDataMap);

    return errorCode;
}


/**
******************************************************************************
* SetEeprom - write data to eeprom
*
* @param    eeprom:in  data to write to eeprom
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetEeprom(const AdcEeprom *eeprom)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

    if (eeprom)
    {
        refDataMap.clear();

        // copy reference data to map
        refData.id = REF_DATA_ID_EEPROM;
        refData.u.ee = (AdcEeprom *)eeprom;
        CreateReferenceData(refData, refDataMap);

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE_EE, refDataMap);
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* SetTiltCompensation - set parameter for tilt compensation
*
* @param    tiltCompensation:in  pointer to structure to set parameter for tilt compensation
* @param    tccSettings:in		 pointer to ssp settings
* @param    linSettings:in		 pointer to lin settings
* @param    wdtaSettings:in		 pointer to wdta settings
* @param	spiritLevelCalMode:in		 pointer to spirit level calibration mode
*
* @return   errorCode
* @remarks  only state and limit are writable
******************************************************************************
*/
short AdcRbs::SetTiltCompensation(const AdcTiltComp *tiltCompensation, const map<string, string> *tccSettings, const map<string, string> *linSettings, const map<string, string> *wdtaSettings, const AdcSpiritLevelCmd *spiritLevelCmd)
{
	short           errorCode = LarsErr::E_SUCCESS;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

	if (tiltCompensation || tccSettings || linSettings || wdtaSettings || spiritLevelCmd)
    {
        refDataMap.clear();

		if (tiltCompensation)
		{
			// copy reference data to map
			refData.id = REF_DATA_ID_TILT_COMP;
			refData.u.tc = (AdcTiltComp *)tiltCompensation;
			CreateReferenceData(refData, refDataMap);
		}

		if (tccSettings && !tccSettings->empty())
		{
			// copy reference data to map
			refData.id = REF_DATA_ID_TILT_TCC;
			refData.u.tcc = (map<string, string> *)tccSettings;
			CreateReferenceData(refData, refDataMap);
		}

		if (linSettings && !linSettings->empty())
		{
			// copy reference data to map
			refData.id = REF_DATA_ID_TILT_LIN;
			refData.u.lin = (map<string, string> *)linSettings;
			CreateReferenceData(refData, refDataMap);
		}

		if (wdtaSettings && !wdtaSettings->empty())
		{
			map<string, string> tmpWdtaSettings = *wdtaSettings;
			// convert degree to digits
			if (ConvertDegreeToDigits(&tmpWdtaSettings) == true)
			{
				// copy reference data to map
				refData.id = REF_DATA_ID_TILT_WDTA;
				refData.u.wdta = &tmpWdtaSettings;
				CreateReferenceData(refData, refDataMap);
			}
		}

		if (spiritLevelCmd)
		{
			// copy reference data to map
			refData.id = REF_DATA_ID_SPIRIT_LEVEL;
			refData.u.spiritLevelCmd = *spiritLevelCmd;
			CreateReferenceData(refData, refDataMap);
		}

		if (refDataMap.size())
			// send request receive respnse
			errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE_TC, refDataMap);
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* SetTiltAngleConfirm - function to confirm that the tilt compensation is correct under max angle
*
* @param    cmd:in				ADC_TILT_ANGLE_RESET			reset the confirmed titl angle
*								ADC_TILT_ANGLE_CONFIRM			only a greater max tilt angle will taken over
*								ADC_TILT_ANGLE_CONFIRM_FORCE	the current max tilt angle will taken over
*
* @return   errorCode
* @remarks  
******************************************************************************
*/
short AdcRbs::SetTiltAngleConfirm(const AdcTiltAngleConfirmCmd cmd)
{
	short           errorCode = LarsErr::E_SUCCESS;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy reference data to map
	refData.id = REF_DATA_ID_TILT_COMP_CHECKED;
	refData.u.tiltAngleConfirmCmd = cmd;
	CreateReferenceData(refData, refDataMap);

	if (refDataMap.size())
		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetScaleSpecificSettingsGeneral - set scale specific settings general
*
* @param    scaleSpecificSettings:in		pointer to scale specific settings
* @param    scaleSpecificSettingsSealed:in	pointer to scale specific settings sealed
*
* @return   errorCode
******************************************************************************
*/
short AdcRbs::SetScaleSpecificSettingsGeneral(const map<string, string> *scaleSpecificSettings, const map<string, string> *scaleSpecificSettingsSealed)
{
	short           errorCode = LarsErr::E_SUCCESS;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (scaleSpecificSettings || scaleSpecificSettingsSealed)
	{
		refDataMap.clear();

		if (scaleSpecificSettings && !scaleSpecificSettings->empty())
		{
			// copy reference data to map
			refData.id = REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL;
			refData.u.ssp = (map<string, string> *)scaleSpecificSettings;
			CreateReferenceData(refData, refDataMap);
		}

		if (scaleSpecificSettingsSealed && !scaleSpecificSettingsSealed->empty())
		{
			// copy reference data to map
			refData.id = REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL_SEALED;
			refData.u.ssps = (map<string, string> *)scaleSpecificSettingsSealed;
			CreateReferenceData(refData, refDataMap);
		}

		if (refDataMap.size())
			// send request receive respnse
			errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE, refDataMap);
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* SetFilter - set filter parameter
*
* @param    filter:in	pointer to structure to set parameter for adc filter
*
* @return   errorCode
* @remarks  
******************************************************************************
*/
short AdcRbs::SetFilter(const AdcFilter *filter)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (filter)
	{
		refDataMap.clear();

		// copy filter index to map
		refData.id = REF_DATA_ID_FILTER_IDX;
		refData.u.filter = (AdcFilter *)filter;
		CreateReferenceData(refData, refDataMap);

		// copy offset stable time to map
		refData.id = REF_DATA_ID_OFFSET_STABLE_TIME;
		refData.u.filter = (AdcFilter *)filter;
		CreateReferenceData(refData, refDataMap);

		// copy stable range to map
		refData.id = REF_DATA_ID_STABLE_RANGE;
		refData.u.filter = (AdcFilter *)filter;
		CreateReferenceData(refData, refDataMap);


		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE_FILTER, refDataMap);
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* SetGFactor - set filter parameter
*
* @param    gFactor:in	g factor
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetGFactor(const short gFactor)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy filter index to map
	refData.id = REF_DATA_ID_GFACTOR;
	refData.u.gFactor = gFactor;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE_GFACTOR, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetProdSettings - set production settings
*
* @param    prodSettings:in	pointer to structure for production settings
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetProdSettings(const AdcProdSettings *prodSettings)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy production site id to map
	refData.id = REF_DATA_ID_PROD_SITE;
	refData.u.prodSiteID = prodSettings->prodSiteID;
	CreateReferenceData(refData, refDataMap);

	// copy g factor offset to map
	refData.id = REF_DATA_ID_PROD_GFACTOR_OFFSET;
	refData.u.gFactor = prodSettings->gFactorOffset;
	CreateReferenceData(refData, refDataMap);

	// copy date to map
	refData.id = REF_DATA_ID_PROD_DATE;
	strcpy(refData.u.dateStr, prodSettings->date);
	CreateReferenceData(refData, refDataMap);

	if (strlen(prodSettings->wsc))
	{
		// copy load cell string only if it is not empty
		refData.id = REF_DATA_ID_PROD_WSC;
		strcpy(refData.u.wsc, prodSettings->wsc);
		CreateReferenceData(refData, refDataMap);
	}

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE_PROD_SETTINGS, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetConversionWeightUnit - set factor to convert weight unit
*
* @param    factor:in	multiply factor
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetConversionWeightUnit(const long factor)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy production site id to map
	refData.id = REF_DATA_ID_CONVERSION_WEIGHT_UNIT;
	refData.u.conversionWeightUnit = factor;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE_CONVERSION_WEIGHT_UNIT, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetInterfaceMode - set interface mode
*
* @param    interfaceMode:in	pointer to structure to set interface mode
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetInterfaceMode(const AdcInterfaceMode *interfaceMode)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy interface auto detection to map
	refData.id = REF_DATA_ID_INTERFACE_AUTO_DETECTION;
	refData.u.interfaceAutoDetection = interfaceMode->autoDetection;
	CreateReferenceData(refData, refDataMap);

	// copy interface auto detection to map
	refData.id = REF_DATA_ID_INTERFACE_MODE;
	refData.u.interfaceMode = interfaceMode->mode;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE_INTERFACE_MODE, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetZeroPointTracking - set zero point tracking mode
*
* @param    mode:in		zero point tracking mode
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetZeroPointTracking(const short mode)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy zero point tracking mode to map
	refData.id = REF_DATA_ID_ZERO_POINT_TRACKING;
	refData.u.mode = mode;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_ZEROSETTING_ZEROTRACKING_CONFIG, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetVerifParamProtected - set verification parameter protected
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetVerifParamProtected()
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy verification parameter protected mode to map
	refData.id = REF_DATA_ID_VERIF_PARAM_PROTECTED;
	refData.u.verifParamProtected = 1;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetUpdateAllowed - set update allowed parameter
*
* @param    mode:in		update allowed parameter
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetUpdateAllowed(const short mode)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy update allowed mode to map
	refData.id = REF_DATA_ID_UPDATE_ALLOWED;
	refData.u.verifParamProtected = mode;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetOperatingMode - set adc operating mode
*
* @param    mode:in		adc operating mode
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetOperatingMode(AdcOperatingMode mode)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy operating mode to map
	refData.id = REF_DATA_ID_OPERATING_MODE;
	refData.u.opMode = mode;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetWarmUpTime - set warm up time in seconds
*
* @param    seconds:in	warm up time in seconds
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetWarmUpTime(const long seconds)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy warm upt time to map
	refData.id = REF_DATA_ID_WARM_UP_TIME;
	refData.u.warmUpTime = seconds;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetZeroSettingInterval - set zero setting interval in seconds
*
* @param    seconds:in	zero setting interval in seconds
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetZeroSettingInterval(const long seconds)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy zero setting interval to map
	refData.id = REF_DATA_ID_ZERO_SETTING_INTERVAL;
	refData.u.zeroSettingInterval = seconds;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_ZEROSETTING_ZEROTRACKING_CONFIG, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetAutomaticZeroSettingTime - set automatic zero setting time in seconds
*
* @param    seconds:in	automatic zero setting time in seconds
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetAutomaticZeroSettingTime(const long seconds)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy automatic zero setting time to map
	refData.id = REF_DATA_ID_AUTOMATIC_ZERO_SETTING_TIME;
	refData.u.automaticZeroSettingTime = seconds;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_ZEROSETTING_ZEROTRACKING_CONFIG, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetInitialZeroSetting - set initial zero-setting
*
* @param    param:in	pointer to initial zero-setting parameters
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetInitialZeroSetting(const AdcInitialZeroSettingParam *param)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy initial zero setting to map
	refData.id = REF_DATA_ID_INITIAL_ZERO_SETTING;
	refData.u.initialZeroSettingParam = (AdcInitialZeroSettingParam *)param;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_ZEROSETTING_ZEROTRACKING_CONFIG, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* SetScaleModel - set scale model
*
* @param    model:in	scale model
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetScaleModel(const string &scaleModel)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy verification parameter protected mode to map
	refData.id = REF_DATA_ID_SCALE_MODEL;
	refData.u.scaleModel = (string *)&scaleModel;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE, refDataMap);

	return errorCode;
}



/**
******************************************************************************
* SetStateAutomaticTiltSensor - set the state of the automatic tilt sensor
*
* @param    state:in		false: disable
*							true:  enable
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetStateAutomaticTiltSensor(const bool state)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy state of the automatic tilt sensor to map
	refData.id = REF_DATA_ID_STATE_AUTOMATIC_TILT_SENSOR;
	refData.u.state = state;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_SET_SCALE_VALUE, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* ZeroScale - zero scale
*
* @param    adcState:out  state of the adc
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::ZeroScale(AdcState *adcState)
{
    short           errorCode;
	short           errorCodeRefData;
    keyValuePair    refDataMap;
    RefDataStruct   refData;
    string          key;
    string          value;

    refDataMap.clear();

    // send request receive respnse
    errorCode = SendRequestReceiveResponse(CMD_SET_ZERO, refDataMap);

    // get adcState
    if (CheckErrorCode(errorCode, true) && adcState)
    {
        refData.id = REF_DATA_ID_LCSTATE;
        refData.u.adcState = adcState;
		errorCodeRefData = GetReferenceData(REF_DATA_ID_LCSTATE_STR, refDataMap, refData, 1);
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
    }

    return errorCode;
}


/**
******************************************************************************
* GetBaseprice - get baseprice
*
* @param    price:out  pointer to structure to get price
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetBaseprice(AdcBasePrice *price)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

    if (price)
    {
        refDataMap.clear();

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE_BP, refDataMap);

        // get baseprice
        if (errorCode == LarsErr::E_SUCCESS)
        {
            refData.id = REF_DATA_ID_BASEPRICE;
            refData.u.bp = price;
            errorCode = GetReferenceData(REF_DATA_ID_BASEPRICE_STR, refDataMap, refData, 4);
        }
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* GetDisplayText - set display text
*
* @param    text:in  pointer to structure to set display text
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetDisplayText(AdcDisplayText *text)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

    if (text)
    {
        refDataMap.clear();

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE_DT, refDataMap);

        // get display text
        if (errorCode == LarsErr::E_SUCCESS)
        {
            // initialize optional parameters
            text->x = 1;
            text->y = 1;

            // parse receiving data
            refData.id = REF_DATA_ID_DISPLAYTEXT;
            refData.u.dt = text;
            errorCode = GetReferenceData(REF_DATA_ID_DISPLAYTEXT_STR, refDataMap, refData, 1);
        }
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* GetScaleMode - get scale mode
*
* @param    mode:out  scale mode
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetScaleMode(AdcScaleMode *mode)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

    if (mode)
    {
        refDataMap.clear();

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE_SM, refDataMap);

        if (errorCode == LarsErr::E_SUCCESS)
        {
            // parse receiving data
            refData.id = REF_DATA_ID_SCALEMODE;
            if ((errorCode = GetReferenceData(REF_DATA_ID_SCALEMODE_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
                *mode = refData.u.sm;
        }
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* GetEeprom - read data from eeprom
*
* @param    eeprom:out  data read from eeprom
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetEeprom(AdcEeprom *eeprom)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

    if (eeprom)
    {
        refDataMap.clear();

        // copy reference data to map
        refData.id = REF_DATA_ID_EEPROM;
        refData.u.ee = eeprom;
        CreateReferenceData(refData, refDataMap);

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE_EE, refDataMap);

        // get eeprom data
        if (errorCode == LarsErr::E_SUCCESS)
        {
            refData.id = REF_DATA_ID_EEPROM;
            refData.u.ee = eeprom;
            errorCode = GetReferenceData(REF_DATA_ID_EEPROM_STR, refDataMap, refData, 4);
        }
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* GetTiltCompensation - get parameter of the tilt compensation
*
* @param    tiltCompensation:out  pointer to structure to get parameter of the tilt compensation
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetTiltCompensation(AdcTiltComp *tiltCompensation)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

    if (tiltCompensation)
    {
		// clear structure
		memset(tiltCompensation, 0, sizeof(AdcTiltComp));

        refDataMap.clear();

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE_TC, refDataMap);

        // get tilt compensation data
        if (errorCode == LarsErr::E_SUCCESS)
        {
            refData.id = REF_DATA_ID_TILT_COMP;
            refData.u.tc = tiltCompensation;
            errorCode = GetReferenceData(REF_DATA_ID_TILT_COMP_STR, refDataMap, refData, 5);
        }
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* GetFilter - get filter parameter 
*
* @param    filter:out  pointer to structure to get filter parameter
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetFilter(AdcFilter *filter)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (filter)
	{
		refDataMap.clear();

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE_FILTER, refDataMap);

		// get filter index
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_FILTER_IDX;
			refData.u.filter = filter;
			errorCode = GetReferenceData(REF_DATA_ID_FILTER_IDX_STR, refDataMap, refData, 1);
		}

		// get offset stable time
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_OFFSET_STABLE_TIME;
			refData.u.filter = filter;
			errorCode = GetReferenceData(REF_DATA_ID_OFFSET_STABLE_TIME_STR, refDataMap, refData, 1);
		}

		// get stable range
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_STABLE_RANGE;
			refData.u.filter = filter;
			errorCode = GetReferenceData(REF_DATA_ID_STABLE_RANGE_STR, refDataMap, refData, 1);
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* GetGFactor - get g factor
*
* @param    gFactor:out  pointer to g factor
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetGFactor(short *gFactor)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (gFactor)
	{
		refDataMap.clear();

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE_GFACTOR, refDataMap);

		// get g factor
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_GFACTOR;
			if ((errorCode = GetReferenceData(REF_DATA_ID_GFACTOR_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*gFactor = refData.u.gFactor;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* GetProdSettings - get production settings
*
* @param    prodSettings:out  pointer to production settings
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetProdSettings(AdcProdSettings *prodSettings)
{
	short           errorCode;
	short			errorCodeRefData;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (prodSettings)
	{
		refDataMap.clear();

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE_PROD_SETTINGS, refDataMap);

		// get production site
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_PROD_SITE;
			if ((errorCode = GetReferenceData(REF_DATA_ID_PROD_SITE_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				prodSettings->prodSiteID = refData.u.prodSiteID;
		}

		// get offset g factor
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_PROD_GFACTOR_OFFSET;
			if ((errorCode = GetReferenceData(REF_DATA_ID_PROD_GFACTOR_OFFSET_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				prodSettings->gFactorOffset = refData.u.gFactor;
		}

		// get calibration date
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_PROD_DATE;
			if ((errorCode = GetReferenceData(REF_DATA_ID_PROD_DATE_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				strcpy(prodSettings->date, refData.u.dateStr);
			else
				prodSettings->date[0] = 0;
		}

		// get wsc string
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_PROD_WSC;
			if ((errorCodeRefData = GetReferenceData(REF_DATA_ID_PROD_WSC_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				strcpy(prodSettings->wsc, refData.u.wsc);
			else
			{
				// parameter is optional, so ignore all errors
				prodSettings->wsc[0] = 0;
				errorCodeRefData = LarsErr::E_SUCCESS;
			}
			errorCode = errorCodeRefData;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* GetInterfaceMode - get interface mode
*
* @param    interfaceMode:out  pointer to interface mode 
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetInterfaceMode(AdcInterfaceMode *interfaceMode)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (interfaceMode)
	{
		refDataMap.clear();

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE_INTERFACE_MODE, refDataMap);

		// get interface auto detection
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_INTERFACE_AUTO_DETECTION;
			if ((errorCode = GetReferenceData(REF_DATA_ID_INTERFACE_AUTO_DETECTION_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				interfaceMode->autoDetection = refData.u.interfaceAutoDetection;
		}

		// get interface mode
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_INTERFACE_MODE;
			if ((errorCode = GetReferenceData(REF_DATA_ID_INTERFACE_MODE_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				interfaceMode->mode = refData.u.interfaceMode;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* GetZeroPointTracking - get mode zero point tracking
*
* @param    mode:out  pointer to mode
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetZeroPointTracking(short *mode)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (mode)
	{
		refDataMap.clear();
		refDataMap[REF_DATA_ID_ZERO_POINT_TRACKING_STR] = "";

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_ZEROSETTING_ZEROTRACKING_CONFIG, refDataMap);

		// get mode
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_ZERO_POINT_TRACKING;
			if ((errorCode = GetReferenceData(REF_DATA_ID_ZERO_POINT_TRACKING_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*mode = refData.u.mode;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* GetZeroSettingInterval - get zero setting interval
*
* @param    seconds:out		zero setting interval in seconds
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetZeroSettingInterval(long *seconds)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (seconds)
	{
		refDataMap.clear();
		refDataMap[REF_DATA_ID_ZERO_SETTING_INTERVAL_STR] = "";

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_ZEROSETTING_ZEROTRACKING_CONFIG, refDataMap);

		// get operating mode
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_ZERO_SETTING_INTERVAL;
			if ((errorCode = GetReferenceData(REF_DATA_ID_ZERO_SETTING_INTERVAL_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*seconds = refData.u.zeroSettingInterval;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* GetAutomaticZeroSettingTime - get automatic zero setting time
*
* @param    seconds:out		automatic zero setting time in seconds
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetAutomaticZeroSettingTime(long *seconds)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (seconds)
	{
		refDataMap.clear();
		refDataMap[REF_DATA_ID_AUTOMATIC_ZERO_SETTING_TIME_STR] = "";

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_ZEROSETTING_ZEROTRACKING_CONFIG, refDataMap);

		// get operating mode
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_AUTOMATIC_ZERO_SETTING_TIME;
			if ((errorCode = GetReferenceData(REF_DATA_ID_AUTOMATIC_ZERO_SETTING_TIME_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*seconds = refData.u.automaticZeroSettingTime;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* GetVerifParamProtected - get verification parameter protected
*
* @param    mode:out	verification parameter protected
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetVerifParamProtected(short *mode)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (mode)
	{
		refDataMap.clear();
		refDataMap[REF_DATA_ID_VERIF_PARAM_PROTECTED_STR] = "";

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE, refDataMap);

		// get mode
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_VERIF_PARAM_PROTECTED;
			if ((errorCode = GetReferenceData(REF_DATA_ID_VERIF_PARAM_PROTECTED_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*mode = refData.u.verifParamProtected;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* GetUpdateAllowed - get update allowed parameter
*
* @param    mode:out	update allowed parameter
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetUpdateAllowed(short *mode)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (mode)
	{
		refDataMap.clear();
		refDataMap[REF_DATA_ID_UPDATE_ALLOWED_STR] = "";

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE, refDataMap);

		// get mode
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_UPDATE_ALLOWED;
			if ((errorCode = GetReferenceData(REF_DATA_ID_UPDATE_ALLOWED_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*mode = refData.u.updateAllowed;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* GetWarmUpTime - get warm up time in seconds
*
* @param    seconds:out	warm up time in seconds
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetWarmUpTime(long *seconds)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (seconds)
	{
		refDataMap.clear();
		refDataMap[REF_DATA_ID_WARM_UP_TIME_STR] = "";

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE, refDataMap);

		// get mode
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_WARM_UP_TIME;
			if ((errorCode = GetReferenceData(REF_DATA_ID_WARM_UP_TIME_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*seconds = refData.u.warmUpTime;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* GetRemainingWarmUpTime - get remaining warm up time in seconds
*
* @param    seconds:out	reamining warm up time in seconds
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetRemainingWarmUpTime(long *seconds)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (seconds)
	{
		refDataMap.clear();
		refDataMap[REF_DATA_ID_REMAINING_WARM_UP_TIME_STR] = "";

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE, refDataMap);

		// get mode
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_REMAINING_WARM_UP_TIME;
			if ((errorCode = GetReferenceData(REF_DATA_ID_REMAINING_WARM_UP_TIME_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*seconds = refData.u.remainingWarmUpTime;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* GetOperatingMode - get adc operating mode
*
* @param    mode:out	adc operating mode
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetOperatingMode(AdcOperatingMode *mode)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (mode)
	{
		refDataMap.clear();
		refDataMap[REF_DATA_ID_OPERATING_MODE_STR] = "";

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE, refDataMap);

		// get operating mode
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_OPERATING_MODE;
			if ((errorCode = GetReferenceData(REF_DATA_ID_OPERATING_MODE_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*mode = refData.u.opMode;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* GetScaleModel - get scale model
*
* @param    model:in	scale model
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetScaleModel(string &scaleModel)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();
	refDataMap[REF_DATA_ID_SCALE_MODEL_STR] = "";

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE, refDataMap);

	// get operating mode
	if (errorCode == LarsErr::E_SUCCESS)
	{
		refData.id = REF_DATA_ID_SCALE_MODEL;
		refData.u.scaleModel = &scaleModel;
		errorCode = GetReferenceData(REF_DATA_ID_SCALE_MODEL_STR, refDataMap, refData, 1);
	}

	return errorCode;
}


/**
******************************************************************************
* GetStateAutomaticTiltSensor - get the state of the automatic tilt sensor
*
* @param    state:out	false: disable
*						true:  enable
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetStateAutomaticTiltSensor(bool *state)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (state)
	{
		refDataMap.clear();
		refDataMap[REF_DATA_ID_STATE_AUTOMATIC_TILT_SENSOR_STR] = "";

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE, refDataMap);

		// get mode
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_STATE_AUTOMATIC_TILT_SENSOR;
			if ((errorCode = GetReferenceData(REF_DATA_ID_STATE_AUTOMATIC_TILT_SENSOR_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*state = refData.u.state;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* FirmwareUpdate - function to update the adc firmware
*
* @param	cmd			command
* @param	data		firmware data
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::FirmwareUpdate(const AdcFrmCmd cmd, const AdcFrmData *data, const string *usbStackVersion, const string *welmecStructureVersion, const short force)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy reference data to map
	refData.id = REF_DATA_ID_FRM_COMMAND;
	refData.u.frmCmd = cmd;
	CreateReferenceData(refData, refDataMap);

	if ((cmd == ADC_FRMUPDATE_START))
	{
		// add date
		struct tm * timeinfo;
		char timeString[80];
		// get local time
		timeinfo = Helpers::GetTime();
		strftime(timeString, sizeof(timeString), "%y%m%d%H%M", timeinfo);
		refData.id = REF_DATA_ID_FRM_DATE;
		refData.u.frmStringData = timeString;
		CreateReferenceData(refData, refDataMap);

		// add usb stack version
		if ((usbStackVersion != NULL) && !usbStackVersion->empty())
		{
			refData.id = REF_DATA_ID_FRM_USBSTACK_VERS;
			refData.u.frmStringData = (char *)usbStackVersion->c_str();
			CreateReferenceData(refData, refDataMap);
		}

		// add welmec structure version
		if ((welmecStructureVersion != NULL) && !welmecStructureVersion->empty())
		{
			refData.id = REF_DATA_ID_FRM_WELMECSTRUCT_VERS;
			refData.u.frmStringData = (char *)welmecStructureVersion->c_str();
			CreateReferenceData(refData, refDataMap);
		}

		// add force
		if (force != 0)
		{
			refData.id = REF_DATA_ID_FRM_FORCE;
			refData.u.frmForce = force;
			CreateReferenceData(refData, refDataMap);
		}
	}

	if ((cmd == ADC_FRMUPDATE_WRITE_DATA) && (data != 0))
	{
		refData.id = REF_DATA_ID_FRM_DATA;
		refData.u.frmData = (AdcFrmData *)data;
		CreateReferenceData(refData, refDataMap);
	}

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_FRM_UPDATE, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* Reset - send reset command to adc
*
* @param type:in	Reset type (HARDWARE_RESET, SOFTWARE_RESET)   
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::Reset(const AdcResetType type)
{
    short           errorCode;
    keyValuePair    refDataMap;
	RefDataStruct   refData;

    refDataMap.clear();

	// copy reference data to map
	refData.id = REF_DATA_ID_RESET_TYPE;
	refData.u.resetType = type;
	CreateReferenceData(refData, refDataMap);

    // send request receive respnse
    errorCode = SendRequestReceiveResponse(CMD_ADC_RESET, refDataMap);

    return errorCode;
}


/**
******************************************************************************
* ReadWeight - read weight
*
* @param    registrationRequest:in  1: application wants to make an registration
*                                   0: only read the current scale values
* @param    adcState:out            state of the adc
* @param    weight:out              current weight
* @param    tare:out                current tare
* @param    basePrice:out           current base price
* @param    sellPrice:out           current sell price
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::ReadWeight(const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcTare *tare, AdcBasePrice *basePrice, AdcPrice *sellPrice)
{
    short           errorCode;
	short			errorCodeRefData;
    keyValuePair    refDataMap;
    RefDataStruct   refData;
    string          key;
    string          value;

    refDataMap.clear();

    // copy reference data to map
    refData.id = REF_DATA_ID_REGISTRATION_REQ;
    refData.u.rr = registrationRequest;
    CreateReferenceData(refData, refDataMap);

    // send request receive respnse
    errorCode = SendRequestReceiveResponse(CMD_READ_WEIGHT, refDataMap);

    // get adcState
    if (CheckErrorCode(errorCode, true) && adcState)
    {
        refData.id = REF_DATA_ID_LCSTATE;
        refData.u.adcState = adcState;
		errorCodeRefData = GetReferenceData(REF_DATA_ID_LCSTATE_STR, refDataMap, refData, 1);
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
    }

    // get weight
	if (CheckErrorCode(errorCode) && weight)
    {
        refData.id = REF_DATA_ID_WEIGHT;
        refData.u.wt = weight;
		errorCodeRefData = GetReferenceData(REF_DATA_ID_WEIGHT_STR, refDataMap, refData, 3);
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
    }

    // get tare (optional)
	if (CheckErrorCode(errorCode) && tare)
    {
        refData.id = REF_DATA_ID_TARE;
		tare->frozen = 0;
        refData.u.tare = tare;
		if ((errorCodeRefData = GetReferenceData(REF_DATA_ID_TARE_STR, refDataMap, refData, 4)) == LarsErr::E_KEY_NOT_FOUND)
		{
			tare->type = AdcTareType::ADC_TARE_NO;
			tare->frozen = 0;
			tare->value.value = 0;
			tare->value.weightUnit = weight->weightUnit;
			tare->value.decimalPlaces = weight->decimalPlaces;
			errorCodeRefData = LarsErr::E_SUCCESS;
		}
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
    }

    // get base price (optional)
	if (CheckErrorCode(errorCode) && basePrice)
    {
        refData.id = REF_DATA_ID_BASEPRICE;
        refData.u.bp = basePrice;
		if ((errorCodeRefData = GetReferenceData(REF_DATA_ID_BASEPRICE_STR, refDataMap, refData, 4)) == LarsErr::E_KEY_NOT_FOUND)
        {
            basePrice->price.value = 0;
            basePrice->price.decimalPlaces = 0;
            basePrice->price.currency[0] = '\0';
			basePrice->weightUnit = AdcWeightUnit::ADC_KILOGRAM;
			errorCodeRefData = LarsErr::E_SUCCESS;
        }
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
    }

    // get selling price (optional)
	if (CheckErrorCode(errorCode) && sellPrice)
    {
        refData.id = REF_DATA_ID_SELLPRICE;
        refData.u.sp = sellPrice;
		if ((errorCodeRefData = GetReferenceData(REF_DATA_ID_SELLPRICE_STR, refDataMap, refData, 3)) == LarsErr::E_KEY_NOT_FOUND)
        {
            sellPrice->value = 0;
            sellPrice->decimalPlaces = 0;
            sellPrice->currency[0] = '\0';
			errorCodeRefData = LarsErr::E_SUCCESS;
        }
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
    }

    return errorCode;
}


/**
******************************************************************************
* GetLogger - function to get adc log book
*
* @param    index:in				index for log book entry
* @param    entry:out				one log book row
* @param    size:in                 size of entry in byte
*
* @return   ADC_SUCCESS
*			ADC_E_USB_ERROR
*			ADC_E_INVALID_HANDLE
* @remarks	The latest log book entry has index = 0
******************************************************************************
*/
short AdcRbs::GetLogger(const short index, char *entry, unsigned long *size)
{
    short           errorCode = LarsErr::E_SUCCESS;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

    if (size)
    {
        refDataMap.clear();

        // copy reference data to map
        refData.id = REF_DATA_ID_LOGBOOK;
        refData.u.idx = index;
        CreateReferenceData(refData, refDataMap);

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_LOGBOOK, refDataMap);

        if (errorCode == LarsErr::E_SUCCESS)
        {
            stringstream    logBookRow;
            bool            bFirstRun = true;

            // remove key stat from map
            refDataMap.erase(REF_DATA_ID_STAT_STR);

            if (refDataMap.size())
            {
                logBookRow << setbase(m_base);
                logBookRow << Lars::STX;

                for (keyValuePair::iterator it = refDataMap.begin(); it != refDataMap.end(); it++)
                {
                    if (!bFirstRun) logBookRow << Lars::GS;
                    logBookRow << (*it).first << Lars::FS << (*it).second;
                    bFirstRun = false;
                }
                logBookRow << Lars::ETX;

                if ((logBookRow.str().size() + 1 <= (size_t)*size) && entry)
                    strcpy(entry, logBookRow.str().c_str());
                else
                    errorCode = LarsErr::E_NOT_ENOUGH_MEMORY;

                // don't forget the terminating \0
                *size = logBookRow.str().size() + 1;
            }
            else if ((*size >= 1) && entry)
            {
                entry[0] = '\0';
                *size = 1;
            }
        }
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* GetRandomAuthentication - function to request the random value from adc
*
* @param    random:out				random value form adc
* @param    size:in/out				size of random
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetRandomAuthentication(unsigned char *random, unsigned long *size)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;
   
    if (random && size)
    {
        refDataMap.clear();

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_GET_RANDOM_AUTH, refDataMap);

        if (errorCode == LarsErr::E_SUCCESS)
        {
            refData.id = REF_DATA_ID_RANDOM_AUTH;
            refData.u.random.value = random;
            refData.u.random.size = size;
            errorCode = GetReferenceData(REF_DATA_ID_RANDOM_AUTH_STR, refDataMap, refData, 1);
        }
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* SetAuthentication - function to do the authentication
*
* @param    authentication:in		struct for authentication 
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetAuthentication(const AdcAuthentication *authentication)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

    if (authentication)
    {
        refDataMap.clear();

        // copy reference data to map
        refData.id = REF_DATA_ID_AUTHENTICATION;
        refData.u.au = (AdcAuthentication *)authentication;
        CreateReferenceData(refData, refDataMap);

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_SET_AUTH, refDataMap);
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* GetMaxDisplayTextChars - get number of characters of the display
*
* @param    number:out				number of characters
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetMaxDisplayTextChars(short *number)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;

    if (number)
    {
        refDataMap.clear();

        // send request receive respnse
        errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE_MAX_DT, refDataMap);

        // get max display characters
        if (errorCode == LarsErr::E_SUCCESS)
        {
            refData.id = REF_DATA_ID_MAX_DISPL_CHAR;
            if ((errorCode = GetReferenceData(REF_DATA_ID_MAX_DISPL_CHAR_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
                *number = refData.u.ndt;
        }
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* GetHighResolution - function to get the high resolution values from the adc
*
* @param    registrationRequest:in		1: application wants to make an registration
*										0: only read the current scale values
* @param    adcState:out				adc state
* @param    weight:out					weight
* @param    weightHighResolution:out	high resolution weight
* @param    tare:out					current tare
* @param    digitValue:out				weight as digit value
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetHighResolution(const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcWeight *weightHighResolution, AdcTare *tare, long *digitValue)
{
    short           errorCode;
	short			errorCodeRefData;
    keyValuePair    refDataMap;
    RefDataStruct   refData;
    string          key;
    string          value;

    refDataMap.clear();

	// copy reference data to map
	refData.id = REF_DATA_ID_REGISTRATION_REQ;
	refData.u.rr = registrationRequest;
	CreateReferenceData(refData, refDataMap);

    // send request receive respnse
    errorCode = SendRequestReceiveResponse(CMD_GET_HIGH_RESOLUTION, refDataMap);

    // get adcState
    if (CheckErrorCode(errorCode, true) && adcState)
    {
        refData.id = REF_DATA_ID_LCSTATE;
        refData.u.adcState = adcState;
		errorCodeRefData = GetReferenceData(REF_DATA_ID_LCSTATE_STR, refDataMap, refData, 1);
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
    }

    // get weight
	if (CheckErrorCode(errorCode) && weight)
    {
        refData.id = REF_DATA_ID_WEIGHT;
        refData.u.wt = weight;
		errorCodeRefData = GetReferenceData(REF_DATA_ID_WEIGHT_STR, refDataMap, refData, 3);
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
    }

    // get high resolution weight
	if (CheckErrorCode(errorCode) && weightHighResolution)
    {
        refData.id = REF_DATA_ID_WEIGHT;
        refData.u.wt = weightHighResolution;
		errorCodeRefData = GetReferenceData(REF_DATA_ID_HIGHWEIGHT_STR, refDataMap, refData, 3);
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
    }

	// get tare (otpional)
	if (CheckErrorCode(errorCode) && tare)
	{
		refData.id = REF_DATA_ID_TARE;
		tare->frozen = 0;
		refData.u.tare = tare;
		if ((errorCodeRefData = GetReferenceData(REF_DATA_ID_TARE_STR, refDataMap, refData, 4)) == LarsErr::E_KEY_NOT_FOUND)
		{
			tare->type = AdcTareType::ADC_TARE_NO;
			tare->frozen = 0;
			tare->value.value = 0;
			tare->value.weightUnit = weight->weightUnit;
			tare->value.decimalPlaces = weight->decimalPlaces;
			errorCodeRefData = LarsErr::E_SUCCESS;
		}
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
	}

    // get raw n value weight
	if (CheckErrorCode(errorCode) && digitValue)
    {
        refData.id = REF_DATA_ID_DIGIT_VALUE;
		// get attribute dig from adc version >= 1.40
		if ((errorCodeRefData = GetReferenceData(REF_DATA_ID_DIGIT_VALUE_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
		{
			*digitValue = refData.u.digitValue;
		}
		else
		{
			refData.id = REF_DATA_ID_RAW_VALUE;
			// get attribute rawn til adc version < 1.40
			if ((errorCodeRefData = GetReferenceData(REF_DATA_ID_RAW_VALUE_STR + "n", refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*digitValue = refData.u.raw;
		}

		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
    }

    return errorCode;
}


/**
******************************************************************************
* GetGrossWeight - function to get the gross weights from the adc
*
* @param    adcState:out				adc state
* @param    grossWeight:out				gross weight
* @param    grossWeightHighResolution:out	high resolution gross weight
* @param    digitValue:out				weight as digit value
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetGrossWeight(AdcState *adcState, AdcWeight *grossWeight, AdcWeight *grossWeightHighResolution, long *digitValue)
{
	short           errorCode;
	short			errorCodeRefData;
	keyValuePair    refDataMap;
	RefDataStruct   refData;
	string          key;
	string          value;

	refDataMap.clear();

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_GET_GROSS_WEIGHT, refDataMap);

	// get adcState
	if (CheckErrorCode(errorCode, true) && adcState)
	{
		refData.id = REF_DATA_ID_LCSTATE;
		refData.u.adcState = adcState;
		errorCodeRefData = GetReferenceData(REF_DATA_ID_LCSTATE_STR, refDataMap, refData, 1);
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
	}

	// get weight
	if (CheckErrorCode(errorCode) && grossWeight)
	{
		refData.id = REF_DATA_ID_WEIGHT;
		refData.u.wt = grossWeight;
		errorCodeRefData = GetReferenceData(REF_DATA_ID_GROSSWEIGHT_STR, refDataMap, refData, 3);
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
	}

	// get high resolution weight
	if (CheckErrorCode(errorCode) && grossWeightHighResolution)
	{
		refData.id = REF_DATA_ID_WEIGHT;
		refData.u.wt = grossWeightHighResolution;
		errorCodeRefData = GetReferenceData(REF_DATA_ID_HIGHGROSSWEIGHT_STR, refDataMap, refData, 3);
		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
	}

	// get raw n value weight
	if (CheckErrorCode(errorCode) && digitValue)
	{
		refData.id = REF_DATA_ID_DIGIT_VALUE;
		// get attribute dig from adc version >= 1.40
		if ((errorCodeRefData = GetReferenceData(REF_DATA_ID_DIGIT_VALUE_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
		{
			*digitValue = refData.u.digitValue;
		}
		else
		{
			refData.id = REF_DATA_ID_RAW_VALUE;
			// get attribute rawn til adc version < 1.40
			if ((errorCodeRefData = GetReferenceData(REF_DATA_ID_RAW_VALUE_STR + "n", refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*digitValue = refData.u.raw;
		}

		if (errorCode == LarsErr::E_SUCCESS) errorCode = errorCodeRefData;
	}

	return errorCode;
}


/**
******************************************************************************
* GetDiagnosticData - function to get diagnostic data from the adc
*
* @param    name:in			if name == EmptyString the function give all diagnostic values
* @param    sensorHealth:out	    diagnostic values
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetDiagnosticData(const char *name, map<string, AdcSensorHealth> &sensorHealth)
{
    short           errorCode;
    keyValuePair    refDataMap;
    RefDataStruct   refData;
	AdcSensorHealth structSensorHealth;

    refDataMap.clear();

    // copy reference data to map
    refData.id = REF_DATA_ID_DIAGNOSTIC;
    refData.u.dia = (char *)name;
    CreateReferenceData(refData, refDataMap);

    // send request receive respnse
    errorCode = SendRequestReceiveResponse(CMD_DIA, refDataMap);

    if (errorCode == LarsErr::E_SUCCESS)
    {
		sensorHealth.clear();

		for (keyValuePair::iterator it = refDataMap.begin(); it != refDataMap.end(); it++)
		{
			if ((*it).first != "stat")
			{
				refData.id = REF_DATA_ID_SENSOR_HEALTH;
				refData.u.sensorHealth = &structSensorHealth;
				if ((errorCode = GetReferenceData((*it).first, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
					sensorHealth[(*it).first] = structSensorHealth;
				else
					break;
			}
		}
    }

    return errorCode;
}


/**
******************************************************************************
* ConfigureDiagnosticData - function to configure adc diagnostic data (threshold,...)
*
* @param    sensorHealth:in 	diagnostic values
*
* @remarks
******************************************************************************
*/
short AdcRbs::ConfigureDiagnosticData(const AdcSensorHealth *sensorHealth)
{
	short errorCode;
	RefDataStruct   refData;
	keyValuePair    refDataMap;

	refDataMap.clear();

	// copy reference data to map
	refData.id = REF_DATA_ID_CONFIG_DIAGNOSTIC;
	refData.u.sensorHealth = (AdcSensorHealth *)sensorHealth;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_CONFIGURE_DIAGNOSTIC, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* GetCapabilities - function to read the adc capabilities
*
* @param    capMap:out		map with the capabilities
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetCapabilities(map<short, short> &capMap)
{
    short           errorCode;
    keyValuePair    refDataMap;
    stringstream    input;
    short           capNo;
    short           capValue;

    refDataMap.clear();

    // send request receive respnse
    errorCode = SendRequestReceiveResponse(CMD_GET_CAPABILITIES, refDataMap);

    if (errorCode == LarsErr::E_SUCCESS)
    {
        input >> setbase(m_base);

        // remove key stat from map
        refDataMap.erase(REF_DATA_ID_STAT_STR);

        for (keyValuePair::iterator it = refDataMap.begin(); it != refDataMap.end(); it++)
        {
            // remove first character
            input.clear();
            input << (*it).first.substr(1, (*it).first.size() - 1);
            input >> capNo;

            if (!input.fail())
            {
                input.clear();
                input << (*it).second;
                input >> capValue;
                if (!input.fail()) capMap[capNo] = capValue;
                else errorCode = LarsErr::E_INVALID_PARAMETER;
            }
            else
            {
                errorCode = LarsErr::E_INVALID_PARAMETER;
            }
        }
    }

    return errorCode;
}


/**
******************************************************************************
* SetCountrySettings - function to set the country specific settings
*
* @param    cySettingsMap:in    map with the settings
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetCountrySettings(const map<string, string> *cySettingsMap)
{
    short           errorCode = LarsErr::E_SUCCESS;
    keyValuePair    refDataMap;
	RefDataStruct   refData;

    if (cySettingsMap)
    {
		// copy reference data to map
		refData.id = REF_DATA_ID_COUNTRY_SETTINGS;
		refData.u.csp = (map<string, string> *)cySettingsMap;
		CreateReferenceData(refData, refDataMap);

        if (refDataMap.size())
        {
            // send request receive respnse
            errorCode = SendRequestReceiveResponse(CMD_SET_COUNTRY_SETTINGS, refDataMap);
        }
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* GetCountrySettings - function to get the country specific settings
*
* @param    cySettingsMap:out    map with the settings
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetCountrySettings(map<string, string> &cySettingsMap)
{
	short           errorCode = LarsErr::E_SUCCESS;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_GET_COUNTRY_SETTINGS, refDataMap);

	// get country settings
	if (errorCode == LarsErr::E_SUCCESS)
	{
		refData.id = REF_DATA_ID_COUNTRY_SETTINGS;
		refData.u.csp = &cySettingsMap;
		errorCode = GetReferenceData(REF_DATA_ID_COUNTRY_SETTINGS_STR, refDataMap, refData, COUNTRY_SPECIFIC_STRINGS_MANDATORY_COUNT);
	}

	return errorCode;
}


/**
******************************************************************************
* SetLoadCapacity - function to set the load capacity settings
*
* @param    lcSettingsMap:in    map with the settings
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::SetLoadCapacity(const map<string, string> *lcSettingsMap)
{
    short           errorCode = LarsErr::E_SUCCESS;
    keyValuePair    refDataMap;
	RefDataStruct   refData;

    if (lcSettingsMap)
    {
		// copy reference data to map
		refData.id = REF_DATA_ID_LOAD_CAPACITY_SETTINGS;
		refData.u.csp = (map<string, string> *)lcSettingsMap;
		CreateReferenceData(refData, refDataMap);

        if (refDataMap.size())
        {
            // send request receive respnse
            errorCode = SendRequestReceiveResponse(CMD_SET_LC_SETTINGS, refDataMap);
        }
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    return errorCode;
}


/**
******************************************************************************
* GetLoadCapacity - function to get the load capacity settings
*
* @param    lcSettingsMap:out    map with the settings
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetLoadCapacity(map<string, string> &cySettingsMap)
{
	short           errorCode = LarsErr::E_SUCCESS;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_GET_LC_SETTINGS, refDataMap);

	// get country settings
	if (errorCode == LarsErr::E_SUCCESS)
	{
		refData.id = REF_DATA_ID_LOAD_CAPACITY_SETTINGS;
		refData.u.lcp = &cySettingsMap;
		errorCode = GetReferenceData(REF_DATA_ID_LOAD_CAPACITY_SETTINGS_STR, refDataMap, refData, LOAD_CAPACITY_STRINGS_MANDATORY_COUNT);
	}

	return errorCode;
}


/**
******************************************************************************
* GetVersion - function to get the version of all components
*
* @param    verMap:out		version map
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetVersion(map<string, string> &verMap)
{
    short           errorCode;
    keyValuePair    refDataMap;

    refDataMap.clear();

    // send request receive respnse
    errorCode = SendRequestReceiveResponse(CMD_GET_VERSION, refDataMap);

    if (errorCode == LarsErr::E_SUCCESS)
    {
        // remove key stat from map
        refDataMap.erase(REF_DATA_ID_STAT_STR);

        if (refDataMap.size())
            verMap.insert(refDataMap.begin(), refDataMap.end());

		// get protocol version
		for (keyValuePair::iterator it = refDataMap.begin(); it != refDataMap.end(); it++)
		{
			if ((*it).first == "pno")
			{
				size_t pos = (*it).second.find('.');
				if (pos != string::npos)
				{
					m_major = atol((*it).second.substr(0, pos).c_str());
					m_minor = atol((*it).second.substr(pos, (*it).second.length() - pos).c_str());
				}
			}
		}
    }

    return errorCode;
}


/**
******************************************************************************
* Calibration - function for scale calibration
*
* @param    cmd:in			calibration commando
* @param    adcState:out    state of the adc
* @param    step:out		calibration step
* @param    calibDigit:out	weight in digit in calibration mode
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::Calibration(const AdcCalibCmd cmd, AdcState *adcState, long *step, long *calibDigit)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy reference data to map
	refData.id = REF_DATA_ID_CALIBRATION;
	refData.u.calibCmd = cmd;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_CALIB, refDataMap);

	// get adcState
	if (CheckErrorCode(errorCode, true) && adcState)
	{
		refData.id = REF_DATA_ID_LCSTATE;
		refData.u.adcState = adcState;
		errorCode = GetReferenceData(REF_DATA_ID_LCSTATE_STR, refDataMap, refData, 1);
	}

	// get raw l value weight
	if (CheckErrorCode(errorCode) && calibDigit)
	{
		refData.id = REF_DATA_ID_CALIB_DIGIT_VALUE;
		// get attribute dig from adc version >= 1.40
		if ((errorCode = GetReferenceData(REF_DATA_ID_CALIB_DIGIT_VALUE_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
		{
			*calibDigit = refData.u.calibDigit;
		}
		else
		{
			refData.id = REF_DATA_ID_RAW_VALUE;
			// get attribute rawn til adc version < 1.40
			if ((errorCode = GetReferenceData(REF_DATA_ID_RAW_VALUE_STR + "l", refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				*calibDigit = refData.u.calibDigit;
		}
	}

	// get calib step
	if (CheckErrorCode(errorCode) && step)
	{
		refData.id = REF_DATA_ID_CALIB_STEP;
		if ((errorCode = GetReferenceData(REF_DATA_ID_CALIB_STEP_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
			*step = refData.u.step;
	}

	return errorCode;
}


/**
******************************************************************************
* Parameters - send parameter mode command to adc
*
* @param	mode:in		0: reset adc parameters
*						1: save adc parameters
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::Parameters(const AdcParamMode mode)
{
	short           errorCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	refDataMap.clear();

	// copy reference data to map
	refData.id = REF_DATA_ID_PARAM_MODE;
	refData.u.paramMode = mode;
	CreateReferenceData(refData, refDataMap);

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_PARAMETER_MODE, refDataMap);

	return errorCode;
}


/**
******************************************************************************
* GetInternalDataEx - get internal adc data
*
* @param internalData:in/out	pointer of array internal data
* @param sendRequestCmd:in		send the request command (supported only for special adw versions !!)
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetInternalDataEx(AdcInternalDataEx *internalData, bool sendRequestCmd)
{
	short           errorCode = LarsErr::E_SUCCESS, retCode;
	keyValuePair    refDataMap;
	RefDataStruct   refData;
	int				idx;
	char			idStr[5];

	refDataMap.clear();

	idx = 0;
	while (internalData[idx].ID != 0)
	{
		// copy reference data to map
		refData.id = REF_DATA_ID_RAW_VALUE;
		sprintf(idStr, "raw%c", internalData[idx].ID);
		refData.u.rawID = idStr;
		CreateReferenceData(refData, refDataMap);

		idx++;
	}

	// send request receive respnse
	errorCode = SendRequestReceiveResponse(CMD_GET_INTERNAL_DATA, refDataMap, sendRequestCmd);

	if (errorCode == LarsErr::E_SUCCESS)
	{
		idx = 0;
		while (internalData[idx].ID != 0)
		{
			// get the internal data
			refData.id = REF_DATA_ID_RAW_VALUE;
			if ((retCode = GetReferenceData(REF_DATA_ID_RAW_VALUE_STR + internalData[idx].ID, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
			{
				internalData[idx].value = refData.u.raw;
				internalData[idx].valid = true;
			}
			else
			{
				internalData[idx].value = 0;
				internalData[idx].valid = false;
				if (retCode != LarsErr::E_KEY_NOT_FOUND) errorCode = retCode;
			}

			idx++;
		}
	}
	else
	{
		// error, reset complete structure to default values
		idx = 0;
		while (internalData[idx].ID != 0)
		{
			internalData[idx].value = 0;
			internalData[idx].valid = false;
			idx++;
		}
	}

	return errorCode;
}


/**
******************************************************************************

* GetEepromSize - get eeprom size
*
* @param eepromSize:out	pointer of struct AdcEepromSize
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short AdcRbs::GetEepromSize(AdcEepromSize *eepromSize)
{
	short           errorCode = LarsErr::E_SUCCESS;
	short			errorCodeRefData = LarsErr::E_SUCCESS;
	keyValuePair    refDataMap;
	RefDataStruct   refData;

	if (eepromSize)
	{
		eepromSize->openRegion = 0;
		eepromSize->welmecRegion = 0;
		eepromSize->prodRegion = 0;
		eepromSize->prodSensorsRegion = 0;

		refDataMap.clear();

		// send request receive respnse
		errorCode = SendRequestReceiveResponse(CMD_GET_SCALE_VALUE_EE_SIZE, refDataMap);

		// get eeprom size welmec region
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_EEPROM_WELMEC_SIZE;
			if ((errorCode = GetReferenceData(REF_DATA_ID_EEPROM_WELMEC_SIZE_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				if (eepromSize) eepromSize->welmecRegion = refData.u.size;
		}

		// get eeprom size open region
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_EEPROM_OPEN_SIZE;
			if ((errorCode = GetReferenceData(REF_DATA_ID_EEPROM_OPEN_SIZE_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
				if (eepromSize) eepromSize->openRegion = refData.u.size;
		}

		// get eeprom size prod region
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_EEPROM_PROD_SIZE;
			if ((errorCodeRefData = GetReferenceData(REF_DATA_ID_EEPROM_PROD_SIZE_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
			{
				if (eepromSize) eepromSize->prodRegion = refData.u.size;
			}
			else
			{
				// parameter is optional, so ignore all errors
				errorCodeRefData = LarsErr::E_SUCCESS;
			}
			errorCode = errorCodeRefData;
		}

		// get eeprom size prod sensors region
		if (errorCode == LarsErr::E_SUCCESS)
		{
			refData.id = REF_DATA_ID_EEPROM_PROD_SENSORS_SIZE;
			if ((errorCodeRefData = GetReferenceData(REF_DATA_ID_EEPROM_PROD_SENSORS_SIZE_STR, refDataMap, refData, 1)) == LarsErr::E_SUCCESS)
			{
				if (eepromSize) eepromSize->prodSensorsRegion = refData.u.size;
			}
			else
			{
				// parameter is optional, so ignore all errors
				errorCodeRefData = LarsErr::E_SUCCESS;
			}
			errorCode = errorCodeRefData;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}


/**
******************************************************************************
* CreateReferenceData - create the user data for the rbs protocol
*
* @param
* @return
* @remarks
******************************************************************************
*/
void AdcRbs::CreateReferenceData(const RefDataStruct &refData, keyValuePair &refDataMap)
{
	bool outputValid = true;
    stringstream output;

    output << setbase(m_base);

    switch (refData.id)
    {
        // reference data "tare"
        case REF_DATA_ID_TARE:
        {
            const AdcTare *tare = refData.u.tare;

            switch (tare->type)
            {
			case AdcTareType::ADC_TARE_WEIGHED:
			case AdcTareType::ADC_TARE_WEIGHED_SET_ZERO:
				output << tare->type << Lars::ESC << 0 << Lars::ESC << 0 << Lars::ESC << 0;
                break;
			case AdcTareType::ADC_TARE_KNOWN:
			case AdcTareType::ADC_TARE_LIMIT_DEVICE:
			case AdcTareType::ADC_TARE_LIMIT_CW_CUSTOM:
			case AdcTareType::ADC_TARE_LIMIT_KNOWN_CUSTOM:
			case AdcTareType::ADC_TARE_LIMIT_WEIGHED_CUSTOM:
                output << tare->type << Lars::ESC << tare->value.value << Lars::ESC << tare->value.decimalPlaces << Lars::ESC << tare->value.weightUnit;
                break;
			case AdcTareType::ADC_TARE_PERCENT:
				output << tare->type << Lars::ESC << tare->percentage.value << Lars::ESC << tare->percentage.decimalPlaces << Lars::ESC << tare->percentage.unit;
            	break;
			case AdcTareType::ADC_TARE_NO:
				outputValid = false;
				break;
            }
			
			if (outputValid == true)
				refDataMap[REF_DATA_ID_TARE_STR] = output.str();
            break;
        }

        // reference data "tare priority"
        case REF_DATA_ID_TARE_PRIO:
        {
            output << refData.u.tp;
            refDataMap[REF_DATA_ID_TARE_PRIO_STR] = output.str();
            break;
        }

        // reference data "base price"
        case REF_DATA_ID_BASEPRICE:
        {
            output << refData.u.bp->price.value << Lars::ESC << refData.u.bp->price.currency << Lars::ESC << refData.u.bp->price.decimalPlaces << Lars::ESC << refData.u.bp->weightUnit;
            refDataMap[REF_DATA_ID_BASEPRICE_STR] = output.str();
            break;
        }

        // reference data "display text"
        case REF_DATA_ID_DISPLAYTEXT:
        {
            output << refData.u.dt->x << Lars::ESC << refData.u.dt->y << Lars::ESC << refData.u.dt->text;
            refDataMap[REF_DATA_ID_DISPLAYTEXT_STR] = output.str();
            break;
        }

        // reference data "scale mode"
        case REF_DATA_ID_SCALEMODE:
        {
            output << refData.u.sm;
            refDataMap[REF_DATA_ID_SCALEMODE_STR] = output.str();
            break;
        }

        // reference data "eeprom"
        case REF_DATA_ID_EEPROM:
        {
            //convert data to hex ascii
            string hexAscii;
            if (refData.u.ee->direction == 1)
                StringSource(refData.u.ee->data, refData.u.ee->len, true, new HexEncoder(new StringSink(hexAscii)));
            output << refData.u.ee->region << Lars::ESC << refData.u.ee->startAdr << Lars::ESC << refData.u.ee->len << Lars::ESC << hexAscii;
            refDataMap[REF_DATA_ID_EEPROM_STR] = output.str();
            break;
        }

        // reference data "tilt compensation"
        case REF_DATA_ID_TILT_COMP:
        {
			output << refData.u.tc->state.all << Lars::ESC << Lars::ESC << Lars::ESC << refData.u.tc->limit_angle_1;
            refDataMap[REF_DATA_ID_TILT_COMP_STR] = output.str();
            break;
        }

        // reference data "registration request"
        case REF_DATA_ID_REGISTRATION_REQ:
        {
            output << refData.u.rr;
            refDataMap[REF_DATA_ID_REGISTRATION_REQ_STR] = output.str();
            break;
        }

        // reference data "logbook entry"
        case REF_DATA_ID_LOGBOOK:
        {
            output << refData.u.idx;
            refDataMap[REF_DATA_ID_LOGBOOK_STR] = output.str();
            break;
        }

        // reference data "authentication"
        case REF_DATA_ID_AUTHENTICATION:
        {
			// convert chksum in hex ascii
			output << setbase(16);
			output << std::setfill('0') << std::setw(4) << refData.u.au->chksum;
            refDataMap[REF_DATA_ID_AUTHENTICATION_STR] = output.str();

            if (refData.u.au->swIdentity)
            {
                keyValuePair keyValueMap;
                keyValuePair dummyMap;

                if (GetKeyValuePair(RBS_REFERENCE_DATA, string(refData.u.au->swIdentity), dummyMap, keyValueMap) == LarsErr::E_SUCCESS)
                {
                    refDataMap.insert(keyValueMap.begin(), keyValueMap.end());
                }
            }
            break;
        }

        // reference data "diagnostic"
        case REF_DATA_ID_DIAGNOSTIC:
        {
            if (refData.u.dia)
            {
                keyValuePair keyValueMap;
                keyValuePair dummyMap;

                if (GetKeyValuePair(RBS_REFERENCE_DATA, string(refData.u.dia), dummyMap, keyValueMap) == LarsErr::E_SUCCESS)
                {
                    refDataMap.insert(keyValueMap.begin(), keyValueMap.end());
                }
            }
            break;
        }

		// reference data "sensor health"
		case REF_DATA_ID_CONFIG_DIAGNOSTIC:
		{
			if (refData.u.sensorHealth)
			{
				switch (refData.u.sensorHealth->type)
				{
				case 100:
					output << refData.u.sensorHealth->type;
					for (int idx = 0; idx < refData.u.sensorHealth->Health.type100.number; idx++)
						output << Lars::ESC << refData.u.sensorHealth->Health.type100.ValueUnit[idx].value << Lars::ESC << refData.u.sensorHealth->Health.type100.ValueUnit[idx].unit;
					refDataMap[string(refData.u.sensorHealth->diaParam)] = output.str();
					break;
				}
			}
			break;
		}

		// reference data "tare type"
		case REF_DATA_ID_TARE_TYPE:
		{
			output << refData.u.tare->type;
			refDataMap[REF_DATA_ID_TARE_TYPE_STR] = output.str();
			break;
		}

		// reference data "constant tare"
		case REF_DATA_ID_CONST_TARE:
		{
			output << refData.u.tare->frozen;
			refDataMap[REF_DATA_ID_CONST_TARE_STR] = output.str();
			break;
		}

		// reference data "constant tare"
		case REF_DATA_ID_CONST_BASEPRICE:
		{
			output << refData.u.frozen;
			refDataMap[REF_DATA_ID_CONST_BASEPRICE_STR] = output.str();
			break;
		}


		// reference data "country specific settings"
		case REF_DATA_ID_COUNTRY_SETTINGS:
		{
			bool	firstValue = true;;
			string	value;

			for (int i = 0; i < COUNTRY_SPECIFIC_STRINGS_COUNT; i++)
			{
				if (!firstValue) output << Lars::ESC;
				if (Helpers::KeyExists(*refData.u.csp, COUNTRY_SPECIFIC_STRINGS[i], value))
				{
					output << value;
				}
				firstValue = false;
			}

			refDataMap[REF_DATA_ID_COUNTRY_SETTINGS_STR] = output.str();
			break;
		}

		// reference data "country specific settings"
		case REF_DATA_ID_LOAD_CAPACITY_SETTINGS:
		{
			bool	firstValue = true;;
			string	value;

			for (int i = 0; i < LOAD_CAPACITY_STRINGS_COUNT; i++)
			{
				if (!firstValue) output << Lars::ESC;
				if (Helpers::KeyExists(*refData.u.lcp, LOAD_CAPACITY_STRINGS[i], value))
				{
					output << value;
				}
				firstValue = false;
			}

			refDataMap[REF_DATA_ID_LOAD_CAPACITY_SETTINGS_STR] = output.str();
			break;
		}

		// reference data "calibration"
		case REF_DATA_ID_CALIBRATION:
		{
			output << refData.u.calibCmd;
			refDataMap[REF_DATA_ID_CALIBRATION_STR] = output.str();
			break;
		}

		// reference data "filter index"
		case REF_DATA_ID_FILTER_IDX:
		{
			output << refData.u.filter->idx;
			refDataMap[REF_DATA_ID_FILTER_IDX_STR] = output.str();
			break;
		}

		// reference data "offset stable time"
		case REF_DATA_ID_OFFSET_STABLE_TIME:
		{
			output << refData.u.filter->offsetStableTime;
			refDataMap[REF_DATA_ID_OFFSET_STABLE_TIME_STR] = output.str();
			break;
		}

		// reference data "stable range"
		case REF_DATA_ID_STABLE_RANGE:
		{
			output << refData.u.filter->stableRange;
			refDataMap[REF_DATA_ID_STABLE_RANGE_STR] = output.str();
			break;
		}

		// reference data "parameter mode"
		case REF_DATA_ID_PARAM_MODE:
		{
			output << refData.u.paramMode;
			refDataMap[REF_DATA_ID_PARAM_MODE_STR] = output.str();
			break;
		}

		// reference data "raw value"
		case REF_DATA_ID_RAW_VALUE:
		{
			// output string is empty
			refDataMap[refData.u.rawID] = output.str();
			break;
		}

		// reference data "reset type"
		case REF_DATA_ID_RESET_TYPE:
		{
			output << refData.u.resetType;
			refDataMap[REF_DATA_ID_RESET_TYPE_STR] = output.str();
			break;
		}

		// reference data "g factor"
		case REF_DATA_ID_GFACTOR:
		{
			output << refData.u.gFactor;
			refDataMap[REF_DATA_ID_GFACTOR_STR] = output.str();
			break;
		}

		// reference data "production site ID"
		case REF_DATA_ID_PROD_SITE:
		{
			output << refData.u.prodSiteID;
			refDataMap[REF_DATA_ID_PROD_SITE_STR] = output.str();
			break;
		}

		// reference data "g factor offset"
		case REF_DATA_ID_PROD_GFACTOR_OFFSET:
		{
			output << refData.u.prodSiteID;
			refDataMap[REF_DATA_ID_PROD_GFACTOR_OFFSET_STR] = output.str();
			break;
		}

		// reference data "date"
		case REF_DATA_ID_PROD_DATE:
		{
			output << refData.u.dateStr;
			refDataMap[REF_DATA_ID_PROD_DATE_STR] = output.str();
			break;
		}

		// reference data "wsc"
		case REF_DATA_ID_PROD_WSC:
		{
			output << refData.u.wsc;
			refDataMap[REF_DATA_ID_PROD_WSC_STR] = output.str();
			if (refDataMap[REF_DATA_ID_PROD_WSC_STR].length() < 20)
			{
				// fill string with spaces
				refDataMap[REF_DATA_ID_PROD_WSC_STR].append(sizeof(refData.u.wsc) - refDataMap[REF_DATA_ID_PROD_WSC_STR].length() - 1, ' ');
			}
			break;
		}

		// reference data "conversion weight unit"
		case REF_DATA_ID_CONVERSION_WEIGHT_UNIT:
		{
			output << refData.u.conversionWeightUnit;
			refDataMap[REF_DATA_ID_CONVERSION_WEIGHT_UNIT_STR] = output.str();
			break;
		}

		// reference data "interface auto detection"
		case REF_DATA_ID_INTERFACE_AUTO_DETECTION:
		{
			output << refData.u.interfaceAutoDetection;
			refDataMap[REF_DATA_ID_INTERFACE_AUTO_DETECTION_STR] = output.str();
			break;
		}

		// reference data "interface mode"
		case REF_DATA_ID_INTERFACE_MODE:
		{
			output << refData.u.interfaceMode;
			refDataMap[REF_DATA_ID_INTERFACE_MODE_STR] = output.str();
			break;
		}

		// reference data "zero point tracking"
		case REF_DATA_ID_ZERO_POINT_TRACKING:
		{
			output << refData.u.mode;
			refDataMap[REF_DATA_ID_ZERO_POINT_TRACKING_STR] = output.str();
			break;
		}

		// reference data "zero setting interval"
		case REF_DATA_ID_ZERO_SETTING_INTERVAL:
		{
			output << refData.u.zeroSettingInterval;
			refDataMap[REF_DATA_ID_ZERO_SETTING_INTERVAL_STR] = output.str();
			break;
		}

		// reference data "automatic zero setting time"
		case REF_DATA_ID_AUTOMATIC_ZERO_SETTING_TIME:
		{
			output << refData.u.automaticZeroSettingTime;
			refDataMap[REF_DATA_ID_AUTOMATIC_ZERO_SETTING_TIME_STR] = output.str();
			break;
		}

		// reference data "verif parameter protected"
		case REF_DATA_ID_VERIF_PARAM_PROTECTED:
		{
			output << refData.u.verifParamProtected;
			refDataMap[REF_DATA_ID_VERIF_PARAM_PROTECTED_STR] = output.str();
			break;
		}

		// reference data "update allowed"
		case REF_DATA_ID_UPDATE_ALLOWED:
		{
			output << refData.u.verifParamProtected;
			refDataMap[REF_DATA_ID_UPDATE_ALLOWED_STR] = output.str();
			break;
		}

		// reference data "operating mode"
		case REF_DATA_ID_OPERATING_MODE:
		{
			output << refData.u.opMode;
			refDataMap[REF_DATA_ID_OPERATING_MODE_STR] = output.str();
			break;
		}

		// reference data "firmware update command"
		case REF_DATA_ID_FRM_COMMAND:
		{
			output << refData.u.frmCmd;
			refDataMap[REF_DATA_ID_FRM_COMMAND_STR] = output.str();
			break;
		}

		// reference data "firmware data"
		case REF_DATA_ID_FRM_DATA:
		{
			//convert data to hex ascii
			string hexAscii;
			StringSource(refData.u.frmData->data, refData.u.frmData->len, true, new HexEncoder(new StringSink(hexAscii)));
			output << refData.u.frmData->startAdr << Lars::ESC << refData.u.frmData->len << Lars::ESC << hexAscii;
			refDataMap[REF_DATA_ID_FRM_DATA_STR] = output.str();
			break;
		}

		// reference data "firmware usb stack version"
		case REF_DATA_ID_FRM_USBSTACK_VERS:
		{
			output << refData.u.frmStringData;
			refDataMap[REF_DATA_ID_FRM_USBSTACK_VERS_STR] = output.str();
			break;
		}

		// reference data "firmware welmec structure version"
		case REF_DATA_ID_FRM_WELMECSTRUCT_VERS:
		{
			output << refData.u.frmStringData;
			refDataMap[REF_DATA_ID_FRM_WELMECSTRUCT_VERS_STR] = output.str();
			break;
		}

		// reference data "firmware welmec structure version"
		case REF_DATA_ID_FRM_FORCE:
		{
			output << refData.u.frmForce;
			refDataMap[REF_DATA_ID_FRM_FORCE_STR] = output.str();
			break;
		}

		// reference data "firmware date"
		case REF_DATA_ID_FRM_DATE:
		{
			output << refData.u.frmStringData;
			refDataMap[REF_DATA_ID_FRM_DATE_STR] = output.str();
			break;
		}

		// reference data "tcc settings"
		case REF_DATA_ID_TILT_TCC:
		{
			bool	firstValue = true;;
			string	value;

			for (int i = 0; i < TCC_STRINGS_COUNT; i++)
			{
				if (!firstValue) output << Lars::ESC;
				if (Helpers::KeyExists(*refData.u.tcc, TCC_STRINGS[i], value))
				{
					// patch for float
					if (TCC_STRINGS[i].find("FLOAT") != string::npos)
					{
						value = ConvertFloatIEEToInt(value);
					}
					output << value;
				}
				firstValue = false;
			}

			refDataMap[REF_DATA_ID_TILT_TCC_STR] = output.str();
			break;
		}

		// reference data "lin settings"
		case REF_DATA_ID_TILT_LIN:
		{
			bool	firstValue = true;;
			string	value;

			for (int i = 0; i < LIN_STRINGS_COUNT; i++)
			{
				if (!firstValue) output << Lars::ESC;
				if (Helpers::KeyExists(*refData.u.tcc, LIN_STRINGS[i], value))
				{
					// patch for float
					if (LIN_STRINGS[i].find("FLOAT") != string::npos)
					{
						value = ConvertFloatIEEToInt(value);
					}
					output << value;
				}
				firstValue = false;
			}

			refDataMap[REF_DATA_ID_TILT_LIN_STR] = output.str();
			break;
		}

		// reference data "wdta settings"
		case REF_DATA_ID_TILT_WDTA:
		{
			bool	firstValue = true;;
			string	value;

			for (int i = 0; i < WDTA_STRINGS_COUNT; i++)
			{
				if (!firstValue) output << Lars::ESC;
				if (Helpers::KeyExists(*refData.u.wdta, WDTA_STRINGS[i], value))
				{
					// patch for float
					if (WDTA_STRINGS[i].find("FLOAT") != string::npos)
					{
						value = ConvertFloatIEEToInt(value);
					}
					output << value;
				}
				firstValue = false;
			}

			refDataMap[REF_DATA_ID_TILT_WDTA_STR] = output.str();
			break;
		}

		// reference data "spirit level"
		case REF_DATA_ID_SPIRIT_LEVEL:
		{
			output << refData.u.spiritLevelCmd;
			refDataMap[REF_DATA_ID_SPIRIT_LEVEL_STR] = output.str();
			break;
		}

		// reference data "warm up time"
		case REF_DATA_ID_WARM_UP_TIME:
		{
			output << refData.u.warmUpTime;
			refDataMap[REF_DATA_ID_WARM_UP_TIME_STR] = output.str();
			break;
		}

		// reference data "initial zero setting"
		case REF_DATA_ID_INITIAL_ZERO_SETTING:
		{
			output << refData.u.initialZeroSettingParam->lowerLimit << Lars::ESC << refData.u.initialZeroSettingParam->upperLimit;
			refDataMap[REF_DATA_ID_INITIAL_ZERO_SETTING_STR] = output.str();
			break;
		}

		// reference data "scale specific settings"
		case REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL:
		{
			bool	firstValue = true;;
			string	value;

			for (int i = 0; i < SSP_STRINGS_COUNT; i++)
			{
				if (!firstValue) output << Lars::ESC;
				if (Helpers::KeyExists(*refData.u.ssp, SSP_STRINGS[i], value))
				{
					// patch for float
					if (SSP_STRINGS[i].find("FLOAT") != string::npos)
					{
						value = ConvertFloatIEEToInt(value);
					}
					output << value;
				}
				firstValue = false;
			}

			refDataMap[REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL_STR] = output.str();
			break;
		}

		// reference data "scale specific settings sealed"
		case REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL_SEALED:
		{
			bool	firstValue = true;;
			string	value;

			for (int i = 0; i < SSPS_STRINGS_COUNT; i++)
			{
				if (!firstValue) output << Lars::ESC;
				if (Helpers::KeyExists(*refData.u.ssps, SSPS_STRINGS[i], value))
				{
					// patch for float
					if (SSPS_STRINGS[i].find("FLOAT") != string::npos)
					{
						value = ConvertFloatIEEToInt(value);
					}
					output << value;
				}
				firstValue = false;
			}

			refDataMap[REF_DATA_ID_SCALE_SPECIFIC_SETTINGS_GENERAL_SEALED_STR] = output.str();
			break;
		}

		// reference data "scale model"
		case REF_DATA_ID_SCALE_MODEL:
		{
			refDataMap[REF_DATA_ID_SCALE_MODEL_STR] = *refData.u.scaleModel;
			break;
		}

		// reference data "state automatic tilt sensor"
		case REF_DATA_ID_STATE_AUTOMATIC_TILT_SENSOR:
		{
			output << refData.u.state;
			refDataMap[REF_DATA_ID_STATE_AUTOMATIC_TILT_SENSOR_STR] = output.str();
			break;
		}

		// reference data "tilt compensation checked"
		case REF_DATA_ID_TILT_COMP_CHECKED:
		{
			output << refData.u.tiltAngleConfirmCmd;
			refDataMap[REF_DATA_ID_TILT_COMP_CHECKED_STR] = output.str();
			break;
		}

	}
}


/**
******************************************************************************
* CreateTelegram - create the rbs telegram
*
* @param
* @return
* @remarks
******************************************************************************
*/
short AdcRbs::CreateTelegram(const string &cmd, const keyValuePair &refDataMap, string &telegram, bool crc16, bool createNewOrderID, short previousOrderID, unsigned long flag)
{
    stringstream    output;
    bool            bFirst = true;
    stringstream    length;
    short           orderID;

    output << setbase(m_base);
	if (createNewOrderID)
		orderID = GetOrderID();
	else
		orderID = previousOrderID;
	output << Lars::ESC << orderID;
	
	if (flag) output << Lars::ESC << flag;
		
	output << Lars::GS << cmd;

    if (refDataMap.size())
    {
        output << Lars::STX;
        for (keyValuePair::const_iterator it = refDataMap.begin(); it != refDataMap.end(); ++it)
        {
            if (!bFirst)
            {
                output << Lars::GS;
            }

            output << (*it).first;
            if (!(*it).second.empty()) output << Lars::FS << (*it).second;

            bFirst = false;
        }
		output << Lars::ETX;
		
		if (crc16) output << CalculateChecksum(output.str());
    }
	else
	{
		if (crc16)
		{
			output << Lars::GS;
			output << CalculateChecksum(output.str());
		}
	}
    
	output << Lars::ETB;

    length << setbase(m_base);
	length << output.str().size();
	telegram = Lars::SOH + length.str() + output.str();

    return orderID;
}

/**
******************************************************************************
* GetOrderID - create the order ID
*
* @param
* @return
* @remarks
******************************************************************************
*/
short AdcRbs::GetOrderID()
{
	// orderID from 1 - 99
	if (m_orderID > 98)
	{
		m_orderID = 0;
	}
	
	m_orderID++;

	return m_orderID;
}


/**
******************************************************************************
* CalculateChecksum - calculate the telegram checksum
*
* @param
* @return
* @remarks
******************************************************************************
*/
unsigned short AdcRbs::CalculateChecksum(string str)
{
	Authentication crc16(RBS_CRC_START, RBS_CRC_POLY, RBS_CRC_MASK);

	return crc16.CalcCrc((unsigned char *)str.c_str(), str.length());
}


/**
******************************************************************************
* CheckChecksum - check the telegram checksum
*
* @param
* @return
* @remarks
******************************************************************************
*/
short AdcRbs::CheckChecksum(string str, string crc)
{
	short errorCode = LarsErr::E_SUCCESS;
	size_t start = 0;
	size_t end = 0;
	unsigned short crcNew;

	// index of first ESC
	start = str.find(Lars::ESC);
	// check if telegram contains user data
	if ((end = str.find_last_of(Lars::ETX)) == string::npos)
	{
		// no user data, get pos from last GS
		end = str.find_last_of(Lars::GS);
	}

	if ((start != string::npos) && (end != string::npos))
	{
		crcNew = CalculateChecksum(str.substr(start, end - start + 1));

		if (crcNew != atoi(crc.c_str()))
			errorCode = LarsErr::E_PROTOCOL_CRC;
	}
	else
	{
		errorCode = LarsErr::E_PROTOCOL_CRC;
	}

	return errorCode;
}


/**
******************************************************************************
* SendRequest - send an request to the ADC
*
* @param    telegram:in			prepared rbs telegram
* @param	sendRequestCmd:in	send the request command (supported only for special adw versions !!)
* @return
* @remarks
******************************************************************************
*/
short AdcRbs::SendRequestReceiveResponse(const string &cmd, keyValuePair &keyValueMap, bool sendRequestCmd)
{
    short           errorCode = LarsErr::E_SUCCESS;
    string          telegram;
    string          response;
    short           orderID = 0;
    short           orderIDResponse = 0;
	string          cmdResponse = "";
    unsigned long   bytesWritten;
    keyValuePair    headerMap;
	keyValuePair    refDataMap;
    string          key;
    string          value;
    RefDataStruct   refData;
    short           stat;
	short			retries;
	bool			createNewOrderID;
	unsigned long   flag = 0;
	short			timeoutOffset = 0;
	bool			cmdOrderIdOk;

    if (m_interface)
    {

		retries = 0;
		do
		{
			if (retries == 0)
			{
				// new telegram, create new order ID
				createNewOrderID = true;
			}
			else
			{
				// connection error to adc, retry, use previous orderID
				createNewOrderID = false;

				flag |= FLAG_TELEGRAM_REPEAT;
			}

			if (sendRequestCmd)
			{
				// reset error code
				errorCode = LarsErr::E_SUCCESS;

				// create telegram
				orderID = CreateTelegram(cmd, keyValueMap, telegram, m_interface->UseCRC16(), createNewOrderID, orderID, flag);

				g_adcTrace.Trace(AdcTrace::TRC_INFO, "%s\tAPPL -> ADC %s", __FUNCTION__, telegram.c_str());
				// send telegram 
				bytesWritten = m_interface->Write((void *)telegram.c_str(), telegram.size());
				if (bytesWritten != telegram.size())
				{
					errorCode = LarsErr::E_ADC_ERROR;
				}
			}

			if (errorCode != LarsErr::E_ADC_ERROR)
			{
				errorCode = LarsErr::E_SUCCESS;

				do
				{
					// receive response
					if (errorCode == LarsErr::E_SUCCESS)
					{
#ifdef  __GNUC__
						// under linux the read/write access is asynchron, so wait an additional timeout of 0ms
						// before read the response
						std::this_thread::sleep_for(std::chrono::milliseconds(0));
#endif
						if (cmd == CMD_ADC_RESET) timeoutOffset = 2;
						errorCode = ReceiveResponse(response, timeoutOffset);
					}

					// parse response
					if (errorCode == LarsErr::E_SUCCESS)
					{
						errorCode = GetKeyValuePair(RBS_FULL_TELEGRAM, response, headerMap, refDataMap);
					}

					if (errorCode == LarsErr::E_SUCCESS)
					{
						// check if checksum exits and if checksum is ok
						key = HEADER_CRC16;
						if (Helpers::KeyExists(headerMap, key, value))
						{
							errorCode = CheckChecksum(response, value);
						}
					}

					if (errorCode == LarsErr::E_SUCCESS)
					{
						// get orderID from response
						key = HEADER_ORDERID;
						if (Helpers::KeyExists(headerMap, key, value))
						{
							istringstream input(value);
							input >> setbase(m_base);
							input >> orderIDResponse;

							if (orderID != orderIDResponse)
								g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\torderID %d  responseID %d", __FUNCTION__, orderID, orderIDResponse);
						}
						else
						{
							errorCode = LarsErr::E_PROTOCOL;
						}
					}

					if (errorCode == LarsErr::E_SUCCESS)
					{
						// get cmd from response
						key = HEADER_CMD;
						if (Helpers::KeyExists(headerMap, key, value))
						{
							cmdResponse = value;

							if (cmd != cmdResponse)
								g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tcmd %s  cmdResponse %s", __FUNCTION__, cmd.c_str(), cmdResponse.c_str());
						}
						else
						{
							errorCode = LarsErr::E_PROTOCOL;
						}
					}

					if (errorCode == LarsErr::E_SUCCESS)
					{
						// check status for protocol crc error
						refData.id = REF_DATA_ID_STAT;
						if (GetReferenceData(REF_DATA_ID_STAT_STR, refDataMap, refData, 1) == LarsErr::E_SUCCESS)
						{
							stat = refData.u.stat;
							if (stat == ADC_ERROR_PROTOCOL_CRC)
							{
								errorCode = LarsErr::E_PROTOCOL_CRC;
								g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tadc receives telegram with crc error", __FUNCTION__);
							}
						}
						else
						{
							errorCode = LarsErr::E_PROTOCOL;
						}
					}

					// check cmd and orderID
					cmdOrderIdOk = true;
					if (sendRequestCmd && ((cmd != cmdResponse) || (orderID != orderIDResponse)))
						cmdOrderIdOk = false;

				} while ((errorCode == LarsErr::E_SUCCESS) && (cmdOrderIdOk == false));
			}

			// on error try to reconnect to adc
			if (((errorCode == LarsErr::E_ADC_ERROR) || (errorCode == LarsErr::E_ADC_TIMEOUT)) && (retries < TIMEOUT_RETRIES_ATTEMPTS - 1))
			{
				if (!m_interface->Reconnect())
				{
					errorCode = LarsErr::E_NO_DEVICE;
				}
			}

		} while (((errorCode == LarsErr::E_ADC_ERROR) || (errorCode == LarsErr::E_ADC_TIMEOUT) || (errorCode == LarsErr::E_PROTOCOL_CRC) || (errorCode == LarsErr::E_PROTOCOL)) 
				   && (++retries < TIMEOUT_RETRIES_ATTEMPTS));

		keyValueMap.clear();
		if (errorCode == LarsErr::E_SUCCESS)
		{
			// copy content of refDataMap to keyValueMap
			keyValueMap.insert(refDataMap.begin(), refDataMap.end());
		}

        // get state
        if (errorCode == LarsErr::E_SUCCESS)
        {
			// map adc stat error to library error
			switch (stat)
			{
				case ADC_CMD_SUCCESSFUL:			errorCode = LarsErr::E_SUCCESS; break;
				case ADC_CMD_NOT_EXECUTED:			errorCode = LarsErr::E_COMMAND_NOT_EXECUTED; break;
				case ADC_INVALID_PARAMETER:			errorCode = LarsErr::E_INVALID_PARAMETER; break;
				case ADC_FUNCTION_NOT_IMPLEMENTED:	errorCode = LarsErr::E_FUNCTION_NOT_IMPLEMENTED; break;
				case ADC_ERROR_AUTHENTICATION:		errorCode = LarsErr::E_AUTHENTICATION; break;
				case ADC_COMMAND_NOT_SUPPORTED_IN_THIS_OPERATING_MODE:	errorCode = LarsErr::E_COMMAND_NOT_SUPPORTED_IN_THIS_OPERATING_MODE; break;
				case ADC_LOGBOOK_NO_FURTHER_ENTRY:	errorCode = LarsErr::E_LOGBOOK_NO_FURTHER_ENTRY; break;
				case ADC_LOGBOOK_FULL:				errorCode = LarsErr::E_LOGBOOK_FULL; break;
				case ADC_ERROR_TILT_COMPENSATION_SWITCH_ON:				errorCode = LarsErr::E_TILT_COMPENSATION_SWITCH_ON; break;
				case ADC_ERROR_CHKSUM_BLOCK_3_4:	errorCode = LarsErr::E_CHKSUM_BLOCK_3_4; break;
				case ADC_ERROR_PROTOCOL_CRC:		errorCode = LarsErr::E_PROTOCOL_CRC; break;
				case ADC_ERROR_LINEAR_CALIBRATION:	errorCode = LarsErr::E_LINEAR_CALIBRATION; break;
				case ADC_ERROR_EEPROM_ACCESS_VIOLATION:					errorCode = LarsErr::E_EEPROM_ACCESS_VIOLATION; break;
				case ADC_ERROR_CMD_ONLY_FOR_SEPARATE_LOADCELL:			errorCode = LarsErr::E_COMMAND_ONLY_FOR_SEPARATE_LOADCELL; break;
				case ADC_ERROR_UPDATE_NOT_ALLOWED:	errorCode = LarsErr::E_UPDATE_NOT_ALLOWED; break;
				case ADC_ERROR_TCC_SWITCH_ON:		errorCode = LarsErr::E_TCC_SWITCH_ON; break;
				case ADC_ERROR_INCOMPATIBLE_PROD_DATA:					errorCode = LarsErr::E_INCOMPATIBLE_PROD_DATA; break;
				case ADC_ERROR_TARE_OCCUPIED:		errorCode = LarsErr::E_TARE_OCCUPIED; break;
				case ADC_ERROR_KNOWN_TARE_LESS_E:	errorCode = LarsErr::E_KNOWN_TARE_LESS_E; break;
				case ADC_ERROR_BATCH_TARE_NOT_ALLOWED:					errorCode = LarsErr::E_BATCH_TARE_NOT_ALLOWED; break;
				case ADC_ERROR_TARE_OUT_OF_RANGE:	errorCode = LarsErr::E_TARE_OUT_OF_RANGE; break;
				case ADC_ERROR_TARE_OUTSIDE_CLEARING_AREA:				errorCode = LarsErr::E_TARE_OUTSIDE_CLEARING_AREA; break;
				case ADC_ERROR_WS_NOT_SUPPORT_LOAD_CAPACITY:			errorCode = LarsErr::E_WS_NOT_SUPPORT_LOAD_CAPACITY; break;
				case ADC_ERROR_CHKSUM_FIRMWARE:		errorCode = LarsErr::E_CHKSUM_FIRMWARE; break;
				default: errorCode = LarsErr::E_ADC_ERROR; break;
			}
        }
    }

    return errorCode;
}


short AdcRbs::ReceiveResponse(string &response, short timeoutOffset)
{
    short   errorCode = LarsErr::E_SUCCESS;
    short   modulState = WAIT_FOR_SOH;
    short   retry = 0;
    char    receiveByte;
    unsigned long length = 0;
	unsigned long currentReceived = 0;
    short   index = 0;
    short   idxLenStart = 0, idxLenEnd = 0;
    bool    bReceiveOK = false;

    response.clear();

	while (!bReceiveOK && (errorCode == LarsErr::E_SUCCESS) && (retry < (TIMEOUT_RECEIVE + timeoutOffset)))
    {
        switch (modulState)
        {
			case WAIT_FOR_SOH:
				if (m_interface->Read(&receiveByte, 1) == 1)
				{
					if (receiveByte == Lars::SOH)
					{
						// reset index
						index = 0;
						idxLenStart = 0;
						idxLenEnd = 0;

						// store byte in puffer
						m_receiveBuffer[index++] = receiveByte;

						// next state
						modulState = WAIT_FOR_LENGTH;
					}
				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					retry++;
				}
				break;

            case WAIT_FOR_LENGTH:
                if (m_interface->Read(&receiveByte, 1) == 1)
                {
					// check if adc send SOH again
					if (receiveByte == Lars::SOH)
					{
						// make resynchronization, reset index
						index = 0;
						idxLenStart = 0;
						idxLenEnd = 0;

						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\treceive SOH while waiting for protocol length, make resynchronization", __FUNCTION__);
					}
					else if (idxLenStart == 0)
					{
						// set length start index
						idxLenStart = index;
					}

                    // store byte in puffer
                    m_receiveBuffer[index++] = receiveByte;

                    if ((receiveByte == Lars::ESC) || (receiveByte == Lars::GS))
                    {
                        // convert lenght to long
						length = strtol(&m_receiveBuffer[idxLenStart], NULL, 10);
						if ((!length) || (length > AdcProtocol::RECEIVE_BUFFER_SIZE - 1 - index))
                        {
                            // protocol error, try to resync to SOH
							errorCode = LarsErr::E_PROTOCOL;

							m_receiveBuffer[index] = '\0';
							g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tprotocol error, receive length larger than buffer  ADC -> APPL %s", __FUNCTION__, m_receiveBuffer);
                        }
                        else
                        {
                            // decrement length because the character after length is already received
                            length--;
                            idxLenEnd = index;
                            modulState = READ_REMAINING_DATA;
                        }
                    }
                    else if ((index - idxLenStart) > 8)
                    {
						// protocol error
						errorCode = LarsErr::E_PROTOCOL;

						m_receiveBuffer[index] = '\0';
						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tprotocol error, receive length too large  ADC -> APPL %s", __FUNCTION__, m_receiveBuffer);
                    }
                }
                else
                {
                    // protocol error, try to resync to SOH
                    modulState = WAIT_FOR_SOH;
					retry++;
                }
                break;

            case READ_REMAINING_DATA:
                if ((currentReceived = m_interface->Read(&m_receiveBuffer[idxLenEnd], length)) == length)
                {
                    // check for ETB
                    if (m_receiveBuffer[idxLenEnd + length - 1] != Lars::ETB)
                    {
                        // protocol error
						errorCode = LarsErr::E_PROTOCOL;

						m_receiveBuffer[idxLenEnd + length] = '\0';
						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tprotocol error, check for ETB failed  ADC -> APPL %s", __FUNCTION__, m_receiveBuffer);
					}
                    else
                    {
                        m_receiveBuffer[idxLenEnd + length] = '\0';
                        bReceiveOK = true;

						errorCode = LarsErr::E_SUCCESS;
                    }
                }
                else
                {
                    // protocol error
					errorCode = LarsErr::E_PROTOCOL;

					m_receiveBuffer[idxLenEnd + currentReceived] = '\0';
					g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tprotocol error, receive user data length expected: 0x%X  received: 0x%X  ADC -> APPL %s", __FUNCTION__, length, currentReceived, m_receiveBuffer);
                }
                break;
        }
    }

    if (!bReceiveOK)
    {
		if ((retry == TIMEOUT_RECEIVE) && (errorCode == LarsErr::E_SUCCESS))
		{
			m_interface->ResetInterface();
			errorCode = LarsErr::E_ADC_TIMEOUT;
		}
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tprotocol error: 0x%X", __FUNCTION__, errorCode);
    }
    else
    {
        response = string(m_receiveBuffer);
		g_adcTrace.Trace(AdcTrace::TRC_INFO, "%s\tADC -> APPL %s", __FUNCTION__, response.c_str());
    }

    return errorCode;
}

short AdcRbs::GetKeyValuePair(ProtocolType type, const string &keyValueStr, keyValuePair &headerMap, keyValuePair &refDataMap)
{
    short moduleState   = CHECK_FOR_SOH;
    bool endWithEtb     = false;
    bool endWithEtx     = false;
    int headerIdx       = 0;
    int index           = 0;
    int keyBegin        = 0;
    int keyEnd          = 0;
    int valueBegin      = 0;

    refDataMap.clear();
    headerMap.clear();

    if (type == RBS_FULL_TELEGRAM) 
        moduleState = CHECK_FOR_SOH;
    if (type == RBS_REFERENCE_DATA)
        moduleState = CHECK_FOR_STX;

    while (keyValueStr[index] && !endWithEtb && !endWithEtx)
    {
		switch (moduleState)
		{
		case CHECK_FOR_SOH:
			// check for soh
			if (keyValueStr[index] != Lars::SOH)
			{
				// protocol error, return error
				refDataMap.clear();
				headerMap.clear();

				g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tprotocol error, SOH expected [%s]", __FUNCTION__, keyValueStr.c_str());
				return LarsErr::E_INVALID_PARAMETER;
			}
			index++;
			moduleState = PARSE_HEADER;
			break;

		case CHECK_FOR_STX:
			// check for stx
			if (keyValueStr[index] != Lars::STX)
			{
				// protocol error, return error
				refDataMap.clear();
				headerMap.clear();

				g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tprotocol error, STX expected [%s]", __FUNCTION__, keyValueStr.c_str());
				return LarsErr::E_INVALID_PARAMETER;
			}
			index++;
			moduleState = PARSE_KEY;
			break;

		case PARSE_HEADER:
			// check for header attributes
			if ((keyValueStr[index] == Lars::ESC) || (keyValueStr[index] == Lars::GS))
			{
				if (!valueBegin)
					// no user value
					headerMap[HEADER_LENGTH.substr(0, 3) + to_string(headerIdx++)] = keyValueStr.substr(0, 0);
				else
					headerMap[HEADER_LENGTH.substr(0, 3) + to_string(headerIdx++)] = keyValueStr.substr(valueBegin, index - valueBegin);

				keyBegin = keyEnd = valueBegin = 0;

				if (keyValueStr[index] == Lars::GS)
					// go to state command
					moduleState = PARSE_COMMAND;
			}
			else if (!valueBegin) valueBegin = index;
			index++;
			break;

		case PARSE_COMMAND:
			// check for command
			if ((keyValueStr[index] == Lars::STX) || (keyValueStr[index] == Lars::ETB) || (keyValueStr[index] == Lars::GS))
			{
				if (!valueBegin)
				{
					// protocol error, return error
					refDataMap.clear();
					headerMap.clear();

					g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tprotocol error, cmd expected [%s]", __FUNCTION__, keyValueStr.c_str());
					return LarsErr::E_INVALID_PARAMETER;;
				}
				headerMap[HEADER_CMD] = keyValueStr.substr(valueBegin, index - valueBegin);

				if (keyValueStr[index] == Lars::STX)
				{
					keyBegin = keyEnd = valueBegin = 0;

					// go to state parse reference data
					moduleState = PARSE_KEY;
				}
				else if (keyValueStr[index] == Lars::GS)
				{
					keyBegin = keyEnd = valueBegin = 0;

					// go to state parse checksum
					moduleState = PARSE_CHECKSUM;
				}
				else
				{
					// reach end of protocol, finish
					endWithEtb = true;
				}
			}
			else if (!valueBegin) valueBegin = index;
			index++;
			break;


		case PARSE_KEY:
			// get key and check for fs, gs and etx
			if (keyValueStr[index] == Lars::FS)
			{
				keyEnd = index;
				if (keyBegin == keyEnd)
				{
					refDataMap.clear();
					headerMap.clear();

					g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tprotocol error, key expected [%s]", __FUNCTION__, keyValueStr.c_str());
					return LarsErr::E_INVALID_PARAMETER;
				}
				moduleState = PARSE_VALUE;
			}
			else if ((keyValueStr[index] == Lars::GS) || (keyValueStr[index] == Lars::ETX))
			{
				keyEnd = index;

				// key has no user data, so set empty string to the map
				refDataMap[keyValueStr.substr(keyBegin, keyEnd - keyBegin)] = keyValueStr.substr(0, 0);

				keyBegin = keyEnd = valueBegin = 0;

				if (keyValueStr[index] == Lars::GS)
				{
					moduleState = PARSE_KEY;
				}

				if (keyValueStr[index] == Lars::ETX)
				{
					if (type == RBS_FULL_TELEGRAM)
						moduleState = CHECK_FOR_ETB;
					if (type == RBS_REFERENCE_DATA)
						endWithEtx = true;
				}
			}
			else if (!keyBegin) keyBegin = index;
			index++;
			break;

		case PARSE_VALUE:
			// get value and check for gs or etx
			if ((keyValueStr[index] == Lars::GS) || (keyValueStr[index] == Lars::ETX))
			{
				// check for value is empty
				if (valueBegin == 0)
				{
					valueBegin = index;
				}

				refDataMap[keyValueStr.substr(keyBegin, keyEnd - keyBegin)] = keyValueStr.substr(valueBegin, index - valueBegin);

				keyBegin = keyEnd = valueBegin = 0;

				if (keyValueStr[index] == Lars::GS)
				{
					moduleState = PARSE_KEY;
				}

				if (keyValueStr[index] == Lars::ETX)
				{
					if (type == RBS_FULL_TELEGRAM)
						moduleState = PARSE_CHECKSUM;
					if (type == RBS_REFERENCE_DATA)
						endWithEtx = 1;
				}
			}
			else if (!valueBegin) valueBegin = index;
			index++;
			break;

		case PARSE_CHECKSUM:
			// get value and check for etb
			if (keyValueStr[index] == Lars::ETB)
			{
				// check if checksum is available
				if (valueBegin)
				{
					headerMap[HEADER_CRC16] = keyValueStr.substr(valueBegin, index - valueBegin);
				}

				keyBegin = keyEnd = valueBegin = 0;

				moduleState = CHECK_FOR_ETB;
			}
			else
			{
				if (!valueBegin) valueBegin = index;
				index++;
			}
			break;

        case CHECK_FOR_ETB:
            //check for ETB
            if (keyValueStr[index] == Lars::ETB)
            {
                endWithEtb = true;
            }
            break;
        }

    }

    if ((type == RBS_FULL_TELEGRAM) && !endWithEtb)
    {
        headerMap.clear();
        refDataMap.clear();

        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tprotocol error, ETB expected [%s]", __FUNCTION__, keyValueStr.c_str());
        return LarsErr::E_INVALID_PARAMETER;
    }

    if ((type == RBS_REFERENCE_DATA) && !endWithEtx)
    {
        headerMap.clear();
        refDataMap.clear();

        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tprotocol error, ETX expected  [%s]", __FUNCTION__, keyValueStr.c_str());
        return LarsErr::E_INVALID_PARAMETER;
    }

    return LarsErr::E_SUCCESS;
}


void AdcRbs::ParseStructure(const string &structStr, keyValuePair &structMap)
{
    int headerIdx = 0;
    size_t newPos = 0;
    size_t oldPos = 0;

    while ((newPos = structStr.find(Lars::ESC, oldPos)) != string::npos)
    {
        structMap[STRUCT_POS + to_string(headerIdx++)] = structStr.substr(oldPos, newPos - oldPos);
        oldPos = ++newPos;
    }

    // store last element
    structMap[STRUCT_POS + to_string(headerIdx++)] = structStr.substr(oldPos, structStr.size() - oldPos);

    return;
}


bool AdcRbs::CheckErrorCode(short errorCode, bool ignoreADCError)
{
	if (ignoreADCError == false)
	{
		if ((errorCode == LarsErr::E_SUCCESS) || (errorCode == LarsErr::E_AUTHENTICATION))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if ((errorCode == LarsErr::E_SUCCESS) || (errorCode >= LarsErr::E_COMMAND_NOT_EXECUTED))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}


string AdcRbs::ConvertFloatIEEToInt(string value)
{
	float number;
	unsigned int *pInt;
	stringstream floatIEEEToInt;

	try
	{
		Helpers::ReplaceDecimalPoint(value);
		number = stof(value);

		pInt = (unsigned int *)&number;

		floatIEEEToInt << *pInt;

		return floatIEEEToInt.str();
	}
	catch (std::exception &e)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\texception: %s", __FUNCTION__, e.what());
		return value;
	}
}


bool AdcRbs::ConvertDegreeToDigits(keyValuePair *wdtaSettings)
{
	bool bRet;
	AdcTiltComp tiltCompensation;
	keyValuePair::iterator it;

	if (GetTiltCompensation(&tiltCompensation) == LarsErr::E_SUCCESS)
	{
		for (int idx = 0; idx < WDTA_STRINGS_TO_CONVERT_COUNT; idx++)
		{
			// search for y1
			it = wdtaSettings->find(WDTA_STRINGS_TO_CONVERT[idx]);
			if (it != wdtaSettings->end())
			{
				string tmpKeyStr = (*it).first;
				size_t idx = tmpKeyStr.find("FLOAT", 0);
				if (idx != string::npos) tmpKeyStr = tmpKeyStr.replace(idx, strlen("FLOAT"), "INT");

				// convert degree in digits
				string value = (*it).second;
				Helpers::ReplaceDecimalPoint(value);
				float degree = stof(value);
				int digits = (int)(sin((degree * PI) / 180) * tiltCompensation.resolution + 0.5);

				wdtaSettings->erase((*it).first);
				wdtaSettings->insert(std::pair<string, string>(tmpKeyStr, to_string(digits)));
			}
		}
		bRet = true;
	}
	else
	{
		bRet = false;
	}

	return bRet;
}


short AdcRbs::GetReferenceData(const string &masterKey, const keyValuePair &refDataMap, RefDataStruct &refData, short sollDefDataReceived)
{
    short           errorCode = LarsErr::E_SUCCESS;
    short           defDataReceived = 0;
    string          value;
    stringstream    input;
    keyValuePair    structMap;
    string          hexAscii;
    HexDecoder      hexDecoder;

	if (Helpers::KeyExists(refDataMap, masterKey, value))
    {
        input >> setbase(m_base);

        structMap.clear();
        ParseStructure(value, structMap);

		switch (refData.id)
		{
		case REF_DATA_ID_SENSOR_HEALTH:
		{
			int index = 0;

			for (keyValuePair::iterator it = structMap.begin(); it != structMap.end(); it++)
			{
				// check for empty content -> empty means optional
				if (!(*it).second.empty())
				{
					input.clear();
					input << (*it).second;

					if ((*it).first == "pos0")
					{
						// store diaParamString
						if (masterKey.size() <= (sizeof(refData.u.sensorHealth->diaParam) - 1))
							strcpy(refData.u.sensorHealth->diaParam, masterKey.c_str());

						input >> refData.u.sensorHealth->type;
						defDataReceived++;
					}
					else
					{
						if (refData.u.sensorHealth->type == 0)
						{
							if (index < DIA_MAX_VALUE_UNIT)
							{
								if (!(index & 0x01))
								{
									input >> refData.u.sensorHealth->Health.type0.ValueUnit[index >> 1].value;
								}
								else
								{
									input >> refData.u.sensorHealth->Health.type0.ValueUnit[index >> 1].unit;
									refData.u.sensorHealth->Health.type0.number = (index >> 1) + 1;
								}
								index++;
								defDataReceived++;
							}
							else
							{
								errorCode = LarsErr::E_PROTOCOL;
							}
						}
					}
					if (input.fail())
					{
						errorCode = LarsErr::E_PROTOCOL;
						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror set key %s with value %s", __FUNCTION__, masterKey.c_str(), value.c_str());
					}
				}
			}

			if (refData.u.sensorHealth->type == 0)
			{
				// patch input data count depending on type 
				sollDefDataReceived = defDataReceived;

				// check if all value/unit pairs are ok
				if (index & 0x01)
					errorCode = LarsErr::E_PROTOCOL;
			}
			break;
		}

		case REF_DATA_ID_COUNTRY_SETTINGS:
		{
			int idx; 

			for (keyValuePair::iterator it = structMap.begin(); it != structMap.end(); it++)
			{
				// get index
				idx = atoi((*it).first.substr(3).c_str());

				// add entry to map;
				if (idx < COUNTRY_SPECIFIC_STRINGS_COUNT)
				{
					(*refData.u.csp)[COUNTRY_SPECIFIC_STRINGS[idx]] = (*it).second;
					defDataReceived++;
				}
			}
			break;
		}

		case REF_DATA_ID_LOAD_CAPACITY_SETTINGS:
		{
			int idx;

			for (keyValuePair::iterator it = structMap.begin(); it != structMap.end(); it++)
			{
				// get index
				idx = atoi((*it).first.substr(3).c_str());

				// add entry to map;
				if (idx < LOAD_CAPACITY_STRINGS_COUNT)
				{
					(*refData.u.lcp)[LOAD_CAPACITY_STRINGS[idx]] = (*it).second;
					defDataReceived++;
				}
			}
			break;
		}

		default:
		{
			for (keyValuePair::iterator it = structMap.begin(); it != structMap.end(); it++)
			{
				// check for empty content -> empty means optional
				if (!(*it).second.empty())
				{
					input.clear();
					input << (*it).second;

					if ((*it).first == "pos0")
					{
						unsigned short tmpValue;

						switch (refData.id)
						{
							// get stat
						case REF_DATA_ID_STAT: input >> refData.u.stat; defDataReceived++; break;
							// get adcState
						case REF_DATA_ID_LCSTATE: input >> refData.u.adcState->state; defDataReceived++; break;
							// get base price
						case REF_DATA_ID_BASEPRICE: input >> refData.u.bp->price.value; defDataReceived++; break;
							// get sell price
						case REF_DATA_ID_SELLPRICE: input >> refData.u.sp->value; defDataReceived++; break;
							// get scale mode
						case REF_DATA_ID_SCALEMODE: input >> tmpValue; refData.u.sm = (AdcScaleMode)tmpValue; defDataReceived++; break;
							// get displaytext
						case REF_DATA_ID_DISPLAYTEXT: input >> refData.u.dt->x; break;
							// get eeprom data
						case REF_DATA_ID_EEPROM: input >> tmpValue; refData.u.ee->region = (AdcEepromRegion)tmpValue; defDataReceived++; break;
							// get eeprom welmec size
						case REF_DATA_ID_EEPROM_WELMEC_SIZE: input >> refData.u.size; defDataReceived++; break;
							// get eeprom open size
						case REF_DATA_ID_EEPROM_OPEN_SIZE: input >> refData.u.size; defDataReceived++; break;
							// get eeprom prod size
						case REF_DATA_ID_EEPROM_PROD_SIZE: input >> refData.u.size; defDataReceived++; break;
							// get eeprom prod sensors size
						case REF_DATA_ID_EEPROM_PROD_SENSORS_SIZE: input >> refData.u.size; defDataReceived++; break;
							// get tilt compensation data
						case REF_DATA_ID_TILT_COMP: input >> refData.u.tc->state.all; defDataReceived++; break;
							// get raw value 
						case REF_DATA_ID_RAW_VALUE: input >> refData.u.raw; defDataReceived++; break;
							// get max display characters
						case REF_DATA_ID_MAX_DISPL_CHAR: input >> refData.u.ndt; defDataReceived++; break;
							// get weight
						case REF_DATA_ID_WEIGHT: input >> refData.u.wt->value; defDataReceived++; break;
							// get tare
						case REF_DATA_ID_TARE: input >> tmpValue; refData.u.tare->type = (AdcTareType)tmpValue; defDataReceived++; break;
							// get random for authentication
						case REF_DATA_ID_RANDOM_AUTH:
							input >> hexAscii;
							hexDecoder.Put((byte *)hexAscii.data(), hexAscii.size());
							*refData.u.random.size = hexDecoder.Get(refData.u.random.value, *refData.u.random.size);
							defDataReceived++;
							break;
							// get calib step
						case REF_DATA_ID_CALIB_STEP: input >> refData.u.step; defDataReceived++; break;
							// get filter index
						case REF_DATA_ID_FILTER_IDX: input >> refData.u.filter->idx; defDataReceived++; break;
							// get offset stable time
						case REF_DATA_ID_OFFSET_STABLE_TIME: input >> refData.u.filter->offsetStableTime; defDataReceived++; break;
							// get stable range
						case REF_DATA_ID_STABLE_RANGE: input >> refData.u.filter->stableRange; defDataReceived++; break;
							// get g factor
						case REF_DATA_ID_GFACTOR: input >> refData.u.gFactor; defDataReceived++; break;
							// get production site
						case REF_DATA_ID_PROD_SITE: input >> refData.u.prodSiteID; defDataReceived++; break;
							// get g factor offset
						case REF_DATA_ID_PROD_GFACTOR_OFFSET: input >> refData.u.gFactor; defDataReceived++; break;
							// get date
						case REF_DATA_ID_PROD_DATE: 
							if ((*it).second.size() <= (sizeof(refData.u.dateStr) - 1))
							{
								strcpy(refData.u.dateStr, (*it).second.c_str());
								defDataReceived++;
							}
							break;
							// get wsc string
						case REF_DATA_ID_PROD_WSC:
							if ((*it).second.size() <= (sizeof(refData.u.wsc) - 1))
							{
								strcpy(refData.u.wsc, (*it).second.c_str());
								defDataReceived++;
							}
							break;
							// get interface autodetection
						case REF_DATA_ID_INTERFACE_AUTO_DETECTION: input >> refData.u.interfaceAutoDetection; defDataReceived++; break;
							// get interface mode
						case REF_DATA_ID_INTERFACE_MODE: input >> refData.u.interfaceMode; defDataReceived++; break;
							// get zero point tracking mode
						case REF_DATA_ID_ZERO_POINT_TRACKING: input >> refData.u.mode; defDataReceived++; break;
							// get verification parameter protected
						case REF_DATA_ID_VERIF_PARAM_PROTECTED: input >> refData.u.verifParamProtected; defDataReceived++; break;
							// get update allowed
						case REF_DATA_ID_UPDATE_ALLOWED: input >> refData.u.updateAllowed; defDataReceived++; break;
							// get remaining warm up time
						case REF_DATA_ID_REMAINING_WARM_UP_TIME: input >> refData.u.remainingWarmUpTime; defDataReceived++; break;
							// get warm up time
						case REF_DATA_ID_WARM_UP_TIME: input >> refData.u.warmUpTime; defDataReceived++; break;
							// get adc operating mode
						case REF_DATA_ID_OPERATING_MODE: input >> tmpValue; refData.u.opMode = (AdcOperatingMode)tmpValue; defDataReceived++; break;
							// get zero setting interval
						case REF_DATA_ID_ZERO_SETTING_INTERVAL: input >> refData.u.zeroSettingInterval; defDataReceived++; break;
							// get automatic zero setting time
						case REF_DATA_ID_AUTOMATIC_ZERO_SETTING_TIME: input >> refData.u.automaticZeroSettingTime; defDataReceived++; break;
							// get digit value 
						case REF_DATA_ID_DIGIT_VALUE: input >> refData.u.digitValue; defDataReceived++; break;
							// get digit value in calibration mode
						case REF_DATA_ID_CALIB_DIGIT_VALUE: input >> refData.u.calibDigit; defDataReceived++; break;
							// get scale model
						case REF_DATA_ID_SCALE_MODEL: input >> *refData.u.scaleModel; defDataReceived++; break;
							// get state automatic tilt sensor
						case REF_DATA_ID_STATE_AUTOMATIC_TILT_SENSOR: input >> refData.u.state; defDataReceived++; break;

						}
					}
					else if ((*it).first == "pos1")
					{
						switch (refData.id)
						{
							// get base price
						case REF_DATA_ID_BASEPRICE: 
							if ((*it).second.size() <= (sizeof(refData.u.bp->price.currency) - 1))
								strcpy(refData.u.bp->price.currency, (*it).second.c_str());
							break;
							// get sell price
						case REF_DATA_ID_SELLPRICE: 
							if ((*it).second.size() <= (sizeof(refData.u.sp->currency) - 1))
								strcpy(refData.u.sp->currency, (*it).second.c_str());
							break;
							// get displaytext
						case REF_DATA_ID_DISPLAYTEXT: input >> refData.u.dt->y; break;
							// get eeprom data
						case REF_DATA_ID_EEPROM: input >> refData.u.ee->startAdr; defDataReceived++; break;
							// get tilt compensation data
						case REF_DATA_ID_TILT_COMP: input >> refData.u.tc->x; defDataReceived++; break;
							// get weight
						case REF_DATA_ID_WEIGHT: input >> refData.u.wt->decimalPlaces; defDataReceived++; break;
							// get tare
						case REF_DATA_ID_TARE: input >> refData.u.tare->value.value; defDataReceived++; break;
						}
					}
					else if ((*it).first == "pos2")
					{
						unsigned short tmpValue;

						switch (refData.id)
						{
							// get base price
						case REF_DATA_ID_BASEPRICE: input >> refData.u.bp->price.decimalPlaces; defDataReceived++; break;
							// get sell price
						case REF_DATA_ID_SELLPRICE: input >> refData.u.sp->decimalPlaces; defDataReceived++; break;
							// get displaytext
						case REF_DATA_ID_DISPLAYTEXT: 
							if ((*it).second.size() <= (sizeof(refData.u.dt->text) - 1))
								strcpy(refData.u.dt->text, (*it).second.c_str());
							break;
							// get eeprom data
						case REF_DATA_ID_EEPROM: 
							input >> refData.u.ee->len; 
							// if data length == 0 reduce data count
							if (!refData.u.ee->len) sollDefDataReceived--;
							defDataReceived++; 
							break;
							// get tilt compensation data
						case REF_DATA_ID_TILT_COMP: input >> refData.u.tc->y; defDataReceived++; break;
							// get weight
						case REF_DATA_ID_WEIGHT: input >> tmpValue; refData.u.wt->weightUnit = (AdcWeightUnit)tmpValue; defDataReceived++; break;
							// get tare
						case REF_DATA_ID_TARE: input >> refData.u.tare->value.decimalPlaces; defDataReceived++; break;
						}
					}
					else if ((*it).first == "pos3")
					{
						unsigned short tmpValue;

						switch (refData.id)
						{
							// get weight unit
						case REF_DATA_ID_BASEPRICE: input >> tmpValue; refData.u.bp->weightUnit = (AdcWeightUnit)tmpValue; defDataReceived++; break;
							// get eeprom data
						case REF_DATA_ID_EEPROM:
							input >> hexAscii;
							hexDecoder.Put((byte *)hexAscii.data(), hexAscii.size());
							refData.u.ee->len = hexDecoder.Get(refData.u.ee->data, sizeof(refData.u.ee->data));
							defDataReceived++;
							break;
							// get tilt compensation data (set limit_angle_2 = limit_angle_1 for backward compatibility)
						case REF_DATA_ID_TILT_COMP: input >> refData.u.tc->limit_angle_1;  refData.u.tc->limit_angle_2 = refData.u.tc->limit_angle_1;  defDataReceived++; break;
							// get tare
						case REF_DATA_ID_TARE: input >> tmpValue; refData.u.tare->value.weightUnit = (AdcWeightUnit)tmpValue; defDataReceived++; break;
						}
					}
					else if ((*it).first == "pos4")
					{
						switch (refData.id)
						{
							// get tilt compensation data
						case REF_DATA_ID_TILT_COMP: input >> refData.u.tc->resolution; defDataReceived++; break;
						}
					}
					else if ((*it).first == "pos5")
					{
						switch (refData.id)
						{
							// get tilt compensation data
						case REF_DATA_ID_TILT_COMP: input >> refData.u.tc->limit_angle_2; defDataReceived++; break;
						}
					}
					if (input.fail())
					{
						errorCode = LarsErr::E_PROTOCOL;
						g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\terror set key %s with value %s", __FUNCTION__, masterKey.c_str(), value.c_str());
					}
				}
			}
			break;
		}
		}

        if (defDataReceived < sollDefDataReceived) errorCode = LarsErr::E_PROTOCOL;
    }
    else
    {
        errorCode = LarsErr::E_KEY_NOT_FOUND;
		g_adcTrace.Trace(AdcTrace::TRC_INFO, "%s\terror key %s not found", __FUNCTION__, masterKey.c_str());
    }

    return errorCode;
}


short AdcRbs::GetDebugResponse(const string &cmd, short orderID, string &response)
{
    stringstream refData;
    stringstream length;

    refData << setbase(m_base);
    length << setbase(m_base);

    refData << Lars::ESC << orderID << Lars::GS << cmd << Lars::STX << REF_DATA_ID_STAT_STR << Lars::FS << "0";

    if ((cmd == "ST") || (cmd == "STP") || (cmd == "CT") || (cmd == "SZ") || (cmd == "RW") ||
        (cmd == "GHR"))
    {
        refData << Lars::GS << REF_DATA_ID_LCSTATE_STR << Lars::FS << "0";
    }

    if (cmd == "RW")
    {
        refData << Lars::GS << REF_DATA_ID_WEIGHT_STR << Lars::FS << "1000" << Lars::ESC << "3" << Lars::ESC << 0;
        refData << Lars::GS << REF_DATA_ID_TARE_STR << Lars::FS << "1" << Lars::ESC << "50" << Lars::ESC << "3" << Lars::ESC << 0;
        refData << Lars::GS << REF_DATA_ID_BASEPRICE_STR << Lars::FS << "123" << Lars::ESC << "EUR" << Lars::ESC << "3" << Lars::ESC << 0;
        refData << Lars::GS << REF_DATA_ID_SELLPRICE_STR << Lars::FS << "123" << Lars::ESC << "EUR" << Lars::ESC << "3";
    }

    if (cmd == "LOG")
    {
        refData << "vid" << Lars::FS << "0081" << Lars::GS << \
                   "cid" << Lars::FS << "0001" << Lars::GS << \
                   "swvl" << Lars::FS << "1.01.0001" << Lars::GS << \
                   "swid" << Lars::FS << "1234" << Lars::GS << \
                   "swv" << Lars::FS << "2.00" << Lars::GS << \
                   "date" << Lars::FS << "15.09.10 12:55";
    }

    if (cmd == "GA")
    {
        refData << Lars::GS << "no" << Lars::FS << "8269ABCA4B845A96";
    }

    if (cmd == "GHR")
    {
        refData << Lars::GS << REF_DATA_ID_WEIGHT_STR << Lars::FS << "1000" << Lars::ESC << "3" << Lars::ESC << 0;
        refData << Lars::GS << "wth" << Lars::FS << "100000" << Lars::ESC << "5" << Lars::ESC << 0;
        refData << Lars::GS << "raw" << Lars::FS << "123456";

    }

    if (cmd == "DIA")
    {
        refData << Lars::GS << "OVERLOAD" << Lars::FS << "10";
        refData << Lars::GS << "UNDERLOAD" << Lars::FS << "5";
        refData << Lars::GS << "UPDATE" << Lars::FS << "15";
    }

    if (cmd == "CAP")
    {
        refData << Lars::GS << "c1" << Lars::FS << "0";
        refData << Lars::GS << "c2" << Lars::FS << "1";
        refData << Lars::GS << "c3" << Lars::FS << "0";
        refData << Lars::GS << "c4" << Lars::FS << "1";
        refData << Lars::GS << "c5" << Lars::FS << "0";
    }

    if (cmd == "VERS")
    {
        refData << Lars::GS << "Bootloader" << Lars::FS << "1.00.0001";
        refData << Lars::GS << "Firmware" << Lars::FS << "1.00";
    }

    if (cmd == "GSVp")
    {
        refData << Lars::GS << "bp" << Lars::FS << "123" << Lars::ESC << "EUR" << Lars::ESC << "2" << Lars::ESC << 0;
    }

    if (cmd == "GSVt")
    {
        //refData << Lars::GS << "dt" << Lars::FS << "1" << Lars::ESC << "1" << Lars::ESC << "DisplayText";     // with x/y-coordinates
        refData << Lars::GS << "dt" << Lars::FS << Lars::ESC << Lars::ESC << "DisplayText";                     // without x/y-coordinates
    }

    if (cmd == "GSVn")
    {
        refData << Lars::GS << "ndt" << Lars::FS << "10";
    }

    if (cmd == "GSVm")
    {
        refData << Lars::GS << "m" << Lars::FS << "2";
    }

    if (cmd == "GSVe")
    {
        refData << Lars::GS << "ee" << Lars::FS << "200" << Lars::ESC << "10" << Lars::ESC << "0001020304050607080A0B0C0D0E0F10";
    }

    if (cmd == "GSVc")
    {
        refData << Lars::GS << "tc" << Lars::FS << "0" << Lars::ESC << "10" << Lars::ESC << "10" << Lars::ESC << "FF" << Lars::ESC << "100";
    }

    refData << Lars::ETX << Lars::ETB;
    length << refData.str().size();
    response = Lars::SOH + length.str() + refData.str();

    return LarsErr::E_SUCCESS;
}
