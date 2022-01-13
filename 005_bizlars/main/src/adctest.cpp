// adctest.cpp : Defines the entry point for the console application.
//

#ifdef  _MSC_VER
#include "stdafx.h"
#include "vld.h"
#include <Windows.h>
#include <conio.h>
#include <crtdbg.h>
#endif
#ifdef __GNUC__
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <cmath>
#endif
#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <time.h>
#include "bizlars.h"
#include "authentication.h"
using namespace std;

//----------------------------------------------------------------------------
// VersionInfo Tag
//
#define BIN_NAME          "adctest"
#define BIN_DESCR         "Bizerba ADC Test Program"
#define BIN_VERSION       "1.44.0001"
#define VERSION_FOR_AUTHENTICATION	"01.44"

//
// Macro to fill VERSION_INFO struct.
// Uses predefined BIN_NAME, BIN_DESCR and BIN_VERSION constants
//
#define  STANDARD_VERSION_INFO   {"",\
                                  BIN_NAME,\
                                  BIN_DESCR,\
                                  BIN_VERSION,\
                                  __DATE__,\
                                  __TIME__,\
                                  "",\
                                  ""}

#define  SH_NAME        "Bizerba Weighing Sytems"
#define  ARR_STR        "All Rights Reserved"
#define  _RELEASE_STR   "Release"
#define  _VERSION_STR   "Version"
#define  COPYRIGHT      "Copyright (c) 1999-2010"
#define  BANNER_R1(pVi) printf("%s - %s\n",(pVi)->szModule,\
                        (pVi)->szDescription)
#define  BANNER_R2(pVi) printf("%s %s, %s %s, %s %s\n",(pVi)->szTos,\
                        _VERSION_STR,_RELEASE_STR,\
                        (pVi)->szVersion,\
                        (pVi)->szDate,(pVi)->szTime)
#define  BANNER_R3(pVi) printf("%s, %s - %s\n\n",COPYRIGHT,SH_NAME,ARR_STR)

#define ISBACKSPACEKEY( x )   ( (x) == 0x08 )
#define ISENTERKEY( x )       ( ((x) == 0x0a) || ((x) == 0x0d) )
#define ISSIGN( x )           ( (x) == 0x2d )
#define MAXINPUTSTRING        255
#define CURSORLEFT            0x08

#define VAR_LENGTH          1024

static const char STX = 0x02;
static const char ETX = 0x03;
static const char FS  = 0x1C;
static const char GS  = 0x1D;


// defines for state machine
static const short  CHECK_FOR_STX = 0;
static const short  PARSE_KEY = 1;
static const short  PARSE_VALUE = 2;

// defines for authentication
#define ADW_AUTH_CRC_START      0x4461      /* start value for the Polynomial */
#define ADW_AUTH_CRC_POLY       0x1021      /* Polynomial-coeffizient */
#define ADW_AUTH_CRC_MASK       0x0000      /* Mask for the Polynomial */


#define ADC_PARAMETER_MODE		100	

// define mode for print adc state
#define MODE_READ_WEIGHT		1

/**
******************************************************************************
* function pointer
******************************************************************************
*/
typedef void(*MenuCall)(void);

/**
******************************************************************************
* structures
******************************************************************************
*/

typedef struct {
	unsigned char cChoice;     // menu choice
	char *MenuDescr;           // menu description
	MenuCall pMenuFunction;    // function to call
} MENUTABLE, *PMENUTABLE;


typedef struct {
	char *diaParam;
	short type;
	short number;
	char *valueStr[10];
} CONFIG_DIA_PARAM;

/**
******************************************************************************
* functions
******************************************************************************
*/
void DoOpenAdc();
void DoCloseAdc();
void DoTraceAdc();
void DoQuitAdc();
void DoGetInfo();
void DoSetScaleValues();
void DoGetScaleValues();
void DoZeroScale();
void DoHandleTare();
void DoResetAdc();
void DoReadWeight();
void DoGetLogger();
void DoMakeAuthentication();
void DoGetHighResolution();
void DoGetGrossWeight();
void DoDiagnosticData();
void DoSetCountryFilesPath();
void DoCalibration();
void DoGetInternalData();
void DoMakeTests();
void DoUpdate();
char* GetString(char *pszPrompt, short sMaxLen);
long GetNumeric(char *pszPrompt);
char GetCh(void);
short PrintErrorCode(short errorCode, short printOK);
char *GetTareType(AdcTareType tareType);
char *GetWeightUnit(AdcWeightUnit weightUnit);
char *GetScaleMode(AdcScaleMode scaleMode);
char *GetOperatingMode(AdcOperatingMode opMode);
short Update(const short force = 0);
short SetOperatingMode(AdcOperatingMode mode);
short GetOperatingMode(AdcOperatingMode *mode);
void PrintAdcState(AdcState adcState, int mode = 0, bool autoAuthentication = false, string *adcStateOut = NULL);
void ShowSupportedCountries();
void ShowCompatibleLoadCapacities(char *country);
void FormatValue(long long value, short decimalPlaces, string* formatedString);
unsigned long GetSystemTickCount();
char *GetDimensionUnit(long dimensionUnit);
void EepromReadWrite(AdcScaleValues *scaleValue, bool read);
bool EepromGetSize(AdcEepromSize *eepromSize);
short MakeAuthentication(bool withIdentityString, stringstream *IDstring = nullptr, bool silent = false);
void GetDiagnosticData(void);
void ConfigureDiagnosticData(void);
void ParseInputValueUnit(char *pStrPointer, long *value, long *unit);
void MakeAuthenticationTest();
void MakeEepromTest();
void MakeRegistrationTest();
void MakeUpdateTest();
void MakeSetGetValueTest();
void MakeRoundingTest();



#ifdef __GNUC__
static void _kb_initialize( void );
static void _kb_restore( void );
static int _kbhit( void );
static int _getch( void );
static void Sleep( long dwMilliseconds );

//
// Used to emulate _kbhit() and _getch()
//
static struct termios tOrigKeybConf, tNewKeybConf;
static int    iPeek = -1;
static short  sKbInit = false;
#endif


/**
******************************************************************************
* structures
******************************************************************************
*/
MENUTABLE tMenu[] = {
	{ 'o', (char *)"OpenAdc            ", DoOpenAdc },
	{ 'c', (char *)"CloseAdc           ", DoCloseAdc },
    { 'a', (char *)"ReadWeight         ", DoReadWeight },
    { 'b', (char *)"GetHighResolution  ", DoGetHighResolution },
    { 'd', (char *)"DiagnosticData     ", DoDiagnosticData },
    { 'e', (char *)"GetLogger          ", DoGetLogger },
	{ 'f', (char *)"Calibration        ", DoCalibration },
	{ 'g', (char *)"GetScaleValues     ", DoGetScaleValues },
	{ 'h', (char *)"GetInternalData    ", DoGetInternalData },
    { 'i', (char *)"GetInfo            ", DoGetInfo },
	{ 'j', (char *)"Update             ", DoUpdate },
	{ 'k', (char *)"GetGrossWeight     ", DoGetGrossWeight },
    { 'l', (char *)"SetCountryFilesPath", DoSetCountryFilesPath },
    { 'm', (char *)"MakeAuthentication ", DoMakeAuthentication },
	{ 'p', (char *)"Trace              ", DoTraceAdc },
    { 'r', (char *)"Reset              ", DoResetAdc },
    { 's', (char *)"SetScaleValues     ", DoSetScaleValues },
    { 't', (char *)"Tare               ", DoHandleTare },
	{ 'u', (char *)"Tests              ", DoMakeTests },
    { 'z', (char *)"ZeroScale          ", DoZeroScale },
	{ 'x', (char *)"Quit			   ", DoQuitAdc },
	{ 0, 0, 0 }                         // end of menu entries
};

CONFIG_DIA_PARAM configDiaParam[] = {
	{ (char *)"diau", 100, 1, { (char *)"Threshold over load % or promille", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"" } },
	{ (char *)"diat", 100, 1, { (char *)"Threshold temperatur", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"" } },
	{ (char *)"diaa", 100, 1, { (char *)"Threshold N/kg or N/g", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"" } },
	{ (char *)"diai", 100, 2, { (char *)"Threshold %", (char *)"Debounce time msec or sec", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"", (char *)"" } },
	{ 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }},
};


//
// VERSION_INFO structure definition
// sizeof(VERSION_INFO) is 128 bytes.
//
typedef  struct {
	char  szLabel[8];
	char  szModule[16];
	char  szDescription[46];
	char  szVersion[10];
	char  szDate[12];
	char  szTime[9];
	char  szTos[15];
	char  szReserved[12];
} VERSION_INFO;


/**
******************************************************************************
* globale variables
******************************************************************************
*/
static VERSION_INFO g_tVi = STANDARD_VERSION_INFO;
bool g_exit = false;
short g_adcHandle = 0;

map<AdcCapabilities, string> g_adcCap = { { AdcCapabilities::ADC_CAP_DEVICE_AUTHENTICATION, "Authentication" },
{ AdcCapabilities::ADC_CAP_DISPLAY_WEIGHT, "Weight Display" },
{ AdcCapabilities::ADC_CAP_DISPLAY_TARE, "Tare Display" },
{ AdcCapabilities::ADC_CAP_DISPLAY_BASEPRICE, "Baseprice Display" },
{ AdcCapabilities::ADC_CAP_DISPLAY_PRICE, "Price Display" },
{ AdcCapabilities::ADC_CAP_DISPLAY_TEXT, "Text Display" },
{ AdcCapabilities::ADC_CAP_PRICE_CALCULATION, "Price Calculation" },
{ AdcCapabilities::ADC_CAP_TARE, "Tare function" },
{ AdcCapabilities::ADC_CAP_ZERO_SCALE, "Zero Scale" },
{ AdcCapabilities::ADC_CAP_TILT_COMPENSATION, "Tilt Compensation" },
{ AdcCapabilities::ADC_CAP_TARE_PRIORITY, "Tare Priority" },
{ AdcCapabilities::ADC_CAP_TWO_CONVERTER, "Two Converter" },
{ AdcCapabilities::ADC_CAP_SEPARATE_LOADCELL, "Adc and load cell are separately" },
{ AdcCapabilities::ADC_CAP_FIRMWARE_UPDATE, "Firmware update" },
{ AdcCapabilities::ADC_CAP_HIGHRESOLUTION_WITH_TARE, "High resolution with tare" },
{ AdcCapabilities::ADC_CAP_DIGITS_RESOLUTION_CONFIGURABLE, "Digits resolution configurable" },
{ AdcCapabilities::ADC_CAP_TILT_COMPENSATION_CORNER_CORRECTION, "Tilt compensation corner correction" },
{ AdcCapabilities::ADC_CAP_WEIGHT_DEPENDENT_TILT_ANGLE, "Weight dependent tilt angle" },
{ AdcCapabilities::ADC_CAP_AUTOMATIC_TILT_SENSOR, "Automatic tilt sensor" }
};


char *diaUnits[] = {
(char *)"",
(char *)"digits",
(char *)"\xF8\x43",    // grad celcius
(char *)"msec",
(char *)"sec",
(char *)"min",
(char *)"hours",
(char *)"N/kg",
(char *)"N/g",
(char *)"%",
(char *)"promille",			
(char *)"mg",
(char *)"g",
(char *)"kg",
0,
};
                             
/**
******************************************************************************
* DoQuitAdc - quit the application
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoQuitAdc(void)
{
    // close all connections
    DoCloseAdc();

	g_exit = true;
}


/**
******************************************************************************
* DoOpenAdc - open a connection to the Bizerba weighing system
*
* @param    
* @return   
* @remarks
******************************************************************************
*/
void DoOpenAdc(void)
{
    short			errorCode = 0;
    short			handle;
	unsigned char	performSWReset;
	char			portNr[255];
	unsigned long	size;

	short selection = (short)GetNumeric((char *)"Interface [0: usb   1: serial]: ");
	short swReset = (short)GetNumeric((char *)"Do SW-Reset [0: yes   1: no]: ");

	if (swReset == 1)
		performSWReset = 0;
	else
		performSWReset = 1;

	if (selection == 0)
	{
		errorCode = AdcOpen(NULL, NULL, NULL, &handle, performSWReset);
		if (!PrintErrorCode(errorCode, 1))
		{
			if (handle)
			{
				size = sizeof(portNr);
				AdcGetPortNr(handle, portNr, &size);
				g_adcHandle = handle;
				printf("Successful open Adc, Handle: 0x%04hX	portNr: %s\n", g_adcHandle, portNr);
			}
		}
	}
	else if (selection == 1)
	{
		char portStr[50];

		short portNr = (short)GetNumeric((char *)"COM Port: ");
		sprintf(portStr, "COM%d", portNr);

		errorCode = AdcOpen("AdcSerial", NULL, portStr, &handle, performSWReset);
		if (!PrintErrorCode(errorCode, 1))
		{
			if (handle)
			{
				g_adcHandle = handle;
				printf("Successful open Adc, Handle: 0x%04hX\n", g_adcHandle);
			}
		}
	}

	if (g_adcHandle)
	{
		// check operating mode
		AdcOperatingMode opMode;
		errorCode = GetOperatingMode(&opMode);
		if (!PrintErrorCode(errorCode, 0))
		{
			if (opMode == ADC_BOOTLOADER)
			{
				// adc is in bootloader mode, update firmware
				DoUpdate();
			}
		}
	}
}


/**
******************************************************************************
* DoCloseAdc - close a connection to the Bizerba weighing system
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoCloseAdc(void)
{
    short errorCode;

	if (g_adcHandle)
	{
		errorCode = AdcClose(g_adcHandle);
		PrintErrorCode(errorCode, 1);
	}
	g_adcHandle = 0;
}


/**
******************************************************************************
* DoCloseAdc - close a connection to the Bizerba weighing system
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoTraceAdc(void)
{
	char *pStrPointer = NULL;
	short logLevel = (short)GetNumeric((char *)"set trace level (0=off; 1=Err/Wrn; 2=Ablauf; 3=Info) ");

	if (logLevel)
	{
		pStrPointer = GetString((char *)"Set trace filename + path", 255);
	}

	AdcSetTrace(pStrPointer, logLevel);
}

/**
******************************************************************************
* GetKeyValuePair - separate key/values from string
*
* @param
* @return
* @remarks
******************************************************************************
*/
short GetKeyValuePair(string &keyValueStr, std::map<string, string> &keyValueMap)
{
    short retCode = 1;
    short moduleState = CHECK_FOR_STX;
    short endWithEtx = 0;
    int index = 0;
    int keyBegin = 0;
    int keyEnd = 0;
    int valueBegin = 0;

    keyValueMap.clear();
    while ((keyValueStr[index] != 0) && (endWithEtx == 0))
    {
        switch (moduleState)
        {
        case CHECK_FOR_STX:
            // check for stx
            if (keyValueStr[index] != STX)
            {
                printf("Error: wrong string format !!\n");
                keyValueMap.clear();
                retCode = 0;
                return retCode;
            }
            index++;
            moduleState = PARSE_KEY;
            break;

        case PARSE_KEY:
            // get key and check for fs, gs and etx
            if (keyValueStr[index] == FS)
            {
                keyEnd = index;
                if (keyBegin == keyEnd)
                {
                    printf("Error: wrong string format !!\n");
                    keyValueMap.clear();
                    retCode = 0;
                    return retCode;
                }
                moduleState = PARSE_VALUE;
            }
            else if ((keyValueStr[index] == GS) || (keyValueStr[index] == ETX))
            {
                keyEnd = index;

                // key has no user data, so set empty string to the map
                keyValueMap[keyValueStr.substr(keyBegin, keyEnd - keyBegin)] = keyValueStr.substr(0, 0);

                keyBegin = keyEnd = valueBegin = 0;

                if (keyValueStr[index] == GS)
                {
                    moduleState = PARSE_KEY;
                }

                if (keyValueStr[index] == ETX)
                {
                    endWithEtx = 1;
                }
            }
            else if (!keyBegin) keyBegin = index;
            index++;
            break;

        case PARSE_VALUE:
            // get value and check for gs or etx
            if ((keyValueStr[index] == GS) || (keyValueStr[index] == ETX))
            {
                if (valueBegin == index)
                {
                    printf("Error: wrong string format !!\n");
                    keyValueMap.clear();
                    retCode = 0;
                    return retCode;
                }

				// check for value is empty
				if (valueBegin == 0)
				{
					valueBegin = index;
				}

                keyValueMap[keyValueStr.substr(keyBegin, keyEnd - keyBegin)] = keyValueStr.substr(valueBegin, index - valueBegin);

                keyBegin = keyEnd = valueBegin = 0;

                if (keyValueStr[index] == GS)
                {
                    moduleState = PARSE_KEY;
                }

                if (keyValueStr[index] == ETX)
                {
                    endWithEtx = 1;
                }
            }
            else if (!valueBegin) valueBegin = index;
            index++;
            break;
        }
    }
    
    if (!endWithEtx)
    {
        printf("Error: wrong string format !!\n");
        keyValueMap.clear();
        retCode = 0;
    }

    return retCode;
}

/**
******************************************************************************
* DoGetInfo - get info (Version, ...) from adc
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoGetInfo(void)
{
    short errorCode;
    unsigned long size = 0;
    AdcScaleValues scaleValue;
    std::map <string, string> strStrMap;
	string valueString;
  
    errorCode = AdcGetVersion(g_adcHandle, NULL, &size);
    if (!PrintErrorCode(errorCode, 0))
    {
        char *versionStr = new char[size];
        errorCode = AdcGetVersion(g_adcHandle, versionStr, &size);
        if (!PrintErrorCode(errorCode, 0))
        {
            printf("Version of the components:\n\n");
            string verStr = versionStr;
            if (GetKeyValuePair(verStr, strStrMap))
            {
                for (map<string, string>::iterator it = strStrMap.begin(); it != strStrMap.end(); ++it)
                {
                    printf("\t%s\t\t%s\n", (*it).first.c_str(), (*it).second.c_str());
                }
            }
        }
        // free memory
        if (versionStr != NULL)
            delete[] versionStr;
    }

	// welmec version
	char   logbook[1024];
	std::map <string, string> logBookMap;
	size = sizeof(logbook);
	errorCode = AdcGetLogger(g_adcHandle, 0, logbook, &size);
	printf("\nWelmec Version ");
	if (!PrintErrorCode(errorCode, 0))
	{
		if (strlen(logbook))
		{
			// print out one log book row
			string logbookStr = logbook;
			if (GetKeyValuePair(logbookStr, logBookMap))
			{
				for (map<string, string>::iterator it = logBookMap.begin(); it != logBookMap.end(); ++it)
				{
					printf("[%s : %s] ", (*it).first.c_str(), (*it).second.c_str());
				}
				printf("\n");
			}
		}
	}

	// firmware file version
	char	firmwareFileVersion[512];
	size = sizeof(firmwareFileVersion);
	errorCode = AdcGetFirmwareFileVersion(g_adcHandle, firmwareFileVersion, &size);
	printf("\nFirmware file Version ");
	if (!PrintErrorCode(errorCode, 0))
	{
		printf("%s", firmwareFileVersion);
	}

    
    // print country
    char country[4];
	errorCode = AdcGetCountry(g_adcHandle, country, sizeof(country));
	printf("\nCountry: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		printf("%s", country);

		// print load capacities
		ShowCompatibleLoadCapacities(country);
	}
	

	// print load capacity
	char loadCapacity[128];
	errorCode = AdcGetLoadCapacity(g_adcHandle, loadCapacity, sizeof(loadCapacity));
	printf("\nLoad capacity: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		printf("%s", loadCapacity);
	}

	// print max capacity
	scaleValue.type = AdcValueType::ADC_MAX_CAPACITY;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("\nMaximum capacity: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		FormatValue(scaleValue.ScaleValue.maxCapacity.value, scaleValue.ScaleValue.maxCapacity.decimalPlaces, &valueString);
		printf("%s %s\n", valueString.c_str(), GetWeightUnit(scaleValue.ScaleValue.maxCapacity.weightUnit));
	}

	// print cal strings
	scaleValue.type = ADC_CAL_STRINGS;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("\nCalStrings ");
	if (!PrintErrorCode(errorCode, 0))
	{
		printf("\n\t%s\t%s\t%s\n", scaleValue.ScaleValue.calStrings.maxStr, scaleValue.ScaleValue.calStrings.minStr, scaleValue.ScaleValue.calStrings.eStr);
	}


    // print all capabilities
    unsigned char valueCap;
    printf("\nCapabilities:\n");
    for (map<AdcCapabilities, string>::iterator it = g_adcCap.begin(); it != g_adcCap.end(); ++it)
    {
        errorCode = AdcGetCapability(g_adcHandle, (*it).first, &valueCap);
        if (!PrintErrorCode(errorCode, 0))
        {
            if (valueCap)
                printf("\t%s\tsupported\n", (*it).second.c_str());
            else
                printf("\t%s\tnot supported\n", (*it).second.c_str());
        }
    }

    
    // print maxDisplayTextChars (currently not supported)
    /*scaleValue.type = ADC_MAX_DISPL_TEXT_CHAR;
    errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("\nMax Display Text Chars: ");
    if (!PrintErrorCode(errorCode, 0))
        printf(" %d\n", scaleValue.ScaleValue.maxDisplTextChar);*/


	// print scale lcs state
	AdcState    adcState;
	AdcWeight   weight;
	AdcTare     tare;
	AdcBasePrice  basePrice;
	AdcPrice    sellPrice;
	short       registrationRequest = 0;
	printf("\nlcs-state:");
	errorCode = AdcReadWeight(g_adcHandle, registrationRequest, &adcState, &weight, &tare, &basePrice, &sellPrice);
	if (!PrintErrorCode(errorCode, 0))
	{
		PrintAdcState(adcState);
	}


    // print scale mode
	scaleValue.type = AdcValueType::ADC_SCALE_MODE;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("\nScale mode: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		printf("%s\n", GetScaleMode(scaleValue.ScaleValue.scaleMode));
	}

	// print operating mode
	AdcOperatingMode opMode;
	errorCode = GetOperatingMode(&opMode);
	printf("operating mode: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		printf("%s\n", GetOperatingMode(opMode));
	}


	// print scale model
	char cScaleModel[25];
	cScaleModel[0] = 0;
	errorCode = AdcGetScaleModel(g_adcHandle, cScaleModel, sizeof(cScaleModel));
	printf("scale model: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		printf("%s\n", cScaleModel);
	}

  
	// print production settings
	scaleValue.type = ADC_PRODUCTION_SETTINGS;
	scaleValue.ScaleValue.prodSettings.size = sizeof(AdcProdSettings);
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("\nProduction settings:\n");
	if (!PrintErrorCode(errorCode, 0))
	{
		printf("\tproduction site: %d\n", scaleValue.ScaleValue.prodSettings.prodSiteID);
		printf("\tg factor offset: %.02f promille\n", (float)scaleValue.ScaleValue.prodSettings.gFactorOffset/100);
		printf("\tcalib date: %s\n", scaleValue.ScaleValue.prodSettings.date);
		if (strlen(scaleValue.ScaleValue.prodSettings.wsc))
		{
			printf("\twsc string: %s\n", scaleValue.ScaleValue.prodSettings.wsc);
		}
	}


	// print g factor
	scaleValue.type = ADC_G_FACTOR;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("\ng factor: ");
	if (!PrintErrorCode(errorCode, 0))
		printf("%.02f promille\n", (float)scaleValue.ScaleValue.gFactor/100);

	// print load capacity unit
	scaleValue.type = ADC_LOADCAPACITY_UNIT;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("load capacity unit: ");
	if (!PrintErrorCode(errorCode, 0))
		printf("%s\n", GetWeightUnit(scaleValue.ScaleValue.loadCapacityUnit));

	// print zero point tracking mode
	scaleValue.type = ADC_ZERO_POINT_TRACKING;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("zero point tracking: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		if (scaleValue.ScaleValue.mode == 0)
			printf("disable\n");
		else
			printf("enable\n");
	}

	// print zero setting interval
	scaleValue.type = ADC_ZERO_SETTING_INTERVAL;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("zero setting interval: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		printf("%ld sec\n", scaleValue.ScaleValue.zeroSettingInterval);
	}

	// print automatic zero setting time
	scaleValue.type = ADC_AUTOMATIC_ZERO_SETTING_TIME;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("automatic zero setting time: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		printf("%ld sec\n", scaleValue.ScaleValue.automaticZeroSettingTime);
	}

	// print verification parameter protected
	scaleValue.type = ADC_VERIF_PARAM_PROTECTED;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("verification parameter protected: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		if (scaleValue.ScaleValue.verifParamProtected == 0)
			printf("no\n");
		else
			printf("yes\n");
	}

	// print update allowed
	scaleValue.type = ADC_UPDATE_ALLOWED;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("update allowed: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		if (scaleValue.ScaleValue.updateAllowed == 0)
			printf("no\n");
		else
			printf("yes\n");
	}

	// print warm up time
	scaleValue.type = ADC_WARM_UP_TIME;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("warm up time: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		printf("%ld sec\n", scaleValue.ScaleValue.warmUpTime);
	}

	// print remaining warm up time
	scaleValue.type = ADC_REMAINING_WARM_UP_TIME;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("remaining warm up time: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		printf("%ld sec\n", scaleValue.ScaleValue.warmUpTime);
	}

	// print tilt compensation
	scaleValue.type = ADC_TILT_COMP;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("tilt compensation: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		if (scaleValue.ScaleValue.tiltCompensation.state.bit.tcOn) printf("tilt compensation on\n");
		else printf("tilt compensation off\n");
		if (scaleValue.ScaleValue.tiltCompensation.state.bit.tcCornerLoadCorrectionOn) printf("tilt corner load correction on\n");
		else printf("tilt corner load correction off\n");
		if (scaleValue.ScaleValue.tiltCompensation.state.bit.tcWeightDependentTiltAngleOn) printf("weight dependent tilt angle on\n");
		else printf("weight dependent tilt angle off\n");
	}

	// print state automatic tilt sensor
	scaleValue.type = ADC_AUTOMATIC_TILT_SENSOR;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	printf("automatic tilt sensor: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		if (scaleValue.ScaleValue.state == false)
			printf(" disable\n");
		else
			printf(" enable\n");
	}

	// print tare limit device
	tare.type = ADC_TARE_LIMIT_DEVICE;
	errorCode = AdcGetTare(g_adcHandle, &adcState, &tare);
	printf("tare limit device: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		// print tare
		FormatValue(tare.value.value, tare.value.decimalPlaces, &valueString);
		cout << valueString << " " << GetWeightUnit(tare.value.weightUnit) << endl;
	}

	// print tare limit known
	tare.type = ADC_TARE_LIMIT_CW_CUSTOM;
	errorCode = AdcGetTare(g_adcHandle, &adcState, &tare);
	printf("tare limit catchweigher custom: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		// print tare
		FormatValue(tare.value.value, tare.value.decimalPlaces, &valueString);
		cout << valueString << " " << GetWeightUnit(tare.value.weightUnit) << endl;
	}

	// print tare limit weighed
	tare.type = ADC_TARE_LIMIT_WEIGHED_CUSTOM;
	errorCode = AdcGetTare(g_adcHandle, &adcState, &tare);
	printf("tare limit weighed custom: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		// print tare
		FormatValue(tare.value.value, tare.value.decimalPlaces, &valueString);
		cout << valueString << " " << GetWeightUnit(tare.value.weightUnit) << endl;
	}

	// print tare limit known
	tare.type = ADC_TARE_LIMIT_KNOWN_CUSTOM;
	errorCode = AdcGetTare(g_adcHandle, &adcState, &tare);
	printf("tare limit known custom: ");
	if (!PrintErrorCode(errorCode, 0))
	{
		// print tare
		FormatValue(tare.value.value, tare.value.decimalPlaces, &valueString);
		cout << valueString << " " << GetWeightUnit(tare.value.weightUnit) << endl;
	}

}


/**
******************************************************************************
* DoSetScaleValues - set scale values
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoSetScaleValues(void)
{
    short           errorCode;
    char            *pStrPointer;
    short           state = 0, idx = 0, idxDot = 0;
    char            price[255];
    char            currency[255];
    AdcScaleValues  scaleValue;
    short           selection;
	short			frozen;
	short			mode;
	AdcParamMode	paramMode = AdcParamMode::ADC_SAVE_SENSOR_HEALTH_DATA;

	printf("\tSet baseprice\t\t\t%d\n", AdcValueType::ADC_BASEPRICE);
	printf("\tSet display text\t\t%d\n", AdcValueType::ADC_DISPLAY_TEXT);
	printf("\tSet scale mode\t\t\t%d\n", AdcValueType::ADC_SCALE_MODE);
	printf("\tSet eeprom\t\t\t%d\n", AdcValueType::ADC_EEPROM);
	printf("\tSet tilt compensation\t\t%d\n", AdcValueType::ADC_TILT_COMP);
	printf("\tSet scale filter\t\t%d\n", AdcValueType::ADC_FILTER);
	printf("\tSet gFactor\t\t\t%d\n", AdcValueType::ADC_G_FACTOR);
	printf("\tSet production settings\t\t%d\n", AdcValueType::ADC_PRODUCTION_SETTINGS);
	printf("\tConversion Weight Unit\t\t%d\n", AdcValueType::ADC_CONVERSION_WEIGHT_UNIT);
	printf("\tSet interface mode\t\t%d\n", AdcValueType::ADC_INTERFACE_MODE);
	printf("\tSet zero point tracking mode\t%d\n", AdcValueType::ADC_ZERO_POINT_TRACKING);
	printf("\tSet verif. param. protected\t%d\n", AdcValueType::ADC_VERIF_PARAM_PROTECTED);
	printf("\tSet update allowed\t\t%d\n", AdcValueType::ADC_UPDATE_ALLOWED);
	printf("\tSet warm up time\t\t%d\n", AdcValueType::ADC_WARM_UP_TIME);
	printf("\tSet spirit level calibration\t%d\n", AdcValueType::ADC_SPIRIT_LEVEL_CAL_MODE);
	printf("\tSet zero setting interval\t%d\n", AdcValueType::ADC_ZERO_SETTING_INTERVAL);
	printf("\tSet automatic zero setting time\t%d\n", AdcValueType::ADC_AUTOMATIC_ZERO_SETTING_TIME);
	printf("\tSet initial zero setting\t%d\n", AdcValueType::ADC_INITIAL_ZERO_SETTING);
	printf("\tSet state auto. tilt sensor\t%d\n", AdcValueType::ADC_AUTOMATIC_TILT_SENSOR);
	printf("\tTilt angle confirm\t%d\n", AdcValueType::ADC_TILT_ANGLE_CONFIRM);

	printf("\tADC parameters\t\t\t%d\n\n", ADC_PARAMETER_MODE);

    selection = (short)GetNumeric((char *)"select: ");

    switch (selection)
    {
	case AdcValueType::ADC_BASEPRICE:

        pStrPointer = GetString((char *)"input baseprice + currency [e.g. 1.00 EUR]: ", 255);

		frozen = (short)GetNumeric((char *)"const [0: no  1:yes]");

        // extract price and currency
        state = 0;
        for (int i = 0; i < (int)strlen(pStrPointer); i++)
        {
            // separator price - currency
            if (pStrPointer[i] == 0x20)
            {
                state = 1;
                idx = 0;
                scaleValue.ScaleValue.basePrice.price.decimalPlaces = i - idxDot - 1;
                continue;
            }

            // dot price
            if (pStrPointer[i] == '.')
            {
                idxDot = i;
                continue;
            }

            if (state == 0)
            {
                // price
                price[idx++] = pStrPointer[i];
                price[idx] = '\0';
            }
            else if (state == 1)
            {
                // currency
                currency[idx++] = pStrPointer[i];
                currency[idx] = '\0';
            }

        }

		scaleValue.type = AdcValueType::ADC_BASEPRICE;
        sscanf(price, "%lld", &scaleValue.ScaleValue.basePrice.price.value);
        strcpy(scaleValue.ScaleValue.basePrice.price.currency, currency);
		scaleValue.ScaleValue.basePrice.weightUnit = AdcWeightUnit::ADC_KILOGRAM;
		scaleValue.frozen = frozen;
        errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
        PrintErrorCode(errorCode, 1);
        break;

	case AdcValueType::ADC_DISPLAY_TEXT:
        pStrPointer = GetString((char *)"input display text: ", 255);
        
		scaleValue.type = AdcValueType::ADC_DISPLAY_TEXT;
        strcpy(scaleValue.ScaleValue.displayText.text, pStrPointer);
        scaleValue.ScaleValue.displayText.x = 0;
        scaleValue.ScaleValue.displayText.y = 0;

        errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
        PrintErrorCode(errorCode, 1);
        break;

	case AdcValueType::ADC_SCALE_MODE:
		printf("\tpure scale\t\t\t%d\n", AdcScaleMode::ADC_PURE_SCALE);
		printf("\tsb mode\t\t\t\t%d\n", AdcScaleMode::ADC_SCALE_SB_MODE);
		printf("\tas mode\t\t\t\t%d\n", AdcScaleMode::ADC_SCALE_AS_MODE);
		printf("\tpa mode\t\t\t\t%d\n", AdcScaleMode::ADC_SCALE_PA_MODE);
		printf("\tzg mode\t\t\t\t%d\n", AdcScaleMode::ADC_SCALE_ZG_MODE);
		printf("\tcw mode\t\t\t\t%d\n", AdcScaleMode::ADC_SCALE_CW_MODE);
        selection = (short)GetNumeric((char *)"select: ");
        
		scaleValue.type = AdcValueType::ADC_SCALE_MODE;
        scaleValue.ScaleValue.scaleMode = (AdcScaleMode) selection;

        errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
        PrintErrorCode(errorCode, 1);
        break;

	case AdcValueType::ADC_EEPROM:
		EepromReadWrite(&scaleValue, false);
        break;

	case AdcValueType::ADC_TILT_COMP:
		scaleValue.type = AdcValueType::ADC_TILT_COMP;
		scaleValue.ScaleValue.tiltCompensation.state.all = 0;
        scaleValue.ScaleValue.tiltCompensation.state.bit.tcOn = GetNumeric((char *)"tilt compensaton [0: disable, 1: enable]");
		scaleValue.ScaleValue.tiltCompensation.state.bit.tcCornerLoadCorrectionOn = GetNumeric((char *)"tilt corner load correction [0: disable, 1: enable]");
		scaleValue.ScaleValue.tiltCompensation.state.bit.tcWeightDependentTiltAngleOn = GetNumeric((char *)"weight dependent tilt angle [0: disable, 1: enable]");
		scaleValue.ScaleValue.tiltCompensation.limit_angle_1 = GetNumeric((char *)"input limit angle 1 [4\xF8 = 285]");
        errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
        PrintErrorCode(errorCode, 1);
        break;

	case AdcValueType::ADC_TILT_ANGLE_CONFIRM:
		long tiltAngleConfirmCmd;

		scaleValue.type = AdcValueType::ADC_TILT_ANGLE_CONFIRM;
		tiltAngleConfirmCmd = GetNumeric((char *)"angle take over [0: reset 1: only greater, 2: always]");
		scaleValue.ScaleValue.tiltAngleConfirmCmd = ADC_CONFIRMED_TILT_ANGLE_RESET;
		if (tiltAngleConfirmCmd == ADC_CONFIRM_CURRENT_MAX_TILT_ANGLE)
			scaleValue.ScaleValue.tiltAngleConfirmCmd = ADC_CONFIRM_CURRENT_MAX_TILT_ANGLE;
		else if (tiltAngleConfirmCmd == ADC_CONFIRM_CURRENT_MAX_TILT_ANGLE_FORCE)
			scaleValue.ScaleValue.tiltAngleConfirmCmd = ADC_CONFIRM_CURRENT_MAX_TILT_ANGLE_FORCE;

		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_FILTER:
		scaleValue.type = AdcValueType::ADC_FILTER;
		scaleValue.ScaleValue.filter.idx = (long)GetNumeric((char *)"Filter index: ");
		scaleValue.ScaleValue.filter.offsetStableTime = (long)GetNumeric((char *)"Offset stable time: ");
		scaleValue.ScaleValue.filter.stableRange = (long)GetNumeric((char *)"stable range [d]: ");

		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_G_FACTOR:
		scaleValue.type = AdcValueType::ADC_G_FACTOR;
		scaleValue.frozen = 0;
		scaleValue.ScaleValue.gFactor = (short)GetNumeric((char *)"gFactor [promille * 100]: ");

		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_PRODUCTION_SETTINGS:
	{
		AdcScaleValues scaleValue;
		short gFactorOffset = 0;
		time_t			rawtime;
		struct tm 		*timeinfo;

		// set production site
		printf("Production site local: \t0\n");
		printf("Production site Balingen: \t1\n");
		printf("Production site WeighTec: \t2\n");
		short site = (short)GetNumeric((char *)"site: ");

		if (site)
		{
			gFactorOffset = (short)GetNumeric((char *)"gFactor offset: ");
		}

		scaleValue.type = AdcValueType::ADC_PRODUCTION_SETTINGS;
		scaleValue.frozen = 0;
		scaleValue.ScaleValue.prodSettings.size = sizeof(AdcProdSettings);
		scaleValue.ScaleValue.prodSettings.prodSiteID = site;
		scaleValue.ScaleValue.prodSettings.gFactorOffset = gFactorOffset;
		// get local time
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(scaleValue.ScaleValue.prodSettings.date, sizeof(scaleValue.ScaleValue.prodSettings.date), "%y%m%d", timeinfo);
		// set wsc string
		scaleValue.ScaleValue.prodSettings.wsc[0] = 0;

		PrintErrorCode(AdcSetScaleValues(g_adcHandle, &scaleValue), 0);

		break;
	}

	case AdcValueType::ADC_CONVERSION_WEIGHT_UNIT:
		scaleValue.type = AdcValueType::ADC_CONVERSION_WEIGHT_UNIT;
		printf("\tcorrection factor Standard\t1000000000\n");
		printf("\tcorrection factor 15 lbs\t0881848937\n");
		printf("\tcorrection factor 30 lbs\t1102311172\n");
		printf("\tcorrection factor 60 lbs\t1102311172\n");
		printf("\tcorrection factor 15 -> 12 kg\t1250000000\n");
		scaleValue.ScaleValue.conversionWeightUnitFactor = (long)GetNumeric((char *)"Factor: ");

		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_INTERFACE_MODE:
		scaleValue.type = AdcValueType::ADC_INTERFACE_MODE;
		scaleValue.ScaleValue.interfaceMode.autoDetection = (short)GetNumeric((char *)"automatic detection [0: disable   1: enable] ");
		if (scaleValue.ScaleValue.interfaceMode.autoDetection == 0)
		{
			printf("\t0\tBB UART1/USB-Stecker	SIO-Bus UART12/HiRose\n");
			printf("\t1\tBB UART1 / USB - Stecker\n");
			printf("\t2\tBB UART1 / HiRose		RBS USB / USB - Stecker\n");
			printf("\t3\tBB UART1 / USB - Stecker	MB UART12 / HiRose\n");
			printf("\t4\tBB USB / USB - Stecker\n");
			printf("\t10\tBB UART1 / USB - Stecker	RBS UART12 / HiRose\n");
			printf("\t12\tBB UART1 / HiRose\n");
			scaleValue.ScaleValue.interfaceMode.mode = (short)GetNumeric((char *)"mode: ");
		}
		else
		{
			scaleValue.ScaleValue.interfaceMode.mode = 0;
		}
		
		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_ZERO_POINT_TRACKING:
		scaleValue.type = AdcValueType::ADC_ZERO_POINT_TRACKING;
		scaleValue.ScaleValue.mode = (short)GetNumeric((char *)"zero point tracking [0: disable   1: enable] ");
		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_VERIF_PARAM_PROTECTED:
		scaleValue.type = AdcValueType::ADC_VERIF_PARAM_PROTECTED;
		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_UPDATE_ALLOWED:
		scaleValue.type = AdcValueType::ADC_UPDATE_ALLOWED;
		scaleValue.ScaleValue.mode = (short)GetNumeric((char *)"update [0: not allowed   1: allowed] ");
		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_WARM_UP_TIME:
		scaleValue.type = AdcValueType::ADC_WARM_UP_TIME;
		scaleValue.ScaleValue.warmUpTime = (short)GetNumeric((char *)"warm up time [seconds] ");
		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_ZERO_SETTING_INTERVAL:
		scaleValue.type = AdcValueType::ADC_ZERO_SETTING_INTERVAL;
		scaleValue.ScaleValue.zeroSettingInterval = (short)GetNumeric((char *)"zero setting interval [seconds] ");
		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_AUTOMATIC_ZERO_SETTING_TIME:
		scaleValue.type = AdcValueType::ADC_AUTOMATIC_ZERO_SETTING_TIME;
		scaleValue.ScaleValue.automaticZeroSettingTime = (short)GetNumeric((char *)"automatic zero setting time [seconds] ");
		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_SPIRIT_LEVEL_CAL_MODE:
		scaleValue.type = AdcValueType::ADC_SPIRIT_LEVEL_CAL_MODE;
		mode = (short)GetNumeric((char *)"calibration [0: reset   1: set] ");
		if (mode == 0)
			scaleValue.ScaleValue.spiritLevelCmd = ADC_SPIRIT_LEVEL_RESET;
		else
			scaleValue.ScaleValue.spiritLevelCmd = ADC_SPIRIT_LEVEL_SET_ZERO;
		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_INITIAL_ZERO_SETTING:
		scaleValue.type = AdcValueType::ADC_INITIAL_ZERO_SETTING;
		while (1)
		{
			mode = (short)GetNumeric((char *)"neg. initial zero setting [%] ");
			if (mode > 0)
				printf("\n\tError: value must be <= 0\n\n");
			else
				break;
		}
		scaleValue.ScaleValue.initialZeroSettingParam.lowerLimit = mode;
		while (1)
		{
			mode = (short)GetNumeric((char *)"pos. initial zero setting [%] ");
			if (mode < 0)
				printf("\n\tError: value must be >= 0\n\n");
			else
				break;
		}
		scaleValue.ScaleValue.initialZeroSettingParam.upperLimit = mode;
		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case AdcValueType::ADC_AUTOMATIC_TILT_SENSOR:
		short mode;
		scaleValue.type = AdcValueType::ADC_AUTOMATIC_TILT_SENSOR;
		mode = (short)GetNumeric((char *)"auto. tilt sensor [0: disable   1: enable] ");
		if (mode)
			scaleValue.ScaleValue.state = true;
		else
			scaleValue.ScaleValue.state = false;
		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 1);
		break;

	case ADC_PARAMETER_MODE:
		printf("\tload default config. sensor health\t\t\t%d\n", AdcParamMode::ADC_LOAD_DEF_CONFIG_SENSOR_HEALTH_DATA);
		printf("\tsave sensor health data\t\t\t%d\n", AdcParamMode::ADC_SAVE_SENSOR_HEALTH_DATA);
		printf("\tfactory settings\t\t\t%d\n", AdcParamMode::ADC_FACTORY_SETTINGS);
		printf("\treset sensor health data\t\t\t%d\n", AdcParamMode::ADC_RESET_SENSOR_HEALTH_DATA);

		mode = (short)GetNumeric((char *)"mode: ");

		if (mode == AdcParamMode::ADC_LOAD_DEF_CONFIG_SENSOR_HEALTH_DATA)
			paramMode = AdcParamMode::ADC_LOAD_DEF_CONFIG_SENSOR_HEALTH_DATA;
		else if (mode == AdcParamMode::ADC_FACTORY_SETTINGS)
			paramMode = AdcParamMode::ADC_FACTORY_SETTINGS;
		else if (mode == AdcParamMode::ADC_RESET_SENSOR_HEALTH_DATA)
			paramMode = AdcParamMode::ADC_RESET_SENSOR_HEALTH_DATA;
		else 
			paramMode = AdcParamMode::ADC_SAVE_SENSOR_HEALTH_DATA;

		errorCode = AdcParameters(g_adcHandle, paramMode);
		PrintErrorCode(errorCode, 1);
		break;

    default:
        printf("invalid input\n");
        break;
   }
}


/**
******************************************************************************
* DoGetScaleValues - get scale values
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoGetScaleValues(void)
{
    short           errorCode;
    AdcScaleValues  scaleValue;
    short           selection;

	printf("\tGet baseprice\t\t\t%d\n", AdcValueType::ADC_BASEPRICE);
	printf("\tGet display text\t\t%d\n", AdcValueType::ADC_DISPLAY_TEXT);
	printf("\tGet scale mode\t\t\t%d\n", AdcValueType::ADC_SCALE_MODE);
	printf("\tGet eeprom\t\t\t%d\n", AdcValueType::ADC_EEPROM);
	printf("\tGet tilt compensation\t\t%d\n", AdcValueType::ADC_TILT_COMP);
	printf("\tGet filter\t\t\t%d\n", AdcValueType::ADC_FILTER);
	printf("\tGet interface Mode\t\t%d\n\n", AdcValueType::ADC_INTERFACE_MODE);
	printf("\tGet verif. param. protected\t%d\n", AdcValueType::ADC_VERIF_PARAM_PROTECTED);
	printf("\tGet update allowed\t\t%d\n", AdcValueType::ADC_UPDATE_ALLOWED);
	printf("\tGet remaining warm up time\t%d\n", AdcValueType::ADC_REMAINING_WARM_UP_TIME);
	printf("\tGet load capacity params\t%d\n", AdcValueType::ADC_LOAD_CAPACITY_PARAMS);
	printf("\tGet initial zero setting\t%d\n", AdcValueType::ADC_INITIAL_ZERO_SETTING);
	printf("\tGet state auto. tilt sensor\t%d\n", AdcValueType::ADC_AUTOMATIC_TILT_SENSOR);

	
    selection = (short)GetNumeric((char *)"select: ");

    switch (selection)
    {
	case AdcValueType::ADC_BASEPRICE:
		 scaleValue.type = AdcValueType::ADC_BASEPRICE;
         errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
         if (!PrintErrorCode(errorCode, 1))
         {
             string valueString;

			 // print base price
			 FormatValue(scaleValue.ScaleValue.basePrice.price.value, scaleValue.ScaleValue.basePrice.price.decimalPlaces, &valueString);
             printf("Baseprice: %s %s/%s\n", valueString.c_str(), scaleValue.ScaleValue.basePrice.price.currency, GetWeightUnit(scaleValue.ScaleValue.basePrice.weightUnit));
         }
         break;

	case AdcValueType::ADC_DISPLAY_TEXT:
		 scaleValue.type = AdcValueType::ADC_DISPLAY_TEXT;
         errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
         if (!PrintErrorCode(errorCode, 1))
         {
             printf("DisplayText [%d/%d]: %s\n", scaleValue.ScaleValue.displayText.x, scaleValue.ScaleValue.displayText.y, scaleValue.ScaleValue.displayText.text);
         }
         break;

	case AdcValueType::ADC_SCALE_MODE:
		 scaleValue.type = AdcValueType::ADC_SCALE_MODE;
         errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
         if (!PrintErrorCode(errorCode, 1))
         {
             printf("Scale mode: %s\n", GetScaleMode(scaleValue.ScaleValue.scaleMode));
         }
         break;

	case AdcValueType::ADC_EEPROM:
		 EepromReadWrite(&scaleValue, true);
         break;

	case AdcValueType::ADC_TILT_COMP:
		 scaleValue.type = AdcValueType::ADC_TILT_COMP;
         errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
         if (!PrintErrorCode(errorCode, 1))
         {
			 printf("Tilt compensation state: %d  limit angle 1: %ld  limit angle 2: %ld  resolution: %ld\n", scaleValue.ScaleValue.tiltCompensation.state.all, scaleValue.ScaleValue.tiltCompensation.limit_angle_1, scaleValue.ScaleValue.tiltCompensation.limit_angle_2, scaleValue.ScaleValue.tiltCompensation.resolution);
             printf("Tilt compensation x: %ld  y: %ld\n", scaleValue.ScaleValue.tiltCompensation.x, scaleValue.ScaleValue.tiltCompensation.y);
		 }
         break;

	case AdcValueType::ADC_FILTER:
		 scaleValue.type = AdcValueType::ADC_FILTER;
		 errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
		 if (!PrintErrorCode(errorCode, 1))
		 {
			 printf("filter parameter: Idx: %ld  offset stable time: %ld  stable range: %ld\n", scaleValue.ScaleValue.filter.idx, scaleValue.ScaleValue.filter.offsetStableTime, scaleValue.ScaleValue.filter.stableRange);
		 }
		 break;

	case AdcValueType::ADC_INTERFACE_MODE:
		scaleValue.type = AdcValueType::ADC_INTERFACE_MODE;
		errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
		if (!PrintErrorCode(errorCode, 1))
		{
			printf("automatic detection: %d\n", scaleValue.ScaleValue.interfaceMode.autoDetection);
			printf("interface mode: %d\t", scaleValue.ScaleValue.interfaceMode.mode);
			switch (scaleValue.ScaleValue.interfaceMode.mode)
			{
			case 0: printf("BB UART1/USB-Stecker	SIO-Bus UART12/HiRose\n"); break;
			case 1: printf("BB UART1 / USB - Stecker\n"); break;
			case 2: printf("BB UART1 / HiRose		RBS USB / USB - Stecker\n"); break;
			case 3: printf("BB UART1 / USB - Stecker	MB UART12 / HiRose\n"); break;
			case 4: printf("BB USB / USB - Stecker\n"); break;
			case 10: printf("BB UART1 / USB - Stecker	RBS UART12 / HiRose\n"); break;
			case 12: printf("BB UART1 / HiRose\n"); break;
			default: printf("unknown\n"); break;
			}
		}
		break;


	case AdcValueType::ADC_VERIF_PARAM_PROTECTED:
		scaleValue.type = AdcValueType::ADC_VERIF_PARAM_PROTECTED;
		errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
		if (!PrintErrorCode(errorCode, 1))
		{
			printf("verification parameter ");
			if (scaleValue.ScaleValue.verifParamProtected == 0)
				printf(" not protected\n");
			else
				printf(" protected\n");
		}
		break;

	case AdcValueType::ADC_UPDATE_ALLOWED:
		scaleValue.type = AdcValueType::ADC_UPDATE_ALLOWED;
		errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
		if (!PrintErrorCode(errorCode, 1))
		{
			printf("update ");
			if (scaleValue.ScaleValue.updateAllowed == 0)
				printf(" not allowed\n");
			else
				printf(" allowed\n");
		}
		break;

	case AdcValueType::ADC_REMAINING_WARM_UP_TIME:
		scaleValue.type = AdcValueType::ADC_REMAINING_WARM_UP_TIME;
		errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
		if (!PrintErrorCode(errorCode, 1))
		{
			printf("remaining warm up time %ld seconds\n", scaleValue.ScaleValue.warmUpTime);
		}
		break;

	case AdcValueType::ADC_LOAD_CAPACITY_PARAMS:
		scaleValue.type = AdcValueType::ADC_LOAD_CAPACITY_PARAMS;
		errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
		if (!PrintErrorCode(errorCode, 1))
		{
			printf("\nload capacity: %s\t\tmax capacity: %ld\n", scaleValue.ScaleValue.loadCapacityParams.loadCapacity, scaleValue.ScaleValue.loadCapacityParams.maxCapacity);
			printf("unit: %s\t\t\tmultiInterval: %ld\n", GetWeightUnit(scaleValue.ScaleValue.loadCapacityParams.unit), scaleValue.ScaleValue.loadCapacityParams.multiInterval);
			printf("limit Interval1: %ld\t\tlimit Interval2: %ld\n", scaleValue.ScaleValue.loadCapacityParams.limitInterval1, scaleValue.ScaleValue.loadCapacityParams.limitInterval2);
			printf("tare limit weighed: %ld\ttare limit known: %ld\n", scaleValue.ScaleValue.loadCapacityParams.tareLimitWeighed, scaleValue.ScaleValue.loadCapacityParams.tareLimitKnown);
			printf("e1: %ld\t\t\te2: %ld\t\t\te3: %ld\n", scaleValue.ScaleValue.loadCapacityParams.e1, scaleValue.ScaleValue.loadCapacityParams.e2, scaleValue.ScaleValue.loadCapacityParams.e3);
			printf("decimal places 1: %d\tdecimal places 2: %d\tdecimal places 3: %d\n", scaleValue.ScaleValue.loadCapacityParams.decimalPlaces1, scaleValue.ScaleValue.loadCapacityParams.decimalPlaces2, scaleValue.ScaleValue.loadCapacityParams.decimalPlaces3);
		}
		break;

	case AdcValueType::ADC_INITIAL_ZERO_SETTING:
		scaleValue.type = AdcValueType::ADC_INITIAL_ZERO_SETTING;
		errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
		if (!PrintErrorCode(errorCode, 1))
		{
			printf("\ninitial zero setting neg. %d %%    pos. %d %%\n", scaleValue.ScaleValue.initialZeroSettingParam.lowerLimit, scaleValue.ScaleValue.initialZeroSettingParam.upperLimit);
		}
		break;

	case AdcValueType::ADC_AUTOMATIC_TILT_SENSOR:
		scaleValue.type = AdcValueType::ADC_AUTOMATIC_TILT_SENSOR;
		errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
		if (!PrintErrorCode(errorCode, 1))
		{
			printf("\nstate auto.tilt sensor");
			if (scaleValue.ScaleValue.state == false)
				printf(" disable\n");
			else
				printf(" enable\n");
		}
		break;

     default:
         printf("invalid input\n");
         break;
     }
}

/**
******************************************************************************
* EepromReadWrite - read or write data to eeprom
*
* @param
* @return
* @remarks
******************************************************************************
*/
void EepromReadWrite(AdcScaleValues *scaleValue, bool read)
{
	short   errorCode;
	short   selection;
	char    *pStrPointer;
	AdcEepromSize	sEepromSize;
	long	eepromSize;

	if (!EepromGetSize(&sEepromSize))
	{
		return;
	}

	scaleValue->type = AdcValueType::ADC_EEPROM;

	selection = (short)GetNumeric((char *)"input region [0: welmec    1:open     2:prod     3:prod-sensors]:");
	if (selection == 0)
	{
		scaleValue->ScaleValue.eeprom.region = ADC_WELMEC_REGION;
		eepromSize = sEepromSize.welmecRegion;
	}
	else if (selection == 1)
	{
		scaleValue->ScaleValue.eeprom.region = ADC_OPEN_REGION;
		eepromSize = sEepromSize.openRegion;
	}
	else if (selection == 2)
	{
		scaleValue->ScaleValue.eeprom.region = ADC_PROD_REGION;
		eepromSize = sEepromSize.prodRegion;
	}
	else if (selection == 3)
	{
		scaleValue->ScaleValue.eeprom.region = ADC_PROD_SENSORS_REGION;
		eepromSize = sEepromSize.prodSensorsRegion;
	}
	else
	{
		printf("\n\tError: Invalid input\n\n");
		return;
	}


	scaleValue->ScaleValue.eeprom.startAdr = -1;
	while (1)
	{
		pStrPointer = GetString((char *)"input start address [hex]: ", 6);
		sscanf(pStrPointer, "0x%08lx", &scaleValue->ScaleValue.eeprom.startAdr);
		if (scaleValue->ScaleValue.eeprom.startAdr == -1)
			printf("\n\tError: Invalid start address, format 0x...\n\n");
		else if (scaleValue->ScaleValue.eeprom.startAdr > eepromSize)
			printf("\n\tError: start address must be <= %lx\n\n", eepromSize);
		else
			break;
	}

	while (1)
	{
		scaleValue->ScaleValue.eeprom.len = (short)GetNumeric((char *)"input length: ");
		if (scaleValue->ScaleValue.eeprom.startAdr + scaleValue->ScaleValue.eeprom.len > eepromSize)
		{
			printf("\n\tError: length must be <= %ld\n\n", eepromSize - scaleValue->ScaleValue.eeprom.startAdr + 1);
		}
		else
			break;
	}

	if (read == true)
	{
		// read eeprom
		errorCode = AdcGetScaleValues(g_adcHandle, scaleValue);
		if (!PrintErrorCode(errorCode, 1))
		{
			for (short i = 0; i < scaleValue->ScaleValue.eeprom.len; i++)
			{
				if (!(i % 8)) printf("\n0x%08lx", scaleValue->ScaleValue.eeprom.startAdr + i);
				printf("  %02x", scaleValue->ScaleValue.eeprom.data[i]);
			}
			printf("\n");
		}
	}
	else
	{
		// write eeprom
		selection = -1;
		while (1)
		{
			pStrPointer = GetString((char *)"start value [hex]: ", 4);
			sscanf(pStrPointer, "0x%02hx", &selection);
			if (selection == -1)
				printf("\n\tError: Invalid start value, format 0x...\n\n");
			else
				break;
		}
		for (short i = selection; i < selection + scaleValue->ScaleValue.eeprom.len; i++)
			scaleValue->ScaleValue.eeprom.data[i - selection] = (unsigned char)i;

		errorCode = AdcSetScaleValues(g_adcHandle, scaleValue);
		PrintErrorCode(errorCode, 1);
	}

	return;
}


/**
******************************************************************************
* EepromGetSize - read eeprom size from adc
*
* @param
* @return
* @remarks
******************************************************************************
*/
bool EepromGetSize(AdcEepromSize *eepromSize)
{
	short errorCode;
	AdcScaleValues  scaleValue;

	scaleValue.type = ADC_EEPROM_SIZE;

	// read eeprom
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	if (!PrintErrorCode(errorCode, 1))
	{
		*eepromSize = scaleValue.ScaleValue.eepromSize;
		return true;
	}
	else
		return false;
}

/**
******************************************************************************
* DoZeroScale - zero scale
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoZeroScale(void)
{
    short       errorCode;
    AdcState    adcState;

    errorCode = AdcZeroScale(g_adcHandle, &adcState);
    if (!PrintErrorCode(errorCode, 1))
    {
        PrintAdcState(adcState);
    }
}


/**
******************************************************************************
* DoHandleTare - set/clear tare
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoHandleTare(void)
{
    short       errorCode;
    AdcState    adcState;
    AdcTare     tare;
    short       selection;

    long select = GetNumeric((char *)"0: set tare   1: set tare const   2: clear tare   3: prio tare");

	if ((select == 0) || (select == 1))
    {

		if (select == 0) tare.frozen = 0;
		else tare.frozen = 1;

		printf("\tweighed\t\t\t%d\n", AdcTareType::ADC_TARE_WEIGHED);
		printf("\tknown\t\t\t%d\n", AdcTareType::ADC_TARE_KNOWN);
		printf("\tweighed/set zero\t%d\n", AdcTareType::ADC_TARE_WEIGHED_SET_ZERO);
		printf("\tpercent\t\t\t%d\n", AdcTareType::ADC_TARE_PERCENT);
		printf("\tset const attribut\t%d\n", AdcTareType::ADC_TARE_ATTRIBUTE_CONST);
		printf("\tlimit device\t\t%d\n", AdcTareType::ADC_TARE_LIMIT_DEVICE);
		printf("\tlimit cw custom\t\t%d\n", AdcTareType::ADC_TARE_LIMIT_CW_CUSTOM);
		printf("\tlimit weighed custom\t%d\n", AdcTareType::ADC_TARE_LIMIT_WEIGHED_CUSTOM);
		printf("\tlimit known custom\t%d\n", AdcTareType::ADC_TARE_LIMIT_KNOWN_CUSTOM);
       
        selection = (short)GetNumeric((char *)"select: ");

        tare.type = (AdcTareType) selection;

        if (selection == ADC_TARE_KNOWN)
        {
            selection = (short)GetNumeric((char *)"input known tare [g]: ");

            tare.value.value = selection;
			tare.value.weightUnit = AdcWeightUnit::ADC_GRAM;
            tare.value.decimalPlaces = 0;
        }
		else if (selection == ADC_TARE_PERCENT)
		{
			selection = (short)GetNumeric((char *)"input percent [%]: ");

			tare.percentage.value = selection;
			tare.percentage.unit = AdcUnit::ADC_PERCENT;
			tare.percentage.decimalPlaces = 0;
		}
		else if ((selection == ADC_TARE_LIMIT_DEVICE) ||
			     (selection == ADC_TARE_LIMIT_CW_CUSTOM) ||
				 (selection == ADC_TARE_LIMIT_WEIGHED_CUSTOM) ||
				 (selection == ADC_TARE_LIMIT_KNOWN_CUSTOM))
		{
			selection = (short)GetNumeric((char *)"input tare limit [g]: ");

			tare.value.value = selection;
			tare.value.weightUnit = AdcWeightUnit::ADC_GRAM;
			tare.value.decimalPlaces = 0;
		}

        errorCode = AdcSetTare(g_adcHandle, &adcState, &tare);
        if (!PrintErrorCode(errorCode, 1))
        {
            PrintAdcState(adcState);
        }
    }
    else if (select == 2)
    {
        errorCode = AdcClearTare(g_adcHandle, &adcState);
        if (!PrintErrorCode(errorCode, 1))
        {
            PrintAdcState(adcState);
        }
    }
    else if (select == 3)
    {
		printf("\tADC_STP_FIRST\t%d\n", AdcTarePriority::ADC_STP_FIRST);
		printf("\tADC_STP_NONE\t%d\n\n", AdcTarePriority::ADC_STP_NONE);

        selection = (short)GetNumeric((char *)"select: ");

        errorCode = AdcSetTarePriority(g_adcHandle, &adcState, (AdcTarePriority)selection);
        if (!PrintErrorCode(errorCode, 1))
        {
            PrintAdcState(adcState);
        }
    }
}


/**
******************************************************************************
* DoResetAdc - reset adc
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoResetAdc(void)
{
   short errorCode;
   short type;

   printf("\tHardware-Reset\t%d\n", AdcResetType::ADC_HARDWARE_RESET);
   printf("\tSoftware-Reset\t%d\n\n", AdcResetType::ADC_SOFTWARE_RESET);

   type = (short)GetNumeric((char *)"select: ");

   errorCode = AdcReset(g_adcHandle, (AdcResetType) type);
   PrintErrorCode(errorCode, 1);
}


/**
******************************************************************************
* DoReadWeight - get weight value from adc
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoReadWeight(void)
{
	short       errorCode, saveErrorCode = ADC_SUCCESS;
    short       registrationRequest;
    AdcState    adcState;
    AdcWeight   weight;
    AdcTare     tare;
    AdcBasePrice  basePrice;
    AdcPrice    sellPrice;
    short       selection;
    string      valueString;
    long long   saveWeightValue = 0;
    AdcState    saveAdcState;
	unsigned long	ulSleep;

	ulSleep = (unsigned long)GetNumeric((char *)"Pollinterval [ms]: ");
	selection = (short)GetNumeric((char *)"0: no registration   1: registration: ");

	if (selection != 0) registrationRequest = 1;
	else registrationRequest = 0;

	saveAdcState.state = 0;

	while (1)
	{
       errorCode = AdcReadWeight(g_adcHandle, registrationRequest, &adcState, &weight, &tare, &basePrice, &sellPrice);

	   if ((saveErrorCode != errorCode) || (saveWeightValue != weight.value) || (saveAdcState.state != adcState.state))
	   {
		   saveErrorCode = errorCode;

		   if (!PrintErrorCode(errorCode, 0))
		   {
			   saveWeightValue = weight.value;
			   saveAdcState.state = adcState.state;

			   // print out weight
			   FormatValue(weight.value, weight.decimalPlaces, &valueString);
			   cout << "Weight:  " << valueString << " " << GetWeightUnit(weight.weightUnit);

			   if ((tare.type != ADC_TARE_NO) && (tare.value.value != 0))
			   {
				   // print tare
				   FormatValue(tare.value.value, tare.value.decimalPlaces, &valueString);
				   cout << "  Tare:  " << valueString << " " << GetWeightUnit(tare.value.weightUnit);
			   }

			   if (basePrice.price.value != 0)
			   {
				   // print base price
				   FormatValue(basePrice.price.value, basePrice.price.decimalPlaces, &valueString);
				   cout << "  Baseprice:  " << valueString << " " << basePrice.price.currency << "/" << GetWeightUnit(basePrice.weightUnit);
			   }

			   if (sellPrice.value != 0)
			   {
				   // print sell price
				   FormatValue(sellPrice.value, sellPrice.decimalPlaces, &valueString);
				   cout << "  Sellprice:  " << valueString << " " << basePrice.price.currency;
			   }

			   cout << "\t";

			   // print out adcState
			   PrintAdcState(adcState, MODE_READ_WEIGHT, true);

			   cout << endl;
		   }
		   else
		   {
			   // error, reset values
			   saveAdcState.state = 0;
			   saveWeightValue = 0;
			   saveErrorCode = ADC_SUCCESS;
		   }
	   }
	   Sleep(ulSleep);

	   if (_kbhit())
	   {
		   char cChar = (char)_getch();

		   if (cChar == 't')
				DoHandleTare();
		   else if (cChar == 'z')
				DoZeroScale();
		   else
				break;
	   }
   } 

}


/**
******************************************************************************
* DoGetLogger - get logger
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoGetLogger(void)
{
    short  errorCode;
    char   logbook[1024];
    short  index = 1;
    unsigned long  size;
    std::map <string, string> logBookMap;

    while (1)
    {
        size = sizeof(logbook);
        errorCode = AdcGetLogger(g_adcHandle, index, logbook, &size);
        if (PrintErrorCode(errorCode, 0)) break;
        if (strlen(logbook))
        {
			printf("%d ", index);

            // print out one log book row
        	string logbookStr = logbook;
            if (GetKeyValuePair(logbookStr, logBookMap))
            {
                for (map<string, string>::iterator it = logBookMap.begin(); it != logBookMap.end(); ++it)
                {
                    printf(" [%s : %s] ", (*it).first.c_str(), (*it).second.c_str());
                }
                printf("\n");
            }
        }
        else
        {
            break;
        }
        index++;
    }
}


/**
******************************************************************************
* DoMakeTests - make test
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoMakeTests(void)
{
	short       selection;

	printf("\tAuthentication\t\t%d\n", 0);
	printf("\tEeprom\t\t%d\n", 1);
	printf("\tRegistration\t\t%d\n", 2);
	printf("\tUpdate\t\t%d\n", 3);
	printf("\tset/get/values\t\t%d\n", 4);
	printf("\trounding test\t\t%d\n", 5);


	selection = (short)GetNumeric((char *)"select: ");

	switch (selection)
	{
	case 0:
		MakeAuthenticationTest();
		break;
	case 1:
		MakeEepromTest();
		break;
	case 2:
		MakeRegistrationTest();
		break;
	case 3:
		MakeUpdateTest();
		break;
	case 4:
		MakeSetGetValueTest();
		break;
	case 5:
		MakeRoundingTest();
		break;
	}
}


/**
******************************************************************************
* MakeAuthenticationTest - make adc authentication test
*
* @param
* @return
* @remarks
******************************************************************************
*/
void MakeAuthenticationTest()
{
	stringstream    *logBookPtr;
	stringstream    logBookEntry1;
	stringstream    logBookEntry2;
	time_t			rawtime;
	struct tm 		*timeinfo;
	char			timeString[80];
	short			index = 0;
	short			errorCode;
	short			infinite;
	char			logbook[1024];
	unsigned long   size;
	std::map <string, string> logBookMap;

	infinite = (short)GetNumeric((char *)"endless [0: no   1: yes]: ");

	while (1)
	{
		index = 0;

		// get local time
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(timeString, sizeof(timeString), "%y%m%d%H%M", timeinfo);

		logBookEntry1 << STX << "vid" << FS << "13" << GS << \
			"cid" << FS << "129" << GS << \
			"swvl" << FS << "99" << GS << \
			"swid" << FS << "1234" << GS << \
			"swv" << FS << "2.00.0001" << GS << \
			"date" << FS << timeString << ETX;

		logBookEntry2 << STX << "vid" << FS << "13" << GS << \
			"cid" << FS << "129" << GS << \
			"swvl" << FS << "98" << GS << \
			"swid" << FS << "1234" << GS << \
			"swv" << FS << "3.00" << GS << \
			"date" << FS << timeString << ETX;

		while (1)
		{
			if ((index & 0x01) == 0x01)
			{
				logBookPtr = &logBookEntry1;
			}
			else
			{
				logBookPtr = &logBookEntry2;
			}
			if ((errorCode = MakeAuthentication(true, logBookPtr, true)) != ADC_SUCCESS)
			{
				break;
			}
			else
			{
				printf("%d. logbook entry\n", ++index);
			}
		}

		if (infinite && (errorCode == ADC_E_LOGBOOK_FULL))
		{
			// read logbook
			index = 1;
			
			while (1)
			{
				size = sizeof(logbook);
				errorCode = AdcGetLogger(g_adcHandle, index, logbook, &size);
				if (PrintErrorCode(errorCode, 0)) break;
				if (strlen(logbook))
				{
					printf("%d ", index);

					// print out one log book row
					string logbookStr = logbook;
					if (GetKeyValuePair(logbookStr, logBookMap))
					{
						for (map<string, string>::iterator it = logBookMap.begin(); it != logBookMap.end(); ++it)
						{
							printf(" [%s : %s] ", (*it).first.c_str(), (*it).second.c_str());
						}
						printf("\n");
					}
				}
				else
				{
					break;
				}
				index++;
			}
		}
		else
		{
			break;
		}

		if (infinite && (errorCode == ADC_E_LOGBOOK_NO_FURTHER_ENTRY))
		{
			printf("Software-Reset\n");
			errorCode = AdcReset(g_adcHandle, AdcResetType::ADC_SOFTWARE_RESET);
			if (PrintErrorCode(errorCode, 0)) break;
			Sleep(300);
		}
		else
		{
			break;
		}

		if (infinite && (errorCode == ADC_SUCCESS))
		{
			// read logbook
			index = 1;

			while (1)
			{
				size = sizeof(logbook);
				errorCode = AdcGetLogger(g_adcHandle, index, logbook, &size);
				if (PrintErrorCode(errorCode, 0)) break;
				if (strlen(logbook))
				{
					printf("%d ", index);

					// print out one log book row
					string logbookStr = logbook;
					if (GetKeyValuePair(logbookStr, logBookMap))
					{
						for (map<string, string>::iterator it = logBookMap.begin(); it != logBookMap.end(); ++it)
						{
							printf(" [%s : %s] ", (*it).first.c_str(), (*it).second.c_str());
						}
						printf("\n");
					}
				}
				else
				{
					break;
				}
				index++;
			}
		}
		else
		{
			break;
		}

		if (_kbhit()) break;
	}
}


/**
******************************************************************************
* MakeEepromTest - make eeprom test
*
* @param
* @return
* @remarks
******************************************************************************
*/
void MakeEepromTest()
{
	long			index = 1;
	short			errorCode = ADC_SUCCESS;
	AdcEepromSize	sEepromSize;
	long			eepromSize;
	AdcScaleValues  scaleValue;
	unsigned char	pattern1;
	unsigned char	pattern2;
	unsigned char	pattern;
	short			selection;
	char			*pStrPointer;

	scaleValue.type = ADC_EEPROM_SIZE;
	// read eeprom size
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	if (PrintErrorCode(errorCode, 0))
	{
		printf("\nError get eeprom size -> cancel\n");
		return;
	}
	sEepromSize = scaleValue.ScaleValue.eepromSize;

	selection = (short)GetNumeric((char *)"input region [0: welmec    1:open     2:prod     3:prod-sensors]:");
	if (selection == 0)
	{
		scaleValue.ScaleValue.eeprom.region = ADC_WELMEC_REGION;
		eepromSize = sEepromSize.welmecRegion;
	}
	else if (selection == 1)
	{
		scaleValue.ScaleValue.eeprom.region = ADC_OPEN_REGION;
		eepromSize = sEepromSize.openRegion;
	}
	else if (selection == 2)
	{
		scaleValue.ScaleValue.eeprom.region = ADC_PROD_REGION;
		eepromSize = sEepromSize.prodRegion;
	}
	else if (selection == 3)
	{
		scaleValue.ScaleValue.eeprom.region = ADC_PROD_SENSORS_REGION;
		eepromSize = sEepromSize.prodSensorsRegion;
	}
	else
	{
		printf("\nError: invalid region\n");
		return;
	}

	scaleValue.ScaleValue.eeprom.startAdr = -1;
	while (1)
	{
		pStrPointer = GetString((char *)"input start address [hex]: ", 6);
		sscanf(pStrPointer, "0x%08lx", &scaleValue.ScaleValue.eeprom.startAdr);
		if (scaleValue.ScaleValue.eeprom.startAdr == -1)
			printf("\n\tError: Invalid start address, format 0x...\n\n");
		else if (scaleValue.ScaleValue.eeprom.startAdr > eepromSize)
			printf("\n\tError: start address must be <= %lx\n\n", eepromSize);
		else
			break;
	}

	while (1)
	{
		scaleValue.ScaleValue.eeprom.len = (short)GetNumeric((char *)"input length: ");
		if (scaleValue.ScaleValue.eeprom.startAdr + scaleValue.ScaleValue.eeprom.len > eepromSize)
		{
			printf("\n\tError: length must be <= %ld\n\n", eepromSize - scaleValue.ScaleValue.eeprom.startAdr + 1);
		}
		else
			break;
	}

	scaleValue.type = AdcValueType::ADC_EEPROM;

	while (1)
	{
		printf("%ld. test run\n", index);

		//set pattern
		if ((index & 0x01) == 0x01)
		{
			pattern1 = 0x55;
			pattern2 = 0xAA;
		}
		else
		{
			pattern1 = 0xAA;
			pattern2 = 0x55;
		}

		for (int i = 0; i < scaleValue.ScaleValue.eeprom.len; i++)
		{
			if (i & 0x01)
				scaleValue.ScaleValue.eeprom.data[i] = pattern1;
			else
				scaleValue.ScaleValue.eeprom.data[i] = pattern2;
		}

		// write eeprom
		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		PrintErrorCode(errorCode, 0);

		if (errorCode == ADC_SUCCESS)
		{
			// read eeprom
			errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
			if (!PrintErrorCode(errorCode, 0))
			{
				for (int i = 0; i < scaleValue.ScaleValue.eeprom.len; i++)
				{
					if (i & 0x01)
					{
						pattern = pattern1;
					}
					else
					{
						pattern = pattern2;
					}
					if (scaleValue.ScaleValue.eeprom.data[i] != pattern)
					{
						printf("\neeprom error adr: 0x%lx  data expected: 0x%x  actual: 0x%x\n", scaleValue.ScaleValue.eeprom.startAdr + i, pattern, scaleValue.ScaleValue.eeprom.data[i]);
						errorCode = ADC_E_ADC_FAILURE;
					}
				}
			}
		}

		if (_kbhit() || errorCode != ADC_SUCCESS)
		{
			break;
		}

		index++;
	}

	return;
}

/**
******************************************************************************
* MakeRegistrationTest - make registration test
*
* @param
* @return
* @remarks
******************************************************************************
*/
void MakeRegistrationTest()
{
	long registrationOk = 0;
	long registrationNok = 0;
	short errorCode;
	unsigned long ulSleep;
	string valueString;
	string sollValueString = "";
	double sollValue;
	double currentValue;
	int eValue = 0;
	bool swIdentify;

	// print scale lcs state
	AdcState    adcState;
	AdcWeight   weight;
	AdcTare     tare;

	ulSleep = (unsigned long)GetNumeric((char *)"Pollinverval [ms]: ");

	while (sollValueString.length() == 0)
	{
		sollValueString = GetString((char *)"Target weight: ", 255);
		if (sollValueString.length() != 0)
		{
			// replace control sequence
			size_t pos;
			if ((pos = sollValueString.find(',')) != std::string::npos)
				sollValueString.replace(pos, 1, ".");
		}
	}

	while (!eValue)
	{
		eValue = (int)GetNumeric((char*)"e: ");
	}

	sollValue = std::stod(sollValueString);

	while (1)
	{
		errorCode = AdcReadWeight(g_adcHandle, 1, &adcState, &weight, &tare, NULL, NULL);

		if (!PrintErrorCode(errorCode, 0))
		{
			// check for authentication
			if ((adcState.bit.needAuthentication) || ((adcState.bit.needLogbook)))
			{
				swIdentify = false;
				if (adcState.bit.needLogbook)
				{
					swIdentify = true;
				}
				MakeAuthentication(swIdentify);
			}

			if (adcState.bit.weightRegistered)
			{
				// print out weight
				FormatValue(weight.value, weight.decimalPlaces, &valueString);
				currentValue = std::stod(valueString);

				if (abs(currentValue - sollValue) > eValue)
				{
					cout << "Registration nok, weight:  " << valueString << " " << GetWeightUnit(weight.weightUnit);
					registrationNok++;
				}
				else
					registrationOk++;
			}
		}

		// check for cancel
		if (_kbhit() || errorCode != ADC_SUCCESS)
		{
			break;
		}

		Sleep(ulSleep);
	}

	printf("Result:\n");
	printf("Registration ok: %ld\n", registrationOk);
	printf("Registration nok: %ld\n", registrationNok);
}


/**
******************************************************************************
* MakeUpdateTest - make update tests
*
* @param
* @return
* @remarks
******************************************************************************
*/
void MakeUpdateTest()
{
	short	errorCode;
	unsigned long ulSleep;
	string	dir;
	unsigned long updateOk;
	unsigned long updateNok;

	ulSleep = (unsigned long)GetNumeric((char *)"Pollinverval [ms]: ");
	dir = GetString((char *)"Enter firmware directory: ", 255);

	errorCode = AdcSetFirmwarePath(g_adcHandle, dir.c_str());
	if (!PrintErrorCode(errorCode, 0))
	{
		updateOk = 0;
		updateNok = 0;
		while (1)
		{
			errorCode = Update();

			if (!PrintErrorCode(errorCode, 0))
			{
				updateOk++;
			}
			else
			{
				updateNok++;
			}

			// check for cancel
			if (_kbhit())
			{
				break;
			}

			Sleep(ulSleep);
		}

		printf("Updates: %ld   ok: %ld   nok: %ld\n", updateNok + updateOk, updateOk, updateNok);
	}
}


/**
******************************************************************************
* MakeReadWeightTest - make read weight
*
* @param
* @return
* @remarks
******************************************************************************
*/
short MakeReadWeightTest(int number)
{
	short errorCode = ADC_SUCCESS;
	bool swIdentify;

	AdcState    adcState;
	AdcWeight   weight;
	AdcTare     tare;

	for (int idx = 0; idx < number; idx++)
	{
		errorCode = AdcReadWeight(g_adcHandle, 1, &adcState, &weight, &tare, NULL, NULL);

		if (!PrintErrorCode(errorCode, 0))
		{
			// check for authentication
			if ((adcState.bit.needAuthentication) || ((adcState.bit.needLogbook)))
			{
				swIdentify = false;
				if (adcState.bit.needLogbook)
				{
					swIdentify = true;
				}
				MakeAuthentication(swIdentify);
			}
		}
		else
		{
			break;
		}
	}

	return errorCode;
}


/**
******************************************************************************
* MakeSetGetValueTest - set settings / get settings /get weight values test
*
* @param
* @return
* @remarks
******************************************************************************
*/
void MakeSetGetValueTest()
{
	short			errorCode;
	AdcScaleValues  scaleValue;
	string			mode;

	short initalUpdateAllowed = 1;
	long initialWarmUpTime = 0xAAAA;
	AdcScaleMode initialScaleMode = AdcScaleMode::ADC_SCALE_AS_MODE;
	short initalgFactor = 100;
	short initalZeroPointTracking = 1;
	long initalZeroSettingInterval = 5000;
	long initialAutomaticZeroSettingTime = 5;
	bool initialStateAutomaticTiltSensor = true;


	for (int idx = 0; idx < 8; idx++)
	{
		switch (idx)
		{
		case 0:
			// set initial update allowed
			mode = "param update allowed";
			scaleValue.type = AdcValueType::ADC_UPDATE_ALLOWED;
			scaleValue.ScaleValue.updateAllowed = initalUpdateAllowed;
			break;

		case 1:
			// set initial warm up time
			mode = "warm up time";
			scaleValue.type = AdcValueType::ADC_WARM_UP_TIME;
			scaleValue.ScaleValue.warmUpTime = initialWarmUpTime;
			break;

		case 2:
			// set initial scale mode
			mode = "scale mode";
			scaleValue.type = AdcValueType::ADC_SCALE_MODE;
			scaleValue.ScaleValue.scaleMode = initialScaleMode;
			break;

		case 3:
			// set initial gFactor
			mode = "gFactor";
			scaleValue.type = AdcValueType::ADC_G_FACTOR;
			scaleValue.ScaleValue.gFactor = initalgFactor;
			break;

		case 4:
			// set initial zero point tracking
			mode = "zero point tracking";
			scaleValue.type = AdcValueType::ADC_ZERO_POINT_TRACKING;
			scaleValue.ScaleValue.mode = initalZeroPointTracking;
			break;

		case 5:
			// set initial zero setting interval
			mode = "zero setting interval";
			scaleValue.type = AdcValueType::ADC_ZERO_SETTING_INTERVAL;
			scaleValue.ScaleValue.zeroSettingInterval = initalZeroSettingInterval;
			break;

		case 6:
			// set initial automatic zero setting time
			mode = "automatic zero setting time";
			scaleValue.type = AdcValueType::ADC_AUTOMATIC_ZERO_SETTING_TIME;
			scaleValue.ScaleValue.automaticZeroSettingTime = initialAutomaticZeroSettingTime;
			break;

		case 7:
			// set initial state automatic tilt sensor
			mode = "automatic tilt sensor";
			scaleValue.type = AdcValueType::ADC_AUTOMATIC_TILT_SENSOR;
			scaleValue.ScaleValue.state = initialStateAutomaticTiltSensor;
			break;
		}

		errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
		if (PrintErrorCode(errorCode, 0))
		{
			printf("error set initial %s\n", mode.c_str());
			return;
		}

	}


	while (1)
	{
		// parameter update allowed
		short updateAllowed;
		scaleValue.type = AdcValueType::ADC_UPDATE_ALLOWED;
		for (int idx = 0; idx < 4; idx++)
		{
			if ((idx == 0) || (idx == 3))
				updateAllowed = initalUpdateAllowed;
			else if ((idx == 1) || (idx == 2)) 
				updateAllowed = 0;

			switch (idx)
			{
			case 0:
			case 2:
				errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error get param update allowed\n", idx);
					return;
				}
				// check parameter update allowed
				if (scaleValue.ScaleValue.updateAllowed != updateAllowed)
				{
					printf("%d: error get param update allowed: 0x%x  expected %x\n", idx, scaleValue.ScaleValue.updateAllowed, updateAllowed);
					return;
				}
				break;

			case 1:
			case 3:
				scaleValue.ScaleValue.updateAllowed = updateAllowed;
				errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error set param update allowed\n", idx);
					return;
				}
				break;
			}
		}

		// read weight 1
		errorCode = MakeReadWeightTest(10);
		if (PrintErrorCode(errorCode, 0))
		{
			printf("error read weight 1\n");
			break;
		}

		// remaining warm up time
		long warmUpTime;
		scaleValue.type = AdcValueType::ADC_WARM_UP_TIME;
		for (int idx = 0; idx < 4; idx++)
		{
			if ((idx == 0) || (idx == 3))
				warmUpTime = initialWarmUpTime;
			else if ((idx == 1) || (idx == 2)) 
				warmUpTime = 0x5555;

			switch (idx)
			{
			case 0:
			case 2:
				errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error get warm up time\n", idx);
					return;
				}
				// check remaining warm up time
				if (scaleValue.ScaleValue.warmUpTime != warmUpTime)
				{
					printf("%d: error get warm up time: 0x%lx  expected %lx\n", idx, scaleValue.ScaleValue.warmUpTime, warmUpTime);
					return;
				}
				break;

			case 1:
			case 3:
				scaleValue.ScaleValue.warmUpTime = warmUpTime;
				errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error set new warm up time\n", idx);
					return;
				}
				break;
			}
		}


		// parameter update allowed
		bool stateAutomaticTiltSensor;
		scaleValue.type = AdcValueType::ADC_AUTOMATIC_TILT_SENSOR;
		for (int idx = 0; idx < 4; idx++)
		{
			if ((idx == 0) || (idx == 3))
				stateAutomaticTiltSensor = initialStateAutomaticTiltSensor;
			else if ((idx == 1) || (idx == 2))
				stateAutomaticTiltSensor = !initialStateAutomaticTiltSensor;

			switch (idx)
			{
			case 0:
			case 2:
				errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error get state automatic tilt sensor\n", idx);
					return;
				}
				// check state automatic tilt sensor
				if (scaleValue.ScaleValue.state != stateAutomaticTiltSensor)
				{
					printf("%d: error get state automatic tilt sensor: 0x%x  expected %x\n", idx, scaleValue.ScaleValue.state, stateAutomaticTiltSensor);
					return;
				}
				break;

			case 1:
			case 3:
				scaleValue.ScaleValue.state = stateAutomaticTiltSensor;
				errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error set state automatic tilt sensor\n", idx);
					return;
				}
				break;
			}
		}


		// read weight 2
		errorCode = MakeReadWeightTest(10);
		if (PrintErrorCode(errorCode, 0))
		{
			printf("error read weight 2\n");
			break;
		}

		// scale Mode
		AdcScaleMode scaleMode;
		scaleValue.type = AdcValueType::ADC_SCALE_MODE;
		for (int idx = 0; idx < 4; idx++)
		{
			if ((idx == 0) || (idx == 3))
				scaleMode = initialScaleMode;
			else if ((idx == 1) || (idx == 2))
				scaleMode = AdcScaleMode::ADC_SCALE_ZG_MODE;;

			switch (idx)
			{
			case 0:
			case 2:
				errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error get scale Mode\n", idx);
					return;
				}
				// check scale Mode
				if (scaleValue.ScaleValue.scaleMode != scaleMode)
				{
					printf("%d: error get scale Mode: 0x%x  expected %x\n", idx, scaleValue.ScaleValue.scaleMode, scaleMode);
					return;
				}
				break;

			case 1:
			case 3:
				scaleValue.ScaleValue.scaleMode = scaleMode;
				errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error set scale Mode\n", idx);
					return;
				}
				break;
			}
		}

		// read weight 3
		errorCode = MakeReadWeightTest(10);
		if (PrintErrorCode(errorCode, 0))
		{
			printf("error read weight 3\n");
			break;
		}

		// g factor
		short gFactor;
		scaleValue.type = AdcValueType::ADC_G_FACTOR;
		for (int idx = 0; idx < 4; idx++)
		{
			if ((idx == 0) || (idx == 3))
				gFactor = initalgFactor;
			else if ((idx == 1) || (idx == 2))
				gFactor = 200;

			switch (idx)
			{
			case 0:
			case 2:
				errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error get g factor\n", idx);
					return;
				}
				// check g factor
				if (scaleValue.ScaleValue.gFactor != gFactor)
				{
					printf("%d: error get g factor: 0x%x  expected %x\n", idx, scaleValue.ScaleValue.gFactor, gFactor);
					return;
				}
				break;

			case 1:
			case 3:
				scaleValue.ScaleValue.gFactor = gFactor;
				errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error set g factor\n", idx);
					return;
				}
				break;
			}
		}

		// read weight 4
		errorCode = MakeReadWeightTest(10);
		if (PrintErrorCode(errorCode, 0))
		{
			printf("error read weight 4\n");
			break;
		}

		// zero point tracking
		short zeroPointTracking;
		scaleValue.type = AdcValueType::ADC_ZERO_POINT_TRACKING;
		for (int idx = 0; idx < 4; idx++)
		{
			if ((idx == 0) || (idx == 3))
				zeroPointTracking = initalZeroPointTracking;
			else if ((idx == 1) || (idx == 2))
				zeroPointTracking = 0;

			switch (idx)
			{
			case 0:
			case 2:
				errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error get zero point tracking\n", idx);
					return;
				}
				// check zero point tracking
				if (scaleValue.ScaleValue.mode != zeroPointTracking)
				{
					printf("%d: error get zero point tracking: 0x%x  expected %x\n", idx, scaleValue.ScaleValue.mode, zeroPointTracking);
					return;
				}
				break;

			case 1:
			case 3:
				scaleValue.ScaleValue.mode = zeroPointTracking;
				errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error set g factor\n", idx);
					return;
				}
				break;
			}
		}

		// read weight 5
		errorCode = MakeReadWeightTest(10);
		if (PrintErrorCode(errorCode, 0))
		{
			printf("error read weight 5\n");
			break;
		}

		// zero setting interval
		long zeroSettingInterval;
		scaleValue.type = AdcValueType::ADC_ZERO_SETTING_INTERVAL;
		for (int idx = 0; idx < 4; idx++)
		{
			if ((idx == 0) || (idx == 3))
				zeroSettingInterval = initalZeroSettingInterval;
			else if ((idx == 1) || (idx == 2))
				zeroSettingInterval = 0;

			switch (idx)
			{
			case 0:
			case 2:
				errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error get zero setting interval\n", idx);
					return;
				}
				// check zero setting interval
				if (scaleValue.ScaleValue.zeroSettingInterval != zeroSettingInterval)
				{
					printf("%d: error get zero setting interval: 0x%lx  expected %lx\n", idx, scaleValue.ScaleValue.zeroSettingInterval, zeroSettingInterval);
					return;
				}
				break;

			case 1:
			case 3:
				scaleValue.ScaleValue.zeroSettingInterval = zeroSettingInterval;
				errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error set g factor\n", idx);
					return;
				}
				break;
			}
		}

		// read weight 6
		errorCode = MakeReadWeightTest(10);
		if (PrintErrorCode(errorCode, 0))
		{
			printf("error read weight 6\n");
			break;
		}

		// automatic zero setting time
		long automaticZeroSettingTime;
		scaleValue.type = AdcValueType::ADC_AUTOMATIC_ZERO_SETTING_TIME;
		for (int idx = 0; idx < 4; idx++)
		{
			if ((idx == 0) || (idx == 3))
				automaticZeroSettingTime = initialAutomaticZeroSettingTime;
			else if ((idx == 1) || (idx == 2))
				automaticZeroSettingTime = 0;

			switch (idx)
			{
			case 0:
			case 2:
				errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error get automatic zero setting time\n", idx);
					return;
				}
				// check time
				if (scaleValue.ScaleValue.automaticZeroSettingTime != automaticZeroSettingTime)
				{
					printf("%d: error get automatic zero setting time: 0x%lx  expected %lx\n", idx, scaleValue.ScaleValue.automaticZeroSettingTime, automaticZeroSettingTime);
					return;
				}
				break;

			case 1:
			case 3:
				scaleValue.ScaleValue.automaticZeroSettingTime = automaticZeroSettingTime;
				errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);
				if (PrintErrorCode(errorCode, 0))
				{
					printf("%d: error set automatic zero setting time\n", idx);
					return;
				}
				break;
			}
		}

		// read weight 7
		errorCode = MakeReadWeightTest(10);
		if (PrintErrorCode(errorCode, 0))
		{
			printf("error read weight 7\n");
			break;
		}

		// check for cancel
		if (_kbhit())
		{
			break;
		}
	}
}


/**
******************************************************************************
* MakeReadWeightTest - make read weight
*
* @param
* @return
* @remarks
******************************************************************************
*/
void MakeRoundingTest()
{
	short			errorCode = ADC_SUCCESS;
	short			registrationRequest = 0;
	AdcState		adcState;
	AdcWeight		weight;
	AdcWeight		weightHighResolution;
	long			digitValue, saveDigitValue = 0;
	int				state = 0;
	bool			testFinished = false;
	long long		e;
	long long		calcWeight;
	string			weightString, weightHsString;

	printf("Make sure that the load capacity is max = 3/6/15kg e=1/2/5g\n");
	GetCh();
	printf("Press key to cancel test\n");

	while (1)
	{
		errorCode = AdcGetHighResolution(g_adcHandle, registrationRequest, &adcState, &weight, &weightHighResolution, NULL, &digitValue);
		if (_kbhit() ||PrintErrorCode(errorCode, 0))
		{
			break;
		}

		switch (state)
		{
		case 0:
			// first time save digit value;
			saveDigitValue = digitValue;
			state = 1;
			break;

		case 1:
			// wait for digitValue is different saveDigitValue
			if (digitValue != saveDigitValue) state = 2;
			break;

		case 2:
			// wait for digitValue is again equal saveDigitValue
			if (digitValue == saveDigitValue)
			{
				state = 3;
				testFinished = true;
			}
			break;

		default:
			break;
		}

		// determine weighing range
		if (abs(weightHighResolution.value) < 30000) e = 10;
		else if ((abs(weightHighResolution.value) >= 30000) && (abs(weightHighResolution.value) < 60000)) e = 20;
		else if (abs(weightHighResolution.value) >= 60000) e = 50;

		if (weightHighResolution.value >= 0)
			calcWeight = (((weightHighResolution.value + e / 2) / e) * e) / 10;
		else
			calcWeight = (((weightHighResolution.value - e / 2) / e) * e) / 10;

		if (calcWeight != weight.value)
		{
			// format weight
			FormatValue(weight.value, weight.decimalPlaces, &weightString);
			// format high resolution weight
			FormatValue(weightHighResolution.value, weightHighResolution.decimalPlaces, &weightHsString);

			// print weight
			cout << "rounding error\tWeight:  " << weightString << " " << GetWeightUnit(weight.weightUnit);

			// print high resolution weight
			cout << "\tHighResolution:  " << weightHsString << " " << GetWeightUnit(weightHighResolution.weightUnit);


			cout << "\tDigit Value:  " << digitValue << endl;
		}

		// check rounding

		if (testFinished == true)
		{
			printf("rounding test finished successfully\n");
			break;
		}
	}
}


/**
******************************************************************************
* DoMakeAuthentication - make adc authentication
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoMakeAuthentication(void)
{
	MakeAuthentication(true);
}

/**
******************************************************************************
* MakeAuthentication - make adc authentication
*
* @param
* @return
* @remarks
******************************************************************************
*/
short MakeAuthentication(bool withIdentityString, stringstream *IDstring, bool silent)
{
    short               errorCode;
    unsigned char       random[8];
    unsigned long       size = sizeof(random);
    AdcAuthentication   authentication;
    Authentication      auth(ADW_AUTH_CRC_START, ADW_AUTH_CRC_POLY, ADW_AUTH_CRC_MASK);
	time_t				rawtime;
	struct tm 			*timeinfo;
	char				timeString[80];

    errorCode = AdcGetRandomAuthentication(g_adcHandle, random, &size);
    if (!PrintErrorCode(errorCode, 0))
    {
        authentication.chksum = auth.CalcCrc(random, size);
        authentication.swIdentity = NULL;

		if (withIdentityString)
		{
			if (silent == false)
			{
				printf("\n\trandom:");
				for (unsigned long i = 0; i < size; i++)
					printf("  %02x", random[i]);
				printf("\n");
			}

			if (IDstring)
			{
				authentication.swIdentity = (char *)malloc(IDstring->str().size() + 1);
				if (authentication.swIdentity)
					strcpy(authentication.swIdentity, IDstring->str().c_str());
			}
			else
			{
				// get local time
				time(&rawtime);
				timeinfo = localtime(&rawtime);
				strftime(timeString, sizeof(timeString), "%y%m%d%H%M", timeinfo);

				stringstream    logBookEntry;
				logBookEntry << STX << "vid" << FS << "13" << GS << \
					"cid" << FS << "129" << GS << \
					"swvl" << FS << "001" << GS << \
					"swid" << FS << "8139" << GS << \
					"swv" << FS << VERSION_FOR_AUTHENTICATION << GS << \
					"date" << FS << timeString << ETX;
				// don't forget the terminating 0
				authentication.swIdentity = (char *)malloc(logBookEntry.str().size() + 1);
				if (authentication.swIdentity)
					strcpy(authentication.swIdentity, logBookEntry.str().c_str());

				if (silent == false) printf("\tchecksum: %04hX\n\n", authentication.chksum);
			}
			errorCode = AdcSetAuthentication(g_adcHandle, &authentication);
			if (silent == false)
				PrintErrorCode(errorCode, 1);
			else
				PrintErrorCode(errorCode, 0);
		}
		else
		{
			errorCode = AdcSetAuthentication(g_adcHandle, &authentication);
			PrintErrorCode(errorCode, 0);
		}
        

        // free memory
        if (authentication.swIdentity)
        {
            free(authentication.swIdentity);
        }
    }
	return errorCode;
}


/**
******************************************************************************
* DoGetHighResolution - get high resolution
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoGetHighResolution(void)
{
    short			errorCode;
	short			registrationRequest = 0;
    AdcState		adcState;
    AdcWeight		weight;
    AdcWeight		weightHighResolution;
	AdcTare			tare;
    long			digitValue;
    string			weightString;
	string			weightHsString;
	string			tareString;
    long long		saveWeightValue = 0;
	long long		saveWeightHighResolutionValue = 0;
	AdcTare			saveTare;
    AdcState		saveAdcState;
	unsigned long	ulSleep = 0;
	string			fileName;
	string			message;
	ofstream		logHandle;
	unsigned long	testStartTime;
	unsigned long	currentTime;

	ulSleep = (unsigned long)GetNumeric((char *)"Pollinverval [ms]: ");
	fileName = GetString((char *)"log filename: ", 255);

	if (fileName.empty() == false)
	{
		// create log file
		logHandle.open(fileName, ios::binary);

		// write header
		message = "time stamp [sec],weight,high resolution, C-value, state\r\n";
		// write to file
		logHandle.write(message.c_str(), message.size());
	}

    saveAdcState.state = 0;
	saveTare.type = ADC_TARE_NO;
	saveTare.value.value = 0;
	testStartTime = GetSystemTickCount();
    do
    {
        errorCode = AdcGetHighResolution(g_adcHandle, registrationRequest, &adcState, &weight, &weightHighResolution, &tare, &digitValue);
		currentTime = GetSystemTickCount();

		if (!PrintErrorCode(errorCode, 0))
		{
			// format weight
			FormatValue(weight.value, weight.decimalPlaces, &weightString);
			// format high resolution weight
			FormatValue(weightHighResolution.value, weightHighResolution.decimalPlaces, &weightHsString);

			if ((saveWeightValue != weight.value) ||
				(saveWeightHighResolutionValue != weightHighResolution.value) ||
				(saveTare.value.value != tare.value.value) || (saveTare.type != tare.type) ||
				(saveAdcState.state != adcState.state) )
			{
				saveWeightValue = weight.value;
				saveTare = tare;
				saveAdcState.state = adcState.state;
				saveWeightHighResolutionValue = weightHighResolution.value;

				char tempStr[255];
				sprintf(tempStr, "%.3f", ((float)(currentTime - testStartTime) / 1000));

				if (logHandle.is_open())
				{
					message = string(tempStr) + "," + weightString + "," + weightHsString + "," + to_string(digitValue) + ",";

					// print out adcState
					PrintAdcState(adcState, MODE_READ_WEIGHT, true, &message);

					message += "\r\n";

					// write to file
					logHandle.write(message.c_str(), message.size());
				}

				// print weight
				cout << tempStr << "\tWeight:  " << weightString << " " << GetWeightUnit(weight.weightUnit);

				// print high resolution weight
				cout << "\tHighResolution:  " << weightHsString << " " << GetWeightUnit(weightHighResolution.weightUnit);

				if ((tare.type != ADC_TARE_NO) && (tare.value.value != 0))
				{
					// print tare
					FormatValue(tare.value.value, tare.value.decimalPlaces, &tareString);
					cout << "\tTare:  " << tareString << " " << GetWeightUnit(tare.value.weightUnit);
				}

				cout << "\tDigit Value:  " << digitValue << "\t";

				// print out adcState
				PrintAdcState(adcState, MODE_READ_WEIGHT, true);

				cout << endl;
			}
		}
		else
		{
			// error, reset values
			saveAdcState.state = 0;
			saveWeightValue = 0;
			saveAdcState.state = 0;
			saveTare.type = ADC_TARE_NO;
			saveTare.value.value = 0;
		}
		Sleep(ulSleep);

    } while (!_kbhit());

	// empty keyboard buffer
	_getch();

	if (logHandle.is_open())
	{
		logHandle.close();
	}
}


/**
******************************************************************************
* DoGetGrossWeight - get gross weights from adc
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoGetGrossWeight(void)
{
	short			errorCode;
	AdcState		adcState;
	AdcWeight		grossWeight;
	AdcWeight		grossWeightHighResolution;
	long			digitValue;
	string			grossWeightString;
	string			grossWeightHsString;
	long long		saveGrossWeightValue = 0;
	long long		saveGrossWeightHighResolutionValue = 0;
	AdcState		saveAdcState;
	unsigned long	ulSleep = 0;
	string			fileName;
	string			message;
	ofstream		logHandle;
	unsigned long	testStartTime;
	unsigned long	currentTime;

	ulSleep = (unsigned long)GetNumeric((char *)"Pollinverval [ms]: ");
	fileName = GetString((char *)"log filename: ", 255);

	if (fileName.empty() == false)
	{
		// create log file
		logHandle.open(fileName, ios::binary);

		// write header
		message = "time stamp [sec],weight,high resolution, C-value, state\r\n";
		// write to file
		logHandle.write(message.c_str(), message.size());
	}

	saveAdcState.state = 0;
	testStartTime = GetSystemTickCount();
	do
	{
		errorCode = AdcGetGrossWeight(g_adcHandle, &adcState, &grossWeight, &grossWeightHighResolution, &digitValue);
		currentTime = GetSystemTickCount();

		if (!PrintErrorCode(errorCode, 0))
		{
			// format weight
			FormatValue(grossWeight.value, grossWeight.decimalPlaces, &grossWeightString);
			// format high resolution weight
			FormatValue(grossWeightHighResolution.value, grossWeightHighResolution.decimalPlaces, &grossWeightHsString);

			if ((saveGrossWeightValue != grossWeight.value) ||
				(saveGrossWeightHighResolutionValue != grossWeightHighResolution.value) ||
				(saveAdcState.state != adcState.state))
			{
				saveGrossWeightValue = grossWeight.value;
				saveAdcState.state = adcState.state;
				saveGrossWeightHighResolutionValue = grossWeightHighResolution.value;

				char tempStr[255];
				sprintf(tempStr, "%.3f", ((float)(currentTime - testStartTime) / 1000));

				if (logHandle.is_open())
				{
					message = string(tempStr) + "," + grossWeightString + "," + grossWeightHsString + "," + to_string(digitValue) + ",";

					// print out adcState
					PrintAdcState(adcState, MODE_READ_WEIGHT, true, &message);

					message += "\r\n";

					// write to file
					logHandle.write(message.c_str(), message.size());
				}

				// print gross weight
				cout << tempStr << "\tGW:  " << grossWeightString << " " << GetWeightUnit(grossWeight.weightUnit);

				// print high resolution gross weight
				cout << "\tGWH:  " << grossWeightHsString << " " << GetWeightUnit(grossWeightHighResolution.weightUnit);

				cout << "\tDigit Value:  " << digitValue << "\t";

				// print out adcState
				PrintAdcState(adcState, MODE_READ_WEIGHT, true);

				cout << endl;
			}
		}
		else
		{
			// error, reset values
			saveAdcState.state = 0;
			saveGrossWeightValue = 0;
			saveAdcState.state = 0;
		}
		Sleep(ulSleep);

	} while (!_kbhit());

	// empty keyboard buffer
	_getch();

	if (logHandle.is_open())
	{
		logHandle.close();
	}
}


/**
******************************************************************************
* DoDiagnosticData - get or configure diagnostic data from adc
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoDiagnosticData(void)
{
	short selection;

	printf("\tGet Sensor Health\t0\n");
	printf("\tConfigure Sensor Health\t1\n\n");

	selection = (short)GetNumeric((char *)"select: ");

	switch (selection)
	{
	case 0: GetDiagnosticData(); break;
	case 1: ConfigureDiagnosticData(); break;
	}
}

/**
******************************************************************************
* GetDiagnosticData - get diagnostic data from adc
*
* @param
* @return
* @remarks
******************************************************************************
*/
void GetDiagnosticData(void)
{
    short           errorCode;
    char            name[1024];
	AdcSensorHealth	sensorHealth;
    stringstream    diagnosticStream;
	long			id;
	short			state;
	string			weightString;

    /*diagnosticStream << STX << "diav" << GS << "diau" << ETX;
    strcpy(name, diagnosticStream.str().c_str());*/
	name[0] = '\0';

	//errorCode = AdcGetFirstDiagnosticData(g_adcHandle, NULL, &id, &sensorHealth, &state);	// test with NULL-Pointer
    errorCode = AdcGetFirstDiagnosticData(g_adcHandle, name, &id, &sensorHealth, &state);
    if (!PrintErrorCode(errorCode, 0))
    {
		while (state == 1)
		{
			switch (sensorHealth.type)
			{
			case 0:
				cout << sensorHealth.diaParam;
				for (int idx = 0; idx < sensorHealth.Health.type0.number; idx++)
				{
					cout << "\tvalue: " << sensorHealth.Health.type0.ValueUnit[idx].value << " " << GetDimensionUnit(sensorHealth.Health.type0.ValueUnit[idx].unit) << endl;
				}
				break;


			default:
				cout << sensorHealth.diaParam << "\tUnknown type: %d" << sensorHealth.type << endl;;
				break;
			}

			errorCode = AdcGetNextDiagnosticData(g_adcHandle, id, &sensorHealth, &state);
			PrintErrorCode(errorCode, 0);
		};
    }
}


/**
******************************************************************************
* ConfigureDiagnosticData - configure diagnostic data from adc
*
* @param
* @return
* @remarks
******************************************************************************
*/
void ConfigureDiagnosticData(void)
{
	short selection;
	short index;
	char *pStrPointer;
	int idx;
	CONFIG_DIA_PARAM *pDia;
	AdcSensorHealth sensorHealth;
	long value;
	long unit = 0;
	short errorCode;

	pDia = &configDiaParam[0];
	index = 0;
	while (pDia->diaParam != 0)
	{
		printf("\t%s\t%d\n", pDia->diaParam, index);
		pDia++;
		index++;
	}

	printf("\n");
	selection = (short)GetNumeric((char *)"select: ");

	if (selection < index)
	{
		idx = 1;
		pStrPointer = diaUnits[idx];
		if (pStrPointer) printf("\n\tUnit:");
		while (pStrPointer)
		{
			printf(" %s;", pStrPointer);
			idx++;
			pStrPointer = diaUnits[idx];
		}
		printf("\n\n");

		sensorHealth.type = configDiaParam[selection].type;
		strcpy(sensorHealth.diaParam, configDiaParam[selection].diaParam);

		for (int idx = 0; idx < configDiaParam[selection].number; idx++)
		{
			pStrPointer = (char *)GetString(configDiaParam[selection].valueStr[idx], 255);

			ParseInputValueUnit(pStrPointer, &value, &unit);
			if (sensorHealth.type == 100)
			{
				sensorHealth.Health.type100.ValueUnit[idx].value = value;
				sensorHealth.Health.type100.ValueUnit[idx].unit = unit;
				sensorHealth.Health.type100.number = idx + 1;
			}
		}

		errorCode = AdcConfigureDiagnosticData(g_adcHandle, &sensorHealth);
		PrintErrorCode(errorCode, 0);
	}
}

void ParseInputValueUnit(char *pStrPointer, long *value, long *unit)
{
	char *pPointer;
	bool space = false;
	char unitStr[255];
	long idx;

	pPointer = pStrPointer;
	// find space
	while (*pPointer)
	{
		if (*pPointer++ == 0x20)
		{
			space = true;
			break;
		}
	}

	if (space == true)
	{
		sscanf(pStrPointer, "%ld %s", value, unitStr);

		idx = 1;
		pPointer = diaUnits[idx];
		while (pPointer)
		{
			if (strcmp(pPointer, unitStr) == 0)
			{
				*unit = idx;
				break;
			}
			idx++;
			pPointer = diaUnits[idx];
		}
	}
	else
	{
		sscanf(pStrPointer, "%ld", value);
		*unit = 0;
	}
	return;
}



/**
******************************************************************************
* DoSetCountryFilesPath - set path to country files
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoSetCountryFilesPath(void)
{
    short   errorCode;

    char *pStrPointer = GetString((char *)"path", 255);
    errorCode = AdcSetCountryFilesPath(g_adcHandle, pStrPointer);
    PrintErrorCode(errorCode, 0);
}


/**
******************************************************************************
* DoCalibration - do calibration
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoCalibration(void)
{
	short   errorCode;
	short	selection;
	long	step, oldStep = 0;
	long	calibDigit;
	bool	waitSend = false;
	char	*pStrPointer = NULL;
	AdcCalibCmd cmd;
	AdcState	adcState;
	time_t		rawtime;
	struct tm 	*timeinfo;
	char	country[4];
	char	loadCapacity[128];
	unsigned char valueCap;
	char	*pWscStr = NULL;
	char	cScaleModel[25];
	char	cScaleModelReq[255];

	selection = (short)GetNumeric((char *)"Calibration with tilt compensation [0: no  1: yes]: ");

	if (selection == 0)
		cmd = AdcCalibCmd::ADC_CALIB_START;
	else
		cmd = AdcCalibCmd::ADC_CALIB_START_TILT_COMP;

	errorCode = AdcCalibration(g_adcHandle, cmd, &adcState, NULL, NULL);
	if (PrintErrorCode(errorCode, 0))
	{
		PrintAdcState(adcState);
		return;
	}
	else if (adcState.bit.calibMode == 0)
	{
		PrintAdcState(adcState);
		return;
	}

	// set country
	ShowSupportedCountries();
	pStrPointer = GetString((char *)"country", 3);
	if (strlen(pStrPointer) == 0)
	{
		AdcGetCountry(g_adcHandle, country, sizeof(country));
		printf("use current country ""%s"" for calibration\n", country);
		pStrPointer = country;
	}
	errorCode = AdcSetCountry(g_adcHandle, pStrPointer);
	if (PrintErrorCode(errorCode, 0))
	{
		return;
	}

	printf("\n");

	// set load capacity
	ShowCompatibleLoadCapacities(pStrPointer);
	pStrPointer = GetString((char *)"load cell", 7);
	if (strlen(pStrPointer) == 0)
	{
		AdcGetLoadCapacity(g_adcHandle, loadCapacity, sizeof(loadCapacity));
		printf("use current Load capacity ""%s"" for calibration\n", loadCapacity);
		pStrPointer = loadCapacity;
	}
	errorCode = AdcSetLoadCapacity(g_adcHandle, pStrPointer);
	if (PrintErrorCode(errorCode, 0))
	{
		return;
	}

	// ask for ssp file path
	string sspDir = GetString((char *)"Enter ssp directory: ", 255);
	PrintErrorCode(AdcSetSspPath(g_adcHandle, sspDir.c_str()), 0);

	// ask for scale model
	cScaleModel[0] = 0;
	PrintErrorCode(AdcGetScaleModel(g_adcHandle, cScaleModel, sizeof(cScaleModel)), 0);
	string scaleModel;
	if (strlen(cScaleModel))
	{
		sprintf(cScaleModelReq, "Enter scale model [Ist: %s]: ", cScaleModel);
		scaleModel = GetString(cScaleModelReq, 255);
		if (scaleModel.empty())
		{
			int delScaleModel = (short)GetNumeric((char *)"Do you want to delete the scale model [0: no  1: yes]: ");
			if (delScaleModel == 1)
				scaleModel.clear();
			else
				scaleModel = cScaleModel;
		}
	}
	else
	{
		sprintf(cScaleModelReq, "Enter scale model: ");
		scaleModel = GetString(cScaleModelReq, 255);
	}
	PrintErrorCode(AdcSetScaleModel(g_adcHandle, scaleModel.c_str()), 0);

	// set scale specific parameter
	AdcScaleValues  scaleValue;
	scaleValue.type = AdcValueType::ADC_SCALE_SPECIFIC_PARAM_SEALED;
	PrintErrorCode(AdcSetScaleValues(g_adcHandle, &scaleValue), 0);

	while (1)
	{
		cmd = AdcCalibCmd::ADC_CALIB_GET_STATE;
		errorCode = AdcCalibration(g_adcHandle, cmd, &adcState, &step, &calibDigit);
		if (PrintErrorCode(errorCode, 0))
		{
			PrintAdcState(adcState);
			break;
		}

		if (step != 3)
		{
			if (oldStep == 3) printf("\n");
			waitSend = false;
		}

		switch (step)
		{
			case 0:	// unload scale
				selection = (short)GetNumeric((char *)"unload scale [0: yes  1: no   2: cancel]: ");
				break;

			case 1: // load scale with max weight / 2
				selection = (short)GetNumeric((char *)"load scale with half max weight / 2 [0: yes  1: no   2: cancel]: ");
				break;

			case 2: // load scale with max weight
				selection = (short)GetNumeric((char *)"load scale with max weight [0: yes  1: no   2: cancel]: ");
				break;

			case 3: // adc busy
				if (waitSend == false)
				{
					printf("\tadc busy");
					waitSend = true;
				}
				else
				{
					printf(".");
				}
				selection = 1;
				break;

			case 4: // calibration not started
				printf("calibration not started, cancel\n");
				selection = 2;
				break;

			case 10: // unload scale and tilt to the left
				selection = (short)GetNumeric((char *)"tilt to the left and unload scale and [0: yes  1: no   2: cancel]: ");
				break;

			case 12: // load scale with max weight and tilt to the left
				selection = (short)GetNumeric((char *)"tilt to the left and load scale with max weight [0: yes  1: no   2: cancel]: ");
				break;

			case 20: // unload scale and tilt to the rigth
				selection = (short)GetNumeric((char *)"tilt to the rigth and unload scale [0: yes  1: no   2: cancel]: ");
				break;

			case 22: // load scale with max weight and tilt to the right
				selection = (short)GetNumeric((char *)"tilt to the right and load scale with max weight [0: yes  1: no   2: cancel]: ");
				break;

			case 30: // unload scale and tilt backwards
				selection = (short)GetNumeric((char *)"tilt backwards and unload scale [0: yes  1: no   2: cancel]: ");
				break;

			case 32: // load scale with max weight and tilt backwards
				selection = (short)GetNumeric((char *)"tilt backwards and load scale with max weight [0: yes  1: no   2: cancel]: ");
				break;

			case 40: // unload scale and tilt forward
				selection = (short)GetNumeric((char *)"tilt forward and unload scale [0: yes  1: no   2: cancel]: ");
				break;

			case 42: // load scale with max weight and tilt forward
				selection = (short)GetNumeric((char *)"tilt forward and load scale with max weight [0: yes  1: no   2: cancel]: ");
				break;

			case 50: // align spirit level
				selection = (short)GetNumeric((char *)"align spirit level [0: yes  1: no   2: cancel]: ");
				break;

			case 1000: // calib finish
				selection = 1000;
				break;

			default: // unknown step
				selection = (short)GetNumeric((char *)"unkown step [0: yes  1: no   2: cancel]: ");
				break;
		}

		if (selection == 0)
		{
			// confirm step
			cmd = AdcCalibCmd::ADC_CALIB_STEP_CONFIRM;
			errorCode = AdcCalibration(g_adcHandle, cmd, &adcState, NULL, NULL);
			if (PrintErrorCode(errorCode, 0))
			{
				PrintAdcState(adcState);
			}
		}
		else if (selection == 2)
		{
			// cancel
			cmd = AdcCalibCmd::ADC_CALIB_CANCEL;
			errorCode = AdcCalibration(g_adcHandle, cmd, &adcState, NULL, NULL);
			if (PrintErrorCode(errorCode, 0))
			{
				PrintAdcState(adcState);
			}
			break;
		}
		else if (selection == 1000)
		{
			AdcScaleValues scaleValue;
			short gFactorOffset = 0;

			// set production site
			printf("Production site local: \t0\n");
			printf("Production site Balingen: \t1\n");
			printf("Production site WeighTec: \t2\n");
			short site = (short)GetNumeric((char *)"site: ");

			if (site)
			{
				gFactorOffset = (short)GetNumeric((char *)"gFactor offset: ");
			}

			errorCode = AdcGetCapability(g_adcHandle, ADC_CAP_SEPARATE_LOADCELL, &valueCap);
			if (!PrintErrorCode(errorCode, 0) && valueCap)
			{
				// adc and load cell are separated, so ask for wsc string
				pWscStr = GetString((char *)"wsc string", 21);
			}

			scaleValue.type = AdcValueType::ADC_PRODUCTION_SETTINGS;
			scaleValue.frozen = 0;
			scaleValue.ScaleValue.prodSettings.size = sizeof (AdcProdSettings);
			scaleValue.ScaleValue.prodSettings.prodSiteID = site;
			scaleValue.ScaleValue.prodSettings.gFactorOffset = gFactorOffset;
			// get local time
			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(scaleValue.ScaleValue.prodSettings.date, sizeof(scaleValue.ScaleValue.prodSettings.date), "%y%m%d", timeinfo);
			// set wsc string
			if (pWscStr && (strlen(pWscStr) < sizeof(scaleValue.ScaleValue.prodSettings.wsc) - 1))
			{
				strcpy(scaleValue.ScaleValue.prodSettings.wsc, pWscStr);
			}
			else
			{
				scaleValue.ScaleValue.prodSettings.wsc[0] = 0;
			}
			PrintErrorCode(AdcSetScaleValues(g_adcHandle, &scaleValue), 0);

			// set g factor as last step
			short gFactor = (short)GetNumeric((char *)"gFactor: ");
			if (gFactor != 0)
			{
				scaleValue.type = AdcValueType::ADC_G_FACTOR;
				scaleValue.frozen = 0;
				scaleValue.ScaleValue.gFactor = gFactor;
				PrintErrorCode(AdcSetScaleValues(g_adcHandle, &scaleValue), 0);
			}

			printf("calib finish\n");
			break;
		}

		oldStep = step;
	}
}



/**
******************************************************************************
* DoGetInternalData - get internal adc data
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoGetInternalData(void)
{
	short			errorCode;
	unsigned long	ulSleep = 0;
	string			idStr;
	stringstream	message;
	string			fileName;
	ofstream		logHandle;
	AdcInternalDataEx* internalData;
	long			count = 0, count2 = 0;
	long			virbrationCompensation = 0;
	bool			bFirstRun;
	bool			bSendRequestCmd;
	bool			bWriteToLogFile = false;


	idStr = GetString((char *)"Enter ID [comma separated]: ", 255);
	if (idStr.empty() == true)
	{
		return;
	}

	ulSleep = (unsigned long)GetNumeric((char *)"Pollinverval [ms]: ");

	fileName = GetString((char *)"log filename: ", 255);
	if (fileName.empty() == false)
	{
		// create log file
		logHandle.open(fileName, ios::binary);
		if (logHandle.is_open()) bWriteToLogFile = true;
	}

	if (bWriteToLogFile)
	{
		virbrationCompensation = GetNumeric((char *)"vibration compensation [0: no    1:yes]: ");
	}

	// count ','
	for (unsigned int idx = 0; idx < idStr.length(); idx++)
	{
		if (idStr[idx] == ',') count++;
	}

	count += 2;		// min one value plus end identifier

	// allocate structure
	internalData = new AdcInternalDataEx[count];

	if (internalData == NULL)
	{
		printf("Error: can't allocate memory\n");
		return;
	}

	// copy raw ids to structure
	count2 = 0;
	for (unsigned int idx = 0; idx < idStr.length(); idx++)
	{
		if (idStr[idx] != ',')
		{
			if (count2 < count - 1) internalData[count2++].ID = idStr[idx];
		}
	}
	internalData[count2].ID = 0;		// end identifier

	bSendRequestCmd = true;
	do
	{
		errorCode = AdcGetInternalDataEx(g_adcHandle, internalData, bSendRequestCmd);

		if (PrintErrorCode(errorCode, 0)) break;

		if (bWriteToLogFile)
		{
			message.str(std::string());
			bFirstRun = true;
			count = 0;
			while (internalData[count].ID && internalData[count].valid)
			{
				if (bFirstRun == false)
					message << "\t";
				message << internalData[count].ID << internalData[count].value;

				bFirstRun = false;
				count++;;
			}

			message << "\r\n";

			// write to file
			logHandle.write(message.str().c_str(), message.str().size());

			if (ulSleep) Sleep(ulSleep);
		}
		else
		{
			// out structure
			count = 0;
			while (internalData[count].ID && internalData[count].valid)
			{
				cout << "\t" << internalData[count].ID << "\t" << internalData[count].value;
				count++;

				if ((count % 3) == 0) cout << endl;
			}

			if ((count % 3) != 0) cout << endl;

			Sleep(ulSleep);
		}

		if (virbrationCompensation) 
			bSendRequestCmd = false;

	} while (!_kbhit());

	if (errorCode == ADC_SUCCESS)
	{
		// stop internal data output
		if (virbrationCompensation) 
			AdcGetInternalDataEx(g_adcHandle, internalData, true);

		// empty keyboard buffer
		_getch();
	}

	delete[] internalData;

	if (logHandle.is_open())
	{
		logHandle.close();
	}

	return;
}


/**
******************************************************************************
* DoUpdate - make a firmware update
*
* @param
* @return
* @remarks
******************************************************************************
*/
void DoUpdate(void)
{
	short	errorCode;
	string	dir;
	short	force;

	dir = GetString((char *)"Enter firmware directory: ", 255);

	force = (short)GetNumeric((char *)"Force [0: no   1: yes]: ");

	errorCode = AdcSetFirmwarePath(g_adcHandle, dir.c_str());
	if (!PrintErrorCode(errorCode, 0))
	{
		// do Update
		Update(force);
	}
}

/**
******************************************************************************
* Update - make a firmware update
*
* @param
* @return
* @remarks
******************************************************************************
*/
short Update(const short force)
{
	short	errorCode;

	// set operating mode to bootloader
	errorCode = SetOperatingMode(ADC_BOOTLOADER);
	if (!PrintErrorCode(errorCode, 0))
	{
		// make authentication
		errorCode = MakeAuthentication(false);
		if (!PrintErrorCode(errorCode, 0))
		{
			// update adc
			errorCode = AdcUpdate(g_adcHandle, force);
			PrintErrorCode(errorCode, 0);
		}
	}

	PrintErrorCode(SetOperatingMode(ADC_APPLICATION), 0);

	return errorCode;
}


/**
******************************************************************************
* SetOperatingMode - set adc operating mode
*
* @param        mode:in		operating mode
* @return
* @remarks
******************************************************************************
*/
short SetOperatingMode(AdcOperatingMode mode)
{
	short errorCode = ADC_SUCCESS;
	AdcScaleValues  scaleValue;

	// set operating mode
	scaleValue.type = ADC_OPERATING_MODE;
	scaleValue.ScaleValue.opMode = mode;
	errorCode = AdcSetScaleValues(g_adcHandle, &scaleValue);

	return errorCode;
}


/**
******************************************************************************
* GetOperatingMode - get adc operating mode
*
* @param        mode:out		operating mode
* @return
* @remarks
******************************************************************************
*/
short GetOperatingMode(AdcOperatingMode *mode)
{
	short errorCode = ADC_SUCCESS;
	AdcScaleValues  scaleValue;

	// set operating mode
	scaleValue.type = ADC_OPERATING_MODE;
	errorCode = AdcGetScaleValues(g_adcHandle, &scaleValue);
	*mode = scaleValue.ScaleValue.opMode;

	return errorCode;
}




/**
******************************************************************************
* PrintAdcState - print adc state in a readable form
*
* @param        adcState:in
* @return
* @remarks
******************************************************************************
*/
void PrintAdcState(AdcState adcState, int mode, bool autoAuthentication, string *adcStateOut)
{
	bool swIdentify = false;
	string adcStateStr;

	switch (mode)
	{
	case MODE_READ_WEIGHT:
		if (adcState.bit.underZero) adcStateStr += " underload /";
		if (adcState.bit.underWeight) adcStateStr += " weight within minimum load range /";
		if (adcState.bit.overWeight) adcStateStr += " overload /";
		if (adcState.bit.sameWeight) adcStateStr += " no motion since last weighing /";
		if (adcState.bit.tareConst) adcStateStr += " tare const /";
		if (adcState.bit.busy) adcStateStr += " busy /";
		if ((adcState.bit.needAuthentication) || ((adcState.bit.needLogbook)))
		{
			if (adcState.bit.needAuthentication) adcStateStr += " requests an authentication /";
			if (adcState.bit.needLogbook)
			{
				adcStateStr += " requests an authentication with logbook /";
				swIdentify = true;
			}
			if (autoAuthentication)
			{
				MakeAuthentication(swIdentify);
			}
		}
		if (adcState.bit.weightUnstable) adcStateStr += " weight unstable /";
		if (adcState.bit.scaleNotReady == 1) adcStateStr += " scale not ready /";
		if (adcState.bit.scaleCalError == 1) adcStateStr += " calib params corrupted /";
		if (adcState.bit.tiltCompOutsideLimit) adcStateStr += " tilt too large /";
		if (adcState.bit.weightRegistered == 1) adcStateStr += " weight registered /";
		if (adcState.bit.calibState == 1) adcStateStr += " pre-calibrated / ";
		if (adcState.bit.zeroPointCorrPerformed == 1) adcStateStr += " zero point correction performed /";
		if (adcState.bit.zeroIndicator == 1) adcStateStr += " zero indicator / ";
		if (adcState.bit.warmUpTime == 1) adcStateStr += " warm up time not expired / ";
		if (adcState.bit.zeroPointCorrRequest == 1) adcStateStr += " zero point correction request / ";
		if (adcState.bit.outsideZeroRange == 1) adcStateStr += " switch on scale is outside zero range";
		if (adcState.bit.outsideZeroTrackingRange == 1) adcStateStr += " automatic zero tracking impossible";

		if (adcStateOut)
		{
			*adcStateOut += adcStateStr;
		}
		else
		{
			printf("%s", adcStateStr.c_str());
		}
		break;

	default:
		ostringstream convert;
		convert << std::hex << adcState.state;
		if (adcState.state) adcStateStr = "\n\tstate: 0x" + convert.str() + "\n";
		if (adcState.bit.underZero) adcStateStr += "\t\tscale in underload\n";
		if (adcState.bit.underWeight) adcStateStr += "\t\tweight within minimum load range\n";
		if (adcState.bit.overWeight) adcStateStr += "\t\tscale in overload\n";
		if (adcState.bit.sameWeight) adcStateStr += "\t\tno motion since last weighing\n";
		if (adcState.bit.busy) adcStateStr += "\t\tbusy\n";
		if (adcState.bit.needAuthentication) 
		{
			if (adcState.bit.needAuthentication) adcStateStr += "\t\tadc requests an authentication\n";
			if (adcState.bit.needLogbook)
			{
				adcStateStr += "\t\tadc requests an authentication with logbook\n";
				swIdentify = true;
			}
			if (autoAuthentication)
			{
				MakeAuthentication(swIdentify);
			}
		}
		if (adcState.bit.tiltCompOutsideLimit) adcStateStr += "\t\ttilt too large\n";
		if (adcState.bit.weightUnstable) adcStateStr += "\t\tweight unstable\n";
		if (adcState.bit.calibMode == 0) adcStateStr += "\t\twelmec mode\n";
		if (adcState.bit.calibMode == 1) adcStateStr += "\t\tno welmec mode or in calib mode\n";
		if (adcState.bit.scaleNotReady == 1) adcStateStr += "\t\tscale not ready\n";
		if (adcState.bit.scaleCalError == 1) adcStateStr += "\t\tcalib params corrupted\n";
		if (adcState.bit.weightRegistered == 1) adcStateStr += "\t\tweight registered\n";
		if (adcState.bit.calibState == 1) adcStateStr += "\t\tpre-calibrated\n";
		if (adcState.bit.zeroPointCorrPerformed == 1) adcStateStr += "\t\tzero point correction performed\n";
		if (adcState.bit.zeroIndicator == 1) adcStateStr += "\t\tzero indicator range\n";
		if (adcState.bit.warmUpTime == 1) adcStateStr += "\t\twarm up time not expired\n";
		if (adcState.bit.zeroPointCorrRequest == 1) adcStateStr += "\t\tzero point correction request\n";
		if (adcState.bit.outsideZeroRange == 1) adcStateStr += "\t\tswitch on scale is outside zero range\n";
		if (adcState.bit.outsideZeroTrackingRange == 1) adcStateStr += " automatic zero tracking impossible";

		printf("%s", adcStateStr.c_str());
	}
    return;
}


/**
******************************************************************************
* PrintErrorCode - print errorcode in a readable form
*
* @param        errorCode:in
*               printOK:in      0: print not ok message
*                               1: print ok message
* @return       0: ok       1: error
* @remarks
******************************************************************************
*/
short PrintErrorCode(short errorCode, short printOK)
{
    short retCode = 1;
    switch (errorCode)
    {
    case ADC_SUCCESS:
        if (printOK) printf("\noperation successful\n"); retCode = 0; break;
    case ADC_E_ADC_ALREADY_OPEN:
        printf("\nError: 0x%x: Adc already open\n", errorCode); break;
    case ADC_E_ADC_FAILURE:
        printf("\nError: 0x%x: Adc error\n", errorCode); break;
    case ADC_E_AUTHENTICATION:
		printf("\nError: 0x%x: Adc authentication error\n", errorCode); retCode = 0; break;
    case ADC_E_INVALID_HANDLE:
        printf("\nError: 0x%x: Adc invalid handle\n", errorCode); break;
    case ADC_E_INVALID_PARAMETER:
        printf("\nError: 0x%x: Adc invalid parameter\n", errorCode); break;
    case ADC_E_NO_DEVICE:
        printf("\nError: 0x%x: Adc not found\n", errorCode); break;
    case ADC_E_USB_ERROR:
        printf("\nError: 0x%x: Adc USB error\n", errorCode); break;
    case ADC_E_NOT_ENOUGH_MEMORY:
        printf("\nError: 0x%x: not enough memory\n", errorCode); break;
    case ADC_E_COUNTRY_NOT_SUPPORTED:
        printf("\nError: 0x%x: country not supported\n", errorCode); break;
    case ADC_E_FILE_CORRUPT:
        printf("\nError: 0x%x: file is corrupt\n", errorCode); break;
    case ADC_E_FILE_NOT_FOUND:
        printf("\nError: 0x%x: file not found or no access to file\n", errorCode); break;
	case ADC_E_COMMAND_NOT_EXECUTED:
		printf("\nError: 0x%x: command not executed\n", errorCode); break;
	case ADC_E_FUNCTION_NOT_IMPLEMENTED:
		printf("\nError: 0x%x: function not implemented\n", errorCode); break;
	case ADC_E_ADC_TIMEOUT:
		printf("\nError: 0x%x: adc timeout\n", errorCode); break;
	case ADC_E_SENSOR_HEALTH_QUEUE_FULL:
		printf("\nError: 0x%x: sensor health queue full\n", errorCode); break;
	case ADC_E_PROTOCOL_ERROR:
		printf("\nError: 0x%x: adc protocol error\n", errorCode); break;
	case ADC_E_PROTOCOL_CRC_ERROR:
		printf("\nError: 0x%x: adc protocol crc error\n", errorCode); break;
	case ADC_E_EEPROM_ACCESS_VIOLATION:
		printf("\nError: 0x%x: eeprom access violation\n", errorCode); break;
	case ADC_E_LOAD_CAPACITY_NOT_SUPPORTED:
		printf("\nError: 0x%x: load capacity not supported\n", errorCode); break;
	case ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE:
		printf("\nError: 0x%x: function not supported in this operating mode\n", errorCode); break;
	case ADC_E_LOGBOOK_NO_FURTHER_ENTRY:
		printf("\nError: 0x%x: no further logbook entry available\n", errorCode); break;
	case ADC_E_LOGBOOK_FULL:
		printf("\nError: 0x%x: logbook full\n", errorCode); break;
	case ADC_E_TILT_COMPENSATION_SWITCH_ON:
		printf("\nError: 0x%x: tilt compensation switch on, adc parameter wrong", errorCode);
		break;
	case ADC_E_CHKSUM_BLOCK_3_4:
		printf("\nError: 0x%x: wrong checksum eeprom block 3 / 4", errorCode);
		break;
	case ADC_E_CALIBRATION:
		printf("\nError: 0x%x: linear calibration or zero point detection", errorCode);
		break;
	case ADC_E_COMMAND_ONLY_FOR_SEPARATE_LOADCELL:
		printf("\nError: 0x%x: command supported only for separate load cells", errorCode);
		break;
	case ADC_E_UPDATE_NOT_ALLOWED:
		printf("\nError: 0x%x: software update not allowed", errorCode);
		break;
	case ADC_E_TCC_SWITCH_ON:
		printf("\nError: 0x%x: tilt compensation switch on, tcc parameter wrong", errorCode);
		break;
	case ADC_E_INCOMPATIBLE_PROD_DATA:
		printf("\nError: 0x%x: production data incompatible to firmware", errorCode);
		break;
	case ADC_E_TARE_OCCUPIED:
		printf("\nError: 0x%x: tare already occupied", errorCode);
		break;
	case ADC_E_KNOWN_TARE_LESS_E:
		printf("\nError: 0x%x: known tare less than e", errorCode);
		break;
	case ADC_E_BATCH_TARE_NOT_ALLOWED:
		printf("\nError: 0x%x: batch tare not allowed", errorCode);
		break;
	case ADC_E_TARE_OUT_OF_RANGE:
		printf("\nError: 0x%x: tare out of range", errorCode);
		break;
	case ADC_E_TARE_OUTSIDE_CLEARING_AREA:
		printf("\nError: 0x%x: tare outside clearing area", errorCode);
		break;
	case ADC_E_WS_NOT_SUPPORT_LOAD_CAPACITY:
		printf("\nError: 0x%x: weighing system does not support load capacity", errorCode);
		break;
	case ADC_E_CHKSUM_FIRMWARE:
		printf("\nError: 0x%x: wrong firmware checksum", errorCode);
		break;
    default:
        printf("\nError: 0x%x: UNKOWN\n", errorCode); break;
    }

    return retCode;
}


/**
******************************************************************************
* GetTareType - print tare type as string
*
* @param        tareType:in   enum
* @return       tare type as string
* @remarks
******************************************************************************
*/
char *GetTareType(AdcTareType tareType)
{
    switch (tareType)
    {
	case AdcTareType::ADC_TARE_WEIGHED: return (char *)"weighed"; break;
	case AdcTareType::ADC_TARE_KNOWN: return (char *)"known"; break;
	case AdcTareType::ADC_TARE_PERCENT: return (char *)"percent"; break;
    default: return (char *)"unknown"; break;
    }
}


/**
******************************************************************************
* GetWeightUnit - print weight unit as string
*
* @param        weightUnit:in   enum
* @return       weight unit as string
* @remarks
******************************************************************************
*/
char *GetWeightUnit(AdcWeightUnit weightUnit)
{
    switch (weightUnit)
    {
	case AdcWeightUnit::ADC_GRAM: return (char *)"g"; break;
	case AdcWeightUnit::ADC_KILOGRAM: return (char *)"kg"; break;
	case AdcWeightUnit::ADC_OUNCE: return (char *)"oz"; break;
	case AdcWeightUnit::ADC_POUND: return (char *)"lb"; break;
	case AdcWeightUnit::ADC_MILLIGRAM: return (char *)"mg"; break;
	case AdcWeightUnit::ADC_MICROGRAM: return (char *)"ug"; break;
    default: return (char *)"unknown"; break;
    }
}


/**
******************************************************************************
* GetDimensionUnit - print dimension unit as string
*
* @param        weightUnit:in   enum
* @return       weight unit as string
* @remarks
******************************************************************************
*/
char *GetDimensionUnit(long dimensionUnit)
{
	switch (dimensionUnit)
	{
	case 1: return diaUnits[1]; break;
	case 2: return diaUnits[2]; break;		// celcius
	case 3: return diaUnits[3]; break;
	case 4: return diaUnits[4]; break;
	case 5: return diaUnits[5]; break;
	case 6: return diaUnits[6]; break;
	case 7: return diaUnits[7]; break;
	case 8: return diaUnits[8]; break;
	case 9: return diaUnits[9]; break;
	case 10: return diaUnits[10]; break;			// promille
	case 11: return diaUnits[11]; break;
	case 12: return diaUnits[12]; break;
	case 13: return diaUnits[13]; break;
	default: return diaUnits[0]; break;
	}
}


/**
******************************************************************************
* GetScaleMode - print scale mode as string
*
* @param        scaleMode:in   enum
* @return       scale mode as string
* @remarks
******************************************************************************
*/
char *GetScaleMode(AdcScaleMode scaleMode)
{
    switch (scaleMode)
    {
	case AdcScaleMode::ADC_PURE_SCALE: return (char *)"pure scale"; break;
	case AdcScaleMode::ADC_SCALE_SB_MODE: return (char *)"sb mode"; break;
	case AdcScaleMode::ADC_SCALE_AS_MODE: return (char *)"as mode"; break;
	case AdcScaleMode::ADC_SCALE_PA_MODE: return (char *)"pa mode"; break;
	case AdcScaleMode::ADC_SCALE_ZG_MODE: return (char *)"zg mode"; break;
	case AdcScaleMode::ADC_SCALE_CW_MODE: return (char *)"cw mode"; break;
    default: return (char *)"unknown"; break;
    }
}

/**
******************************************************************************
* GetOpertingMode - print operating mode as string
*
* @param        opMode:in   enum
* @return       operating mode as string
* @remarks
******************************************************************************
*/
char *GetOperatingMode(AdcOperatingMode opMode)
{
	switch (opMode)
	{
	case AdcOperatingMode::ADC_BOOTLOADER: return (char *)"bootloader"; break;
	case AdcOperatingMode::ADC_APPLICATION: return (char *)"application"; break;
	default: return (char *)"unknown"; break;
	}
}



/**
******************************************************************************
* ShowSupportedCountries - print supported countries
*
* @param
* @return
* @remarks
******************************************************************************
*/
void ShowSupportedCountries()
{
	short errorCode;
	unsigned long size = 0;
	map<string, string> strStrMap;
	short index = 0;

	errorCode = AdcGetSupportedCountries(g_adcHandle, NULL, &size);
	if (!PrintErrorCode(errorCode, 0))
	{
		char *countries = new char[size];
		errorCode = AdcGetSupportedCountries(g_adcHandle, countries, &size);
		if (!PrintErrorCode(errorCode, 0))
		{
			printf("Supported countries:\n");

			if (size != 0)
			{
				string countriesStr = countries;
				if (GetKeyValuePair(countriesStr, strStrMap))
				{
					for (map<string, string>::iterator it = strStrMap.begin(); it != strStrMap.end(); ++it)
					{
						printf("\t%s", (*it).first.c_str());
						if ((++index % 7) == 0) printf("\n");
					}
					if ((index % 7) != 0) printf("\n");
				}
			}
		}
		// free memory
		if (countries != NULL)
			delete[] countries;
	}

	return;
}


/**
******************************************************************************
* ShowCompatibleLoadCapacities - print compatible load capacities
*
* @param        
* @return       
* @remarks
******************************************************************************
*/
void ShowCompatibleLoadCapacities(char *country)
{
    short				errorCode;
    unsigned long		size = 0;
    map<string, string> strStrMap;
	AdcCalStrings		calStrings;

	if (AdcGetCompatibleLoadCapacities(g_adcHandle, country, NULL, &size) == ADC_SUCCESS)
    {
        char *loadCapacities = new char[size];
		errorCode = AdcGetCompatibleLoadCapacities(g_adcHandle, country, loadCapacities, &size);
        if (!PrintErrorCode(errorCode, 0))
        {
            printf("Compatible load capacities:\n");

			if (size != 0)
			{
				string loadCapStr = loadCapacities;
				if (GetKeyValuePair(loadCapStr, strStrMap))
				{
					for (map<string, string>::iterator it = strStrMap.begin(); it != strStrMap.end(); ++it)
					{
						// get calibration string for the load capacity
						calStrings.eStr[0] = 0;
						calStrings.maxStr[0] = 0;
						calStrings.minStr[0] = 0;
						AdcGetCalStrings4LoadCapacity(g_adcHandle, (*it).first.c_str(), &calStrings);

						printf("\t%s\t%s\t%s\t%s\n", (*it).first.c_str(), calStrings.maxStr, calStrings.minStr, calStrings.eStr);
					}
				}
			}
        }
        // free memory
        if (loadCapacities != NULL)
            delete[] loadCapacities;
    }

    return;
}


/**
******************************************************************************
* FormatValue - format an value
*
* @param        
* @return       
* @remarks
******************************************************************************
*/
void FormatValue(long long value, short decimalPlaces, string* formatedString)
{
	char tempString[255];

	*formatedString = to_string(value);
	if (value >= 0)
	{
		if ((short)formatedString->length() <= decimalPlaces)
		{
			sprintf(tempString, "%0*lld", decimalPlaces + 1, value);
			*formatedString = tempString;
		}
	}
	else
	{
		if ((short)formatedString->length() <= decimalPlaces + 1)
		{
			sprintf(tempString, "%0*lld", decimalPlaces + 2, value);
			*formatedString = tempString;
		}
	}

	if (decimalPlaces)
		formatedString->insert(formatedString->end() - decimalPlaces, '.');
}

#ifdef __GNUC__
/*****************************************************************************/
/* Routine  : _kbhit                                                         */
/*                                                                           */
/* Arguments: -                                                              */
/*                                                                           */
/* Function : Checks the console for keyboard input.                         */
/*                                                                           */
/*            The _kbhit function checks the console for a recent keystroke. */
/*            If the function returns a nonzero value, a keystroke is        */
/*            waiting in the buffer. The program can then call _getch to get */
/*            the keystroke.                                                 */
/*                                                                           */
/* Return   : TRUE if a key was pressed, otherwise FALSE                     */
/*****************************************************************************/
static int _kbhit( void )
{
   char cCh;
   int iRead;

   if ( !sKbInit )
   {
      _kb_initialize(  );
      sKbInit = true;
   }

   if ( iPeek != -1 )
      return true;

   tNewKeybConf.c_cc[VMIN] = 0;
   tcsetattr( 0, TCSANOW, &tNewKeybConf );

   iRead = read( 0, &cCh, 1 );

   tNewKeybConf.c_cc[VMIN] = 1;
   tcsetattr( 0, TCSANOW, &tNewKeybConf );

   if ( iRead == 1 )
   {
      iPeek = cCh;
      return true;
   }

   return false;
}

/*****************************************************************************/
/* Routine  : _getch                                                         */
/*                                                                           */
/* Arguments: -                                                              */
/*                                                                           */
/* Function : Get a character from the console without echo                  */
/*                                                                           */
/* Return   : a single keyboard character                                    */
/*****************************************************************************/
static int _getch( void )
{
   char ch;

   if ( !sKbInit )
   {
      _kb_initialize(  );
      sKbInit = true;
   }

   if ( iPeek != -1 )
   {
      ch = iPeek;
      iPeek = -1;
      return ch;
   }

   read( 0, &ch, 1 );

   return ch;
}

/*****************************************************************************/
/* Routine  : _kb_initialize                                                 */
/*                                                                           */
/* Arguments: -                                                              */
/*                                                                           */
/* Function : Initialize the keyboard in cbreak mode.                        */
/*                                                                           */
/* Return   : -                                                              */
/*****************************************************************************/
static void _kb_initialize( void )
{
   tcgetattr( 0, &tOrigKeybConf );

   tNewKeybConf = tOrigKeybConf;
   tNewKeybConf.c_lflag &= ~ICANON;
   tNewKeybConf.c_lflag &= ~ECHO;
   tNewKeybConf.c_lflag &= ~ISIG;
   tNewKeybConf.c_iflag = 0;
   tNewKeybConf.c_cc[VMIN] = 1;
   tNewKeybConf.c_cc[VTIME] = 0;

   tcsetattr( 0, TCSANOW, &tNewKeybConf );
}

/*****************************************************************************/
/* Routine  : _kb_restore                                                    */
/*                                                                           */
/* Arguments: -                                                              */
/*                                                                           */
/* Function : restore the original keyboard mode.                            */
/*                                                                           */
/*            NOTE: if the process exits whitout restoring the original      */
/*                  keyboard mode, issue a ^J stty sane ^J command at the    */
/*                  command prompt.                                          */
/*                                                                           */
/* Return   : -                                                              */
/*****************************************************************************/
static void _kb_restore( void )
{
   if( sKbInit )
      tcsetattr( 0, TCSANOW, &tOrigKeybConf );
}

/*****************************************************************************/
/* Routine  : Sleep                                                          */
/*                                                                           */
/* Arguments: number of milliseconds to wait                                 */
/*                                                                           */
/* Function : The Sleep function suspends the execution of the current       */
/*            thread for a specified interval.                               */
/*                                                                           */
/*            Try to emulate the semantic of the Win32 Sleep Api.            */
/*                                                                           */
/*            If the parameter is 0, release the processor to another task.  */
/*            The time interval is multiplied by 1000, since nanosleep       */
/*            accept microseconds.                                           */
/*                                                                           */
/*            nanosleep is called until the complete time expiration, even   */
/*            if interrupted.                                                */
/*                                                                           */
/* Note     : the timing is not very precise, since is based on the kernel   */
/*            task switching timer (10ms on i386 arch). So the required      */
/*            timeout is at least 10ms and can be rounded to the next 10ms   */
/*            value (so, for example, a timeout of 10ms can be rounded to    */
/*            20ms).                                                         */
/*                                                                           */
/* Return   : -                                                              */
/*****************************************************************************/
static void Sleep( long dwMilliseconds )
{
   struct timespec req, rem;

   if ( dwMilliseconds )
   {
      rem.tv_sec = dwMilliseconds / 1000;
      rem.tv_nsec = ( dwMilliseconds % 1000 ) * 1000000;

      //
      // do sleep since the entire
      // required delay is expired
      // (even if interrupted)
      //
      do
      {
         errno = 0;
         // don't know why, but the
         // nanosleep call doesn't
         // reset errno !!!
         req = rem;
         nanosleep( &req, &rem );
      }
      while ( errno == EINTR );
   }
}
#endif


/**
******************************************************************************
* LocalVersionDisplay - show menu header
*
* @param
* @return
* @remarks
******************************************************************************
*/
void LocalVersionDisplay(VERSION_INFO * ptInfo)
{
	if (ptInfo != NULL)
	{
		// Display version info
		BANNER_R1(ptInfo);
		BANNER_R2(ptInfo);
		BANNER_R3(ptInfo);
	}
}

/**
******************************************************************************
* ShowMenu - show menu header
*
* @param
* @return
* @remarks
******************************************************************************
*/
void ShowMenu(void)
{
	PMENUTABLE pMt = &tMenu[0];

	char *div = (char *)"-----------------------------------------------------------\n";

	printf("\n%s", div);
	printf("|=             < Choose the Action, please >             =|\n");
	printf("%s", div);

	while (pMt->cChoice)
	{
		printf("%c) %s", pMt->cChoice, pMt->MenuDescr);
		pMt++;
		if (pMt->cChoice)
		{
			printf("\t%c) %s", pMt->cChoice, pMt->MenuDescr);
			pMt++;
		}
		printf("\n");
	}
	printf("%s", div);
	printf("  Select > ");
	fflush(stdout);
}



/*****************************************************************************/
/* Routine  : GetNumeric                                                     */
/*                                                                           */
/* Arguments:                                                                */
/*                                                                           */
/* Function :                                                                */
/*                                                                           */
/* Return   : numeric value                                                  */
/*****************************************************************************/
long GetNumeric(char *pszPrompt)
{
	long lNum = 0;
	int  sign = 0;
	unsigned char c;

	do
	{
		printf("  %s > %ld %c", pszPrompt, lNum, CURSORLEFT);

		if (!lNum)
			printf("%c", CURSORLEFT);

		fflush(stdout);

		c = GetCh();

		if (isdigit(c))
		{
			if (sign == 0) lNum = 10 * lNum + (c - '0');
			else lNum = 10 * lNum - (c - '0');
		}
		else if (ISBACKSPACEKEY(c))
			lNum /= 10;
		else if (ISSIGN(c))
		{
			lNum *= (-1);
			sign ^= 1;
		}

		printf("\r");

	} while (!ISENTERKEY(c));

	printf("\n\n");
	return(lNum);
}


/**
******************************************************************************
* GetString - get string as input
*
* @param
* @return		keyboard input
* @remarks
******************************************************************************
*/
char* GetString(char *pszPrompt, short sMaxLen)
{
	static char pszTmpStr[MAXINPUTSTRING];    // keep it static
	unsigned char iLen = 0;
	unsigned char c;

	if (sMaxLen >= MAXINPUTSTRING)
		sMaxLen = MAXINPUTSTRING - 1;

	do
	{
		pszTmpStr[iLen] = 0;
		printf("  %s > \"%s\" %c%c", pszPrompt, pszTmpStr,
			CURSORLEFT, CURSORLEFT);
		fflush(stdout);

		c = GetCh();

        if (isprint(c))
        {
            pszTmpStr[iLen++] = c;
            pszTmpStr[iLen] = '\0';
        }
		else
			if (ISBACKSPACEKEY(c) && iLen)
				iLen--;

		printf("\r");

	} while (!ISENTERKEY(c) && iLen < sMaxLen);

    printf("  %s > \"%s\" %c%c", pszPrompt, pszTmpStr,
        CURSORLEFT, CURSORLEFT);
    fflush(stdout);

	printf("\n");
	return(pszTmpStr);
}


/**
******************************************************************************
* GetMenuChoice - show menu header
*
* @param
* @return		keyboard input
* @remarks
******************************************************************************
*/
char GetCh(void)
{
	char cChar;

	while (!_kbhit())
		Sleep(1);

	cChar = (char)_getch();

	return(cChar);
}


/**
******************************************************************************
* GetMenuChoice - show menu header
*
* @param
* @return		pointer of menu entry
* @remarks
******************************************************************************
*/
PMENUTABLE GetMenuChoice(void)
{
	char cChoice;

	PMENUTABLE pMt;

	while (true)
	{
		cChoice = GetCh();

		pMt = &tMenu[0];
		while (pMt->cChoice && pMt->cChoice != cChoice)
			pMt++;

		if (pMt->cChoice)   // found !!
		{
			printf("%c\n\n", cChoice);
			break;
		}
	}
	return(pMt);
}


#ifdef _MSC_VER
int _tmain(int argc, _TCHAR* argv[])
{
	DWORD dwError;
	PMENUTABLE pMt;

	/* to dectect memory leaks */
	// now we use vld (visual leak detector) for detecting memory leaks
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	// set thread priority class to REALTIME_PRIORITY_CLASS (only if the application runs under Administration, otherwise HIGH_PRIORITY_CLASS
	if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
	{
		char errorBuf[255];
		dwError = GetLastError();

		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, errorBuf, sizeof(errorBuf), NULL);
		printf("Error set application priority: %d %s\n", dwError, errorBuf);
	}

	// set thread priority
	if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
	{
		char errorBuf[255];
		dwError = GetLastError();

		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, errorBuf, sizeof(errorBuf), NULL);
		printf("Error set application priority: %d %s\n", dwError, errorBuf);
	}
  
	// display program version info
	LocalVersionDisplay(&g_tVi);

  	do
	{
		ShowMenu();

		pMt = GetMenuChoice();

		if (pMt->pMenuFunction)
			pMt->pMenuFunction();
		else
			printf("Menu function not defined! (Choice %x)\n", pMt->cChoice);

	} while (!g_exit);

	return 0;
}
#endif	// _MSC_VER

#ifdef __GNUC__
/*****************************************************************************/
/* Routine  : main                                                           */
/*                                                                           */
/* Arguments: none!                                                          */
/*                                                                           */
/* Function : Main of test program                                           */
/*                                                                           */
/* Return   : none!                                                          */
/*****************************************************************************/
int main( void )
{
   PMENUTABLE pMt;

   atexit( _kb_restore );

   // display program version info
   LocalVersionDisplay( &g_tVi );

   do
   {
      ShowMenu();

      pMt = GetMenuChoice(  );

      if( pMt->pMenuFunction )
         pMt->pMenuFunction( );
      else
         printf( "Menu function not defined! (Choice %x)\n", pMt->cChoice );
   }
   while ( !g_exit );

   return( 0 );
}
#endif


unsigned long GetSystemTickCount()
{
#ifdef  _MSC_VER
	return timeGetTime();
#endif

#ifdef __GNUC__
	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now)) return 0;
	return now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0;
#endif
}
