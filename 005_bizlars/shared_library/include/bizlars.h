/**
******************************************************************************
* File       : bizlars.h
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : BizLars functions
******************************************************************************
*/

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

	// The following ifdef block is the standard way of creating macros which make exporting 
	// from a DLL simpler. All files within this DLL are compiled with the BZOEMPRN_EXPORTS
	// symbol defined on the command line. this symbol should not be defined on any project
	// that uses this DLL. This way any other project whose source files include this file see 
	// BZOEMPRN_API functions as being imported from a DLL, whereas this DLL sees symbols
	// defined with this macro as being exported.
	#ifdef  _MSC_VER

	#ifdef BIZLARS_EXPORTS
	#define BIZLARS_API __declspec(dllexport)
	#else
	#define BIZLARS_API __declspec(dllimport)
	#endif	// BIZLARS_EXPORTS

	#endif	// _MSC_VER

	#ifdef  __GNUC__
	#define BIZLARS_API
	#endif	// __GNUC__


	/**
	******************************************************************************
	* Errorcodes
	******************************************************************************
	*/

	#define ADC_SUCCESS					0			// sucess
	#define ADC_E_ADC_FAILURE			1			// adc error, for detailed error see adcState
	#define ADC_E_NO_DEVICE				2			// adc not connected 
	#define ADC_E_INVALID_HANDLE		3			// invalid handle
	#define ADC_E_USB_ERROR				4			// usb error
	#define ADC_E_INVALID_PARAMETER		5			// invalid input parameter
	#define ADC_E_AUTHENTICATION		6			// the application has not authenticated
	#define ADC_E_ADC_ALREADY_OPEN		7			// adc is already open
    #define ADC_E_NOT_ENOUGH_MEMORY     8           // not enough memory to take the action
    #define ADC_E_COUNTRY_NOT_SUPPORTED 9          // country not supported
    #define ADC_E_FILE_CORRUPT			10          // file is corrupt
    #define ADC_E_FILE_NOT_FOUND        11          // no access to file
	#define ADC_E_ADC_TIMEOUT			12			// adc timeout
    #define ADC_E_SENSOR_HEALTH_QUEUE_FULL		13	// sensor health queue full
	#define ADC_E_PROTOCOL_ERROR		14			// adc protocol error
	#define ADC_E_PROTOCOL_CRC_ERROR	15			// adc protocol crc error
	#define ADC_E_EEPROM_ACCESS_VIOLATION		16	// eeprom access violation
	#define ADC_E_LOAD_CAPACITY_NOT_SUPPORTED	17	// load capacity is not supported
	
	// detailed error from adc
	#define ADC_E_COMMAND_NOT_EXECUTED		100		// command not executed	
	#define ADC_E_FUNCTION_NOT_IMPLEMENTED	101		// function not implemented
	#define ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE	102	// function is not supported in this operating mode
	#define ADC_E_LOGBOOK_NO_FURTHER_ENTRY	103		// no further logbook entries
	#define ADC_E_LOGBOOK_FULL				104		// logbook full
	#define ADC_E_TILT_COMPENSATION_SWITCH_ON 105	// not possible to switch on tilt compensation because of wrong adc parameters
	#define ADC_E_CHKSUM_BLOCK_3_4			106		// wrong checksum eeprom block 3 / 4
	#define ADC_E_CALIBRATION				107		// error linear calibration or zero point detection
	#define ADC_E_COMMAND_ONLY_FOR_SEPARATE_LOADCELL	108		// error command supported only for separated load cells
	#define ADC_E_UPDATE_NOT_ALLOWED		109		// software update not allowed
	#define ADC_E_TCC_SWITCH_ON				110		// not possible to switch tilt compensation because of wrong tcc parameters
	#define ADC_E_INCOMPATIBLE_PROD_DATA	111		// production data incompatible to firmware
	#define ADC_E_TARE_OCCUPIED				112		// tare could not be set because tare is already occupied
	#define ADC_E_KNOWN_TARE_LESS_E			113		// tare could not be set because known tare rounded is less than e
	#define ADC_E_BATCH_TARE_NOT_ALLOWED	114		// batch tare not allowed
	#define ADC_E_TARE_OUT_OF_RANGE			115		// tare could not be set because greater limit or gross weight < 0
	#define ADC_E_TARE_OUTSIDE_CLEARING_AREA	116		// tare could not be cleared because tare is not identical to the negative weight value
	#define ADC_E_WS_NOT_SUPPORT_LOAD_CAPACITY	117		// weighing system does not support load capacity
	#define ADC_E_CHKSUM_FIRMWARE			118		// wrong firmware checksum

	// error codes vendor specific
	#define ADC_E_START_VENDOR_SPECIFIC_AREA	1000	// reserved area for vendor specific error codes
	#define ADC_E_END_VENDOR_SPECIFIC_AREA		1999
	
	/**
	******************************************************************************
	* Trace level
	******************************************************************************
	*/

	#define ADC_TRC_OFF					0
	#define ADC_TRC_ERROR_WARNING		1			
	#define ADC_TRC_ACTION				2			
	#define ADC_TRC_INFO				3			
	
	
	/**
	******************************************************************************
	* enums
	******************************************************************************
	*/

	typedef enum
	{
		ADC_CAP_DEVICE_AUTHENTICATION = 0,  // true: adc supports authentication
		ADC_CAP_DISPLAY_WEIGHT,				// true: weight display available
		ADC_CAP_DISPLAY_TARE,				// true: tare display available
		ADC_CAP_DISPLAY_BASEPRICE,			// true: base price display available
		ADC_CAP_DISPLAY_PRICE,				// true: price display available
		ADC_CAP_DISPLAY_TEXT,				// true: text display available
		ADC_CAP_PRICE_CALCULATION,			// true: function to calculate the price available
		ADC_CAP_TARE,						// true: tare function available
		ADC_CAP_ZERO_SCALE,					// true: zero function available
		ADC_CAP_TILT_COMPENSATION,			// true: tilt compensation available
        ADC_CAP_TARE_PRIORITY,              // true: adc supports tare priority
		ADC_CAP_TWO_CONVERTER,				// true: adc has two converters
		ADC_CAP_SEPARATE_LOADCELL,			// true: adc and load cell are separately
		ADC_CAP_FIRMWARE_UPDATE,			// true: adc supports firmware update
		ADC_CAP_HIGHRESOLUTION_WITH_TARE,	// true: adc supports high resolution with tare
		ADC_CAP_DIGITS_RESOLUTION_CONFIGURABLE,			// true: adc supports load capacity with configurable digits resolution
		ADC_CAP_TILT_COMPENSATION_CORNER_CORRECTION,	// true: adc supports tilt compensation with corner correction (tcc)
		ADC_CAP_WEIGHT_DEPENDENT_TILT_ANGLE,			// true: adc supports weight dependent tilt angle (wdta)
		ADC_CAP_AUTOMATIC_TILT_SENSOR		// true: adc supports an automatic tilt sensor
	} AdcCapabilities;

	typedef enum
	{
		ADC_KILOGRAM = 0,
		ADC_POUND,
		ADC_OUNCE,
		ADC_GRAM,
		ADC_MILLIGRAM,
		ADC_MICROGRAM
	} AdcWeightUnit;

	typedef enum
	{
		ADC_PERCENT
	} AdcUnit;

	typedef enum
	{
		ADC_BASEPRICE = 0,
		ADC_DISPLAY_TEXT,
		ADC_SCALE_MODE,
		ADC_EEPROM,
		ADC_TILT_COMP,
		ADC_SCALE_SPECIFIC_PARAM,
		ADC_SCALE_SPECIFIC_PARAM_SEALED,
		ADC_MAX_DISPL_TEXT_CHAR,
		ADC_FILTER,
		ADC_EEPROM_SIZE,
		ADC_CAL_STRINGS,
		ADC_G_FACTOR,
		ADC_PRODUCTION_SETTINGS,
		ADC_CONVERSION_WEIGHT_UNIT,
		ADC_INTERFACE_MODE,
		ADC_LOADCAPACITY_UNIT,
		ADC_ZERO_POINT_TRACKING,
		ADC_VERIF_PARAM_PROTECTED,
		ADC_UPDATE_ALLOWED,
		ADC_WARM_UP_TIME,
		ADC_REMAINING_WARM_UP_TIME,
		ADC_OPERATING_MODE,
		ADC_SPIRIT_LEVEL_CAL_MODE,
		ADC_ZERO_SETTING_INTERVAL,
		ADC_MAX_CAPACITY,
		ADC_AUTOMATIC_ZERO_SETTING_TIME,
		ADC_LOAD_CAPACITY_PARAMS,
		ADC_INITIAL_ZERO_SETTING,
		ADC_AUTOMATIC_TILT_SENSOR,
		ADC_TILT_ANGLE_CONFIRM
	} AdcValueType;

	typedef enum
	{
		ADC_TARE_WEIGHED = 0,
		ADC_TARE_KNOWN,
		ADC_TARE_WEIGHED_SET_ZERO,
		ADC_TARE_PERCENT,
		ADC_TARE_PERCENT_PLUS_WEIGHED,
		ADC_TARE_PERCENT_PLUS_KNOWN,
		ADC_TARE_ATTRIBUTE_CONST,
		ADC_TARE_LIMIT_DEVICE = 10,
		ADC_TARE_LIMIT_CW_CUSTOM,
		ADC_TARE_LIMIT_WEIGHED_CUSTOM,
		ADC_TARE_LIMIT_KNOWN_CUSTOM,
		ADC_TARE_NO = 100
	} AdcTareType;

    typedef enum
    {
        ADC_STP_FIRST = 0,
        ADC_STP_NONE
    } AdcTarePriority;

	typedef enum
	{
		ADC_PURE_SCALE = 0,		// no check
		ADC_SCALE_SB_MODE,		// check for motion, minimum load range 
		ADC_SCALE_AS_MODE,		// check for motion, minimum load range 
        ADC_SCALE_PA_MODE,		// check for motion, minimum load range
		ADC_SCALE_ZG_MODE,		// no check
		ADC_SCALE_CW_MODE		// check for motion, minimum load range (catchweigher/SWE)
	} AdcScaleMode;

	typedef enum 
	{
		ADC_CALIB_START,			// Start der Kalibrierung (Normal + Linearkalibrierung)
		ADC_CALIB_START_TILT_COMP,	// Start der Kalibrierung mit Neigungskompensation (Normal)
		ADC_CALIB_CANCEL,			// Abbruch Kalibrierung
		ADC_CALIB_GET_STATE,		// Status des ADWs pollen
		ADC_CALIB_STEP_CONFIRM		// Status des ADWs bestätigen (next Step)
	} AdcCalibCmd;

	typedef enum 
	{
		ADC_LOAD_DEF_CONFIG_SENSOR_HEALTH_DATA,		// load default adc sensor health configuration
		ADC_SAVE_SENSOR_HEALTH_DATA,				// save sensor health data
		ADC_FACTORY_SETTINGS,                       // load factory settings
		ADC_RESET_SENSOR_HEALTH_DATA				// reset sensor health data (parameter forbidden for application, only for production sw)
	} AdcParamMode;

	typedef enum 
	{
		ADC_WELMEC_REGION,			// eeprom welmec region
		ADC_OPEN_REGION,			// eeprom open region
		ADC_PROD_REGION,			// eeprom production region
		ADC_PROD_SENSORS_REGION		// eeprom production sensors region
	} AdcEepromRegion;

	typedef enum 
	{
		ADC_HARDWARE_RESET = 0,
		ADC_SOFTWARE_RESET
	} AdcResetType;

	typedef enum
	{
		ADC_BOOTLOADER = 0,
		ADC_APPLICATION
	} AdcOperatingMode;

	typedef enum
	{
		ADC_SPIRIT_LEVEL_RESET = 0,
		ADC_SPIRIT_LEVEL_SET_ZERO,
	} AdcSpiritLevelCmd;

	typedef enum
	{
		ADC_CONFIRMED_TILT_ANGLE_RESET = 0,
		ADC_CONFIRM_CURRENT_MAX_TILT_ANGLE,
		ADC_CONFIRM_CURRENT_MAX_TILT_ANGLE_FORCE
	} AdcTiltAngleConfirmCmd;

	/**
	******************************************************************************
	* structures
	******************************************************************************
	*/

    /**
    ******************************************************************************
    * adc states
    ******************************************************************************
    */
    typedef struct
    {
        unsigned long underZero : 1;             // scale in underload
		unsigned long underWeight : 1;           // weight within minimum load range
		unsigned long overWeight : 1;            // scale in overload
		unsigned long sameWeight : 1;            // no motion since last weighing
		unsigned long tareConst : 1;             // tare const
		unsigned long busy : 1;                  // scale busy
		unsigned long needAuthentication : 1;    // adc requests an authentication, after successful 
												 // authentfication the application gets weighing values
		unsigned long tiltCompOutsideLimit : 1;	 // tilt too large
		unsigned long calibMode : 2;			 // scale in calibration mode
		unsigned long weightUnstable : 1;		 // scale unstable
		unsigned long scaleNotReady : 1;		 // scale state 0: scale ready   1: scale is not ready
		unsigned long scaleCalError : 1;		 // 0: calib params ok	1: calib params corrupt
		unsigned long weightRegistered : 1;		 // 0: not possible to register weight  1: weight registered
		unsigned long calibState : 1;			 // 0: calibrated  1: pre-calibrated
		unsigned long needLogbook : 1;			 // 0: no logbook data necessary for authentication  1: logbook data necessary for authentication
		unsigned long zeroPointCorrPerformed : 1;	// 0: no zero point correction performed  1: zero point correction performed
		unsigned long zeroIndicator : 1;		 // 0: outside of zero indicator range   1: inside zero indicator range
		unsigned long warmUpTime : 1;			 // 0: warm up time expired  1: warm up time not expired
		unsigned long zeroPointCorrRequest : 1;	 // 0: no zero point correction request  1: zero point correction request
		unsigned long outsideZeroRange : 1;		 // 0: switch on scale is inside zero range  1: switch on scale is outside zero range
		unsigned long outsideZeroTrackingRange : 1;	 // 0: automatic zero tracking possible  1: automatic zero tracking impossible
		unsigned long reserved1 : 10;            // reserviert
        unsigned long reserved2 : 32;            // reserviert
	} AdcBitState;

    typedef union
    {
		unsigned long long state;
        AdcBitState bit;
    } AdcState;


    typedef struct
    {
        unsigned short	chksum;
        char	*swIdentity;
    } AdcAuthentication;


	typedef struct
	{
		long long		value;
		AdcWeightUnit	weightUnit;
        short           decimalPlaces;
	} AdcWeight;

	typedef struct
	{
		long long		value;
		AdcUnit			unit;
		short           decimalPlaces;
	} AdcPercent;

	typedef struct
	{
		long long		value;
		char			currency[10];		
        short           decimalPlaces;
	} AdcPrice;

    typedef struct
    {
        AdcPrice		price;
        AdcWeightUnit	weightUnit;
    } AdcBasePrice;

	typedef struct
	{
		short x;
		short y;
		char  text[255];						
	} AdcDisplayText;

	typedef struct
	{
		AdcTareType type;
		short frozen;					// is only considered on set
		AdcWeight value;				// this structure will be ignored if type is ADC_TARE_WEIGHT or ADC_TARE_WEIGHED_SET_ZERO or ADC_TARE_PERCENT or ADC_TARE_PERCENT_PLUS_WEIGHED
		AdcPercent percentage;			// this structure is only valid if type is ADC_TARE_PERCENT or ADC_TARE_PERCENT_PLUS_WEIGHED or ADC_TARE_PERCENT_PLUS_KNOWN
	} AdcTare;

	typedef struct
	{
		AdcEepromRegion	region;
		long			startAdr;
		short			len;
        short			direction;              // 0: read; 1: write
		unsigned char	data[1024];			
	} AdcEeprom;

	typedef struct
	{
		long			welmecRegion;
		long			openRegion;
		long			prodRegion;
		long			prodSensorsRegion;
	} AdcEepromSize;


	typedef struct
	{
		unsigned short tcOn : 1;							// Tilt compensation off / on
		unsigned short tcCornerLoadCorrectionOn : 1;		// Corner load correction off / on
		unsigned short tcWeightDependentTiltAngleOn : 1;	// Weight dependent tilt angle off / on
	} AdcTiltCompBitState;

	typedef union
	{
		AdcTiltCompBitState bit;
		unsigned short		all;
	} AdcTiltCompState;

	typedef struct
	{
		AdcTiltCompState	state;		// Tilt compensation state
		long	x;						// x-value for spirit level
		long	y;						// y-value for spirit level
		long	limit_angle_1;			// limit angle 1 tilt sensor 
		long	limit_angle_2;			// limit angle 2 tilt sensor (weight dependent) 
		long	resolution;				// resolution tilt sensor
	} AdcTiltComp;

	typedef struct
	{
		long	idx;					// Filter index
		long	offsetStableTime;		// offset for stable time
		long	stableRange;			// stable range
	} AdcFilter;

	typedef struct
	{
		char maxStr[128];
		char minStr[128];
		char eStr[128];
	} AdcCalStrings;

	typedef struct
	{
		unsigned short size;
		unsigned short prodSiteID;
		short gFactorOffset;
		char  date[10];
		char  wsc[21];
	} AdcProdSettings;

	typedef struct
	{
		short	autoDetection;
		short	mode;
	}AdcInterfaceMode;

	#define LOAD_CAPACITY_SIZE	20
	typedef struct
	{
		char loadCapacity[LOAD_CAPACITY_SIZE];
		long maxCapacity;
		long multiInterval;			// count of partial weighing ranges
		long limitInterval1;
		long limitInterval2;
		long e1;					// verification scale interval e1
		long e2;					// verification scale interval e2
		long e3;					// verification scale interval e3
		long tareLimitWeighed;
		long tareLimitKnown;
		short decimalPlaces1;		// decimal places interval 1
		short decimalPlaces2;		// decimal places interval 2
		short decimalPlaces3;		// decimal places interval 3
		AdcWeightUnit unit;
	} AdcLoadCapacityParams;

	typedef struct
	{
		short lowerLimit;
		short upperLimit;
	}AdcInitialZeroSettingParam;

	typedef struct
	{
		AdcValueType type;
		short frozen;
		union
		{
            AdcBasePrice			basePrice;
			AdcDisplayText			displayText;
			AdcScaleMode			scaleMode;
			AdcEeprom				eeprom;
			AdcEepromSize			eepromSize;
            AdcTiltComp				tiltCompensation;
			AdcFilter				filter;
			AdcCalStrings			calStrings;
			AdcProdSettings			prodSettings;
			AdcInterfaceMode		interfaceMode;
			AdcWeightUnit			loadCapacityUnit;
			AdcOperatingMode		opMode;
			AdcSpiritLevelCmd		spiritLevelCmd;
			AdcWeight				maxCapacity;
			AdcLoadCapacityParams		loadCapacityParams;
			AdcInitialZeroSettingParam	initialZeroSettingParam;
			AdcTiltAngleConfirmCmd	tiltAngleConfirmCmd;
			short           maxDisplTextChar;
			short			gFactor;
			long			conversionWeightUnitFactor;
			short			mode;
			short			verifParamProtected;
			short			updateAllowed;
			long			warmUpTime;
			long			zeroSettingInterval;
			long			automaticZeroSettingTime;
			bool			state;
		}ScaleValue;
	} AdcScaleValues;

	typedef struct
	{
		char	ID;
		long	value;
	} AdcInternalData;

	typedef struct
	{
		char	ID;
		long	value;
		bool	valid;
	} AdcInternalDataEx;

	typedef struct
	{
		long	value;
		long	unit;
	} AdcDiaValueUnit;

	#define DIA_MAX_VALUE_UNIT	10
	typedef struct
	{
		long number;
		AdcDiaValueUnit ValueUnit[DIA_MAX_VALUE_UNIT];
	} AdcHealthValueUnitArray;

	typedef struct
	{
		long type;
		char diaParam[5];
		union
		{
			AdcHealthValueUnitArray	type0;
			AdcHealthValueUnitArray	type100;
		}Health;
	} AdcSensorHealth;


	/**
	******************************************************************************
	* AdcOpen - Opens a connection to the Bizerba weighing system
	*
	* @param    adcName:in		adc identification
	* @param    protocol:in		protocol (rbs, minibus, siobus ...)
	* @param    port:in	        interface (usb, COMx ...)
	* @param    handle:out		adc handle
	* @param    performSwReset:in	!= 0 perform an Software-Reset
	*
	* @return   ADC_SUCCESS
	*			ADC_E_NO_DEVICE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcOpen(const char *adcName, const char *protocol, const char *port, short *handle, const unsigned char performSwReset);


	/**
	******************************************************************************
	* AdcClose - Close a connection to the Bizerba weighing system
	*
	* @param    handle:in		adc handle
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcClose(const short handle);


	/**
	******************************************************************************
	* AdcReset - Send command reset to the Bizerba weighing system
	*
	* @param    handle:in		adc handle
	* @param    type:in			reset type
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcReset(const short handle, const AdcResetType type);


	/**
	******************************************************************************
	* AdcSetScaleValues - function to set base price, display text,
	*					  adc mode, eeprom-data and tilt compensation
	*
	* @param    handle:in		adc handle
	* @param    scaleValues:in	adc value to set
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	* @remarks	For tilt-compensation only the values "state" and "limit" will be
	*			evaluated
	******************************************************************************
	*/
	BIZLARS_API short AdcSetScaleValues(const short handle, const AdcScaleValues *scaleValues);


	/**
	******************************************************************************
	* AdcSetBasePrice - function to set base price
	*
	* @param    handle:in		adc handle
	* @param    basePrice:in	base price to set
	* @param    frozen:in
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetBasePrice(const short handle, const AdcBasePrice *basePrice, short frozen);


	/**
	******************************************************************************
	* AdcSetDisplayText - function to set display text
	*
	* @param    handle:in		adc handle
	* @param    dispalyText:in	base price to set
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetDisplayText(const short handle, const AdcDisplayText *displayText);


	/**
	******************************************************************************
	* AdcSetScaleMode - function to set scale mode
	*
	* @param    handle:in		adc handle
	* @param    scaleMode:in	scale mode set
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetScaleMode(const short handle, const AdcScaleMode scaleMode);


	/**
	******************************************************************************
	* AdcSetEeprom - function to write to eeprom
	*
	* @param    handle:in		adc handle
	* @param    eeprom:in		eeprom data
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetEeprom(const short handle, const AdcEeprom *eeprom);


	/**
	******************************************************************************
	* AdcSetSspPath - function to set the path to the ssp files
	*
	* @param    handle:in	adc handle
	* @param    path:in		path to the ssp files
	*
	* @return	ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetSspPath(const short handle, const char *path);


	/**
	******************************************************************************
	* AdcSetTiltCompensation - function to configure tilt compensation
	*
	* @param    handle:in				adc handle
	* @param    tiltCompensation:in		tilt compensation
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks	For tilt-compensation only the values "state" and "limit" will be
	*			evaluated
	******************************************************************************
	*/
	BIZLARS_API short AdcSetTiltCompensation(const short handle, const AdcTiltComp *tiltCompensation);


	/**
	******************************************************************************
	* AdcSetScaleSpecificParam - function to set the scale specific parameters
	*
	* @param    handle:in				adc handle
	* @param	presealed:in			false: no sealable parameters
	*									true: sealable parameters
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	******************************************************************************
	*/
	BIZLARS_API short AdcSetScaleSpecificParam(const short handle, const bool presealed);


	/**
	******************************************************************************
	* AdcSetFilter - function to configure adc filter
	*
	* @param    handle:in	adc handle
	* @param    filter:in	filter
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetFilter(const short handle, const AdcFilter *filter);


	/**
	******************************************************************************
	* AdcSetGFactor - function to set the g factor
	*
	* @param    handle:in	adc handle
	* @param    gFactor:in	g factor
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetGFactor(const short handle, const short gFactor);


	/**
	******************************************************************************
	* AdcSetProdSettings - function to set the production settings
	*
	* @param    handle:in	adc handle
	* @param    prodSettings:in	production settings
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetProdSettings(const short handle, const AdcProdSettings *prodSettings);


	/**
	******************************************************************************
	* AdcSetConversionWeightUnit - function to set the factor for conversion the weight unit
	*
	* @param    handle:in	adc handle
	* @param    factor:in	multiply factor
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetConversionWeightUnit(const short handle, const long factor);


	/**
	******************************************************************************
	* AdcSetInterfaceMode - function to set the interface mode
	*
	* @param    handle:in	adc handle
	* @param    factor:in	multiply factor
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetInterfaceMode(const short handle, const AdcInterfaceMode *interfaceMode);


	/**
	******************************************************************************
	* AdcSetZeroPointTracking - function to set the zero point tracking mode
	*
	* @param    handle:in	adc handle
	* @param    mode:in		0 disable
	*						1 enable
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetZeroPointTracking(const short handle, const short mode);


	/**
	******************************************************************************
	* AdcSetZeroSettingInterval - function to set the zero setting interval
	*
	* @param    handle:in	adc handle
	* @param    time:in		zero setting interval in seconds
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetZeroSettingInterval(const short handle, const long time);


	/**
	******************************************************************************
	* AdcSetAutomaticZeroSettingTime - function to set the automatic zero setting time
	*
	* @param    handle:in	adc handle
	* @param    time:in		automatic zero setting time in seconds
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetAutomaticZeroSettingTime(const short handle, const long time);


	/**
	******************************************************************************
	* AdcSetVerificationParameterProtected - function to set the verification parameter to protected
	*
	* @param    handle:in	adc handle
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks	it's not possible to set the verification parameter to 0. To set it to 0
	*			the adc must boot in calibration mode
	******************************************************************************
	*/
	BIZLARS_API short AdcSetVerificationParameterProtected(const short handle);


	/**
	******************************************************************************
	* AdcSetUpdateMode - function to set the update mode
	*
	* @param    handle:in	adc handle
	* @param    mode:in		0 update not allowed
	*						1 update allowed
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks	it's not possible to set the verification parameter to 0. To set it to 0
	*			the adc must boot in calibration mode
	******************************************************************************
	*/
	BIZLARS_API short AdcSetUpdateMode(const short handle, const short mode);


	/**
	******************************************************************************
	* AdcSetWarmUpTime - function to set the warm up time in seconds
	*
	* @param    handle:in	adc handle
	* @param    time:in		warm up time in seconds
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetWarmUpTime(const short handle, const long time);


	/**
	******************************************************************************
	* AdcSetOperatingMode - function to set the adc operating mode
	*
	* @param    handle:in	adc handle
	* @param    mode:in		adc operating mode
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetOperatingMode(const short handle, const AdcOperatingMode mode);


	/**
	******************************************************************************
	* AdcCalSpiritLevel - function to calibrate the spirit level
	*
	* @param    handle:in	adc handle
	* @param    spiritLevelCmd:in	spirit level calibration command
	*
	* @return	ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcCalSpiritLevel(const short handle, const AdcSpiritLevelCmd spiritLevelCmd);


	/**
	******************************************************************************
	* AdcSetInitialZeroSetting - function to set the initial zero setting range
	*
	* @param    handle:in	adc handle
	* @param    param:in	pointer to structure initial zero setting range
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetInitialZeroSetting(const short handle, AdcInitialZeroSettingParam *param);


	/**
	******************************************************************************
	* AdcSetStateAutomaticTiltSensor - function to enable/disable the automatic tilt sensor
	*
	* @param    handle:in	adc handle
	* @param    state:in	false: disable
	*						true:  enable
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks	The mode can be set only in calibration mode
	******************************************************************************
	*/
	BIZLARS_API short AdcSetStateAutomaticTiltSensor(const short handle, const bool state);


	/**
	******************************************************************************
	* AdcGetScaleValues - function to get tare, base price, display text,
	*					  adc mode, eeprom-data and tilt compensation
	*
	* @param    handle:in		adc handle
	* @param    scaleValues:out	adc value to get
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks	For tilt-compensation only the values "state" and "limit" will be
	*			evaluated
	******************************************************************************
	*/
	BIZLARS_API short AdcGetScaleValues(const short handle, AdcScaleValues *scaleValues);


	/**
	******************************************************************************
	* AdcGetBasePrice - function to get base price
	*
	* @param    handle:in		adc handle
	* @param    basePrice:out	base price
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks	
	******************************************************************************
	*/
	BIZLARS_API short AdcGetBasePrice(const short handle, AdcBasePrice *basePrice);


	/**
	******************************************************************************
	* AdcGetDisplayText - function to get display text
	*
	* @param    handle:in		adc handle
	* @param    displayText:out	display text
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetDisplayText(const short handle, AdcDisplayText *displayText);


	/**
	******************************************************************************
	* AdcGetScaleMode - function to get the scale mode
	*
	* @param    handle:in		adc handle
	* @param    scaleMode:out	scale mode
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetScaleMode(const short handle, AdcScaleMode *scaleMode);


	/**
	******************************************************************************
	* AdcGetEeprom - function to read eeprom
	*
	* @param    handle:in		adc handle
	* @param    eeprom:out		eeprom data
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetEeprom(const short handle, AdcEeprom *eeprom);


	/**
	******************************************************************************
	* AdcGetEepromSize - function to get eeprom size
	*
	* @param    handle:in		adc handle
	* @param    eepromSize:out	eeprom size
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetEepromSize(const short handle, AdcEepromSize *eepromSize);
	

	/**
	******************************************************************************
	* AdcGetTiltCompensation - function to get tilt compensation
	*
	* @param    handle:in		adc handle
	* @param    eepromSize:out	eeprom size
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetTiltCompensation(const short handle, AdcTiltComp *tiltCompensation);


	/**
	******************************************************************************
	* AdcGetMaxDisplayTextChar - function to get max display text length
	*
	* @param    handle:in		adc handle
	* @param    maxDisplayTextChar:out	max number
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetMaxDisplayTextChar(const short handle, short *maxDisplayTextChar);


	/**
	******************************************************************************
	* AdcGetFilter - function to get filter parameter
	*
	* @param    handle:in		adc handle
	* @param    filter:out		filter parameter
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetFilter(const short handle, AdcFilter *filter);


	/**
	******************************************************************************
	* AdcGetCalStrings - function to get the calibration strings min/max/e
	*
	* @param    handle:in		adc handle
	* @param    calStrings:out	structure for calibration strings
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetCalStrings(const short handle, AdcCalStrings *calStrings);


	/**
	******************************************************************************
	* AdcGetCalStrings4LoadCapacity - function to get the calibration strings min/max/e
	*								  for a specific load capacity
	* @param    handle:in		adc handle
	* @param	loadCapacity:in	load capacity
	* @param    calStrings:out	structure for calibration strings
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_FILE_CORRUPT
	*			ADC_E_FILE_NOT_FOUND
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetCalStrings4LoadCapacity(const short handle, const char *loadCapacity, AdcCalStrings *calStrings);


	/**
	******************************************************************************
	* AdcGetGFactor - function to get the g factor
	*
	* @param    handle:in		adc handle
	* @param    gFactor:out		pointer to g factor
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetGFactor(const short handle, short *gFactor);


	/**
	******************************************************************************
	* AdcGetProdSettings - function to get the production settings
	*
	* @param    handle:in		adc handle
	* @param    gFactor:out		pointer to production settings
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetProdSettings(const short handle, AdcProdSettings *prodSettings);


	/**
	******************************************************************************
	* AdcGetInterfaceMode - function to get the interface mode
	*
	* @param    handle:in		adc handle
	* @param    interfaceMode:out		pointer to interface mode
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetInterfaceMode(const short handle, AdcInterfaceMode *interfaceMode);


	/**
	******************************************************************************
	* AdcGetLoadCapacityUnit - function to get the load capacity unit
	*
	* @param    handle:in		adc handle
	* @param    unit:out		unit
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetLoadCapacityUnit(const short handle, AdcWeightUnit *unit);


	/**
	******************************************************************************
	* AdcGetZeroPointTracking - function to get the zero point tracking mode
	*
	* @param    handle:in	adc handle
	* @param    mode:out	mode
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetZeroPointTracking(const short handle, short *mode);


	/**
	******************************************************************************
	* AdcGetZeroSettingInterval - function to get the zero setting intveral in seconds
	*
	* @param    handle:in	adc handle
	* @param    time:in		zero setting interval in seconds
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetZeroSettingInterval(const short handle, long *time);


	/**
	******************************************************************************
	* AdcGetAutomaticZeroSettingTime - function to get the automatic zero setting time in seconds
	*
	* @param    handle:in	adc handle
	* @param    time:in		automatic zero setting time in seconds
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetAutomaticZeroSettingTime(const short handle, long *time);


	/**
	******************************************************************************
	* AdcGetVerificationParameter - function to get the state of the verification paramter
	*
	* @param    handle:in	adc handle
	* @param    mode:out	0 not protected
	*						1 protected
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetVerificationParameter(const short handle, short *mode);


	/**
	******************************************************************************
	* AdcGetUpdateMode - function to get the state of the update mode
	*
	* @param    handle:in	adc handle
	* @param    mode:out	0 not allowed
	*						1 allowed
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetUpdateMode(const short handle, short *mode);


	/**
	******************************************************************************
	* AdcGetWarmUpTime - function to get the warm up time in seconds
	*
	* @param    handle:in	adc handle
	* @param    time:out	warm up time in seconds
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetWarmUpTime(const short handle, long *time);


	/**
	******************************************************************************
	* AdcGetRemainingWarmUpTime - function to get the reamining warm up time in seconds
	*
	* @param    handle:in	adc handle
	* @param    time:out	warm up time in seconds
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetRemainingWarmUpTime(const short handle, long *time);


	/**
	******************************************************************************
	* AdcGetMaxCapacity - function to get the maximum capacity of the load cell
	*
	* @param    handle:in		adc handle
	* @param    maxCapacity:out	maximum capacity of the load cell
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetMaxCapacity(const short handle, AdcWeight *maxCapacity);


	/**
	******************************************************************************
	* AdcGetLoadCapacityParams - function to get the load capacity parameters
	*
	* @param    handle:in		adc handle
	* @param    lcparams:out	load capacity params
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetLoadCapacityParams(const short handle, AdcLoadCapacityParams *lcparams);


	/**
	******************************************************************************
	* AdcGetOperatingMode - function to get the adc operating mode
	*
	* @param    handle:in	adc handle
	* @param    mode:out	adc operating mode
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetOperatingMode(const short handle, AdcOperatingMode *mode);


	/**
	******************************************************************************
	* AdcGetInitialZeroSetting - function to get the initial zero setting range
	*
	* @param    handle:in	adc handle
	* @param    param:in	structure initial zero setting range
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetInitialZeroSetting(const short handle, AdcInitialZeroSettingParam *param);


	/**
	******************************************************************************
	* AdcGetStateAutomaticTiltSensor - function to get the state of the automatic tilt sensor
	*
	* @param    handle:in	adc handle
	* @param    state:out	false: disable
	*						true:  enable
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetStateAutomaticTiltSensor(const short handle, bool *state);


	/**
	******************************************************************************
	* AdcZeroScale - function to set the scale zero
	*
	* @param    handle:in		adc handle
	* @param    adcState:out	adc state
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_FAILURE
	*			ADC_E_USB_ERROR
	*			ADC_E_AUTHENTICATION
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
    BIZLARS_API short AdcZeroScale(const short handle, AdcState *adcState);


	/**
	******************************************************************************
	* AdcSetTare - function to set tare (weighed, known, ...)
	*
	* @param    handle:in		adc handle
	* @param    adcState:out	adc state
    * @param    tare:in     	tare structure
    *
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_FAILURE
	*			ADC_E_USB_ERROR
	*			ADC_E_AUTHENTICATION
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
    BIZLARS_API short AdcSetTare(const short handle, AdcState *adcState, const AdcTare *tare);


	/**
	******************************************************************************
	* AdcGetTare - function to get tare limits weighed and known
	*
	* @param    handle:in		adc handle
	* @param    adcState:out	adc state
	* @param    tare:in/out    	tare structure
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_FAILURE
	*			ADC_E_USB_ERROR
	*			ADC_E_AUTHENTICATION
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetTare(const short handle, AdcState *adcState, AdcTare *tare);


    /**
    ******************************************************************************
    * AdcSetTarePriority - function to set the tare priority
    *
    * @param    handle:in		adc handle
    * @param    adcState:out	adc state
    * @param    prio:in     	tare priority
    *
    * @return   ADC_SUCCESS
    *			ADC_E_INVALID_HANDLE
    *			ADC_E_ADC_FAILURE
    *			ADC_E_USB_ERROR
    *			ADC_E_AUTHENTICATION
 	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
   * @remarks
    ******************************************************************************
    */
    BIZLARS_API short AdcSetTarePriority(const short handle, AdcState *adcState, const AdcTarePriority prio);


	/**
	******************************************************************************
	* AdcClearTare - function to clear tare
	*
	* @param    handle:in		adc handle
	* @param    adcState:out	adc state
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_FAILURE
	*			ADC_E_USB_ERROR
	*			ADC_E_AUTHENTICATION
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
    BIZLARS_API short AdcClearTare(const short handle, AdcState *adcState);

	/**
	******************************************************************************
	* AdcReadWeight - function to get the scale values
	*
	* @param    handle:in				adc handle
	* @param    registrationRequest:in	id for registration
	* @param    adcState:out			adc state
	* @param    weight:out				weight value
	* @param    tare:out				tare value
	* @param    basePrice:out			base price value
	* @param    sellPrice:out			sell price value
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_FAILURE
	*			ADC_E_USB_ERROR
	*			ADC_E_AUTHENTICATION
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks	If the application wants to register the article the value registrationRequest must set to 1
	*			The application must evaluate adcState for checking if registration was successful
	*			If the scale mode is set to ADC_PURE_SCALE this value registrationRequest will be ignored
	******************************************************************************
	*/
    BIZLARS_API short AdcReadWeight(const short handle, const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcTare *tare, AdcBasePrice *basePrice, AdcPrice *sellPrice);


	/**
	******************************************************************************
	* AdcGetCapability - function to get the scale capabilities
	*
	* @param    handle:in				adc handle
	* @param    cap:in					scale capability
	* @param    value:out				cap value 0: not supported
	*									cap value 1: supported
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_INVALID_PARAMETER
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetCapability(const short handle, const AdcCapabilities cap, unsigned char *value);


	/**
	******************************************************************************
	* AdcGetLogger - function to get adc log book
	*
	* @param    handle:in				adc handle
	* @param    index:in				index for log book entry
	* @param    entry:out				one log book row
    * @param    size:in/out             size of entry in byte
	*
	* @return   ADC_SUCCESS
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_LOGBOOK_NO_FURTHER_ENTRY
	*			ADC_E_LOGBOOK_FULL
	* @remarks	The latest log book entry has index = 0
	******************************************************************************
	*/
    BIZLARS_API short AdcGetLogger(const short handle, const short index, char *entry, unsigned long *size);


    /**
    ******************************************************************************
    * AdcGetRandomAuthentication - function to get the random value for authentication
    *
    * @param    handle:in				adc handle
    * @param    random:out				random value necessary for authentication
    * @param    size:in/out 			size of random value
    *
    * @return   ADC_SUCCESS
    *			ADC_E_ADC_FAILURE
    *			ADC_E_USB_ERROR
    *			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
    ******************************************************************************
    */
    BIZLARS_API short AdcGetRandomAuthentication(const short handle, unsigned char *random, unsigned long *size);


	/**
	******************************************************************************
	* AdcSetAuthentication - function to make the for authentication
	*
	* @param    handle:in				adc handle
	* @param    authentication:in		struct for authentication
	*
	* @return   ADC_SUCCESS
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
    BIZLARS_API short AdcSetAuthentication(const short handle, const AdcAuthentication *authentication);


	/**
	******************************************************************************
	* AdcSetCountryFilesPath - function to set the path to the adc country files
	*
	* @param    handle:in	adc handle
	* @param    path:in		path to the country files
	*
	* @return	ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetCountryFilesPath(const short handle, const char *path);


	/**
	******************************************************************************
	* AdcGetSupportedCountries - function to get all supported countries
	*
	* @param    handle:in		adc handle
	* @param    countries:out	string with the supported countries
	* @param    size:in		    size of variable name
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*           ADC_E_NOT_ENOUGH_MEMORY
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetSupportedCountries(const short handle, char *countries, unsigned long *size);


	/**
	******************************************************************************
	* AdcSetCountry - function to set the country 
	*
	* @param    handle:in				adc handle
	* @param    name:in				    ISO country name
	*
	* @return   ADC_SUCCESS
	*			ADC_E_ADC_FAILURE
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_HANDLE
	*           ADC_E_FILE_CORRUPT
	*           ADC_E_FILE_NOT_FOUND
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetCountry(const short handle, const char *name);


    /**
    ******************************************************************************
    * AdcGetCountry - function to get the adc country
    *
    * @param    handle:in		adc handle
    * @param    name:out		ISO country name
    * @param    size:in		    size of variable name
    *
    * @return   ADC_SUCCESS
    *			ADC_E_INVALID_HANDLE
    *           ADC_E_NOT_ENOUGH_MEMORY
    * @remarks
    ******************************************************************************
    */
    BIZLARS_API short AdcGetCountry(const short handle, char *name, unsigned long size);


    /**
    ******************************************************************************
    * AdcGetCompatibleLoadCapacities - function to get the compatible load capacities
    *
    * @param    handle:in				adc handle
	* @param    country:in				ISO country name, NULL-Ptr for current country
	* @param    loadCapacities:out		string with the load capacities
    * @param    size:in/out				size of variable name
    *
    * @return   ADC_SUCCESS
    *           ADC_E_INVALID_HANDLE
	*           ADC_E_INVALID_PARAMETER
    *           ADC_E_NOT_ENOUGH_MEMORY
    * @remarks
    ******************************************************************************
    */
	BIZLARS_API short AdcGetCompatibleLoadCapacities(const short handle, const char *country, char *loadCapacities, unsigned long *size);


    /**
    ******************************************************************************
    * AdcSetLoadCapacity - function to set one load capacity
    *
    * @param    loadCapacity:in loadcapacity
    *
    * @return   ADC_SUCCESS
    *           ADC_E_INVALID_HANDLE
    *           ADC_E_INVALID_PARAMETER
    *			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FILE_NOT_FOUND
	* @remarks
    ******************************************************************************
    */
    BIZLARS_API short AdcSetLoadCapacity(const short handle, const char *loadCapacity);


	/**
	******************************************************************************
	* AdcGetLoadCapacity - function to get the curent load capacity
	*
	* @param    loadCapacity:out	loadcapacity
	* @param    size:in				size of variable loadcapacity
	*
	* @return   ADC_SUCCESS
	*           ADC_E_INVALID_HANDLE
	*           ADC_E_INVALID_PARAMETER
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetLoadCapacity(const short handle, char *loadCapacity, unsigned long size);

	
	/**
	******************************************************************************
	* AdcGetHighResolution - function to get the high resolution values from the adc
	*
	* @param    handle:in					adc handle
	* @param    registrationRequest:in		id for registration
    * @param    adcState:out				adc state
	* @param    weight:out					weight
	* @param    weightHighResolution:out	high resolution weight
	* @param    tare:out					tare value
	* @param    digitValue:out				weight as digit value
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_FAILURE
	*			ADC_E_USB_ERROR
	*			ADC_E_AUTHENTICATION
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED* @remarks
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetHighResolution(const short handle, const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcWeight *weightHighResolution, AdcTare *tare, long *digitValue);


	/**
	******************************************************************************
	* AdcGetGrossWeight - function to get the gross weights from the adc
	*
	* @param    handle:in					adc handle
	* @param    adcState:out				adc state
	* @param    grossWeight:out				gross weight
	* @param    grossWeightHighResolution:out	high resolution gross weight
	* @param    digitValue:out				weight as digit value
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_FAILURE
	*			ADC_E_USB_ERROR
	*			ADC_E_AUTHENTICATION
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED* @remarks
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetGrossWeight(const short handle, AdcState *adcState, AdcWeight *grossWeight, AdcWeight *grossWeightHighResolution, long *digitValue);


	/**
	******************************************************************************
	* AdcSetFirmwarePath - function to set the path to the firmware files
	*
	* @param    handle:in	adc handle
	* @param    path:in		path to the firmware files
	*
	* @return	ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetFirmwarePath(const short handle, const char *path);


	/**
	******************************************************************************
	* AdcUpdate - function to update the adc
	*
	* @param    handle:in		adc handle
	* @param    force:in		0: check parameter if update is possible
	*							1: no check, always execute update
	*
	* @return	ADC_SUCCESS
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_HANDLE
	*           ADC_E_FILE_CORRUPT
	*			ADC_E_FILE_NOT_FOUND
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcUpdate(const short handle, const short force = 0);


	/**
	******************************************************************************
	* AdcGetFirmwareFileVersion - function get version of the firmware file
	*
	* @param    handle:in					adc handle
	* @param    versionStr:out				version string of the firmware file
	* @param    size:in/out				    size of versionStr
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_FILE_CORRUPT
	*			ADC_E_FILE_NOT_FOUND
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetFirmwareFileVersion(const short handle, char *versionStr, unsigned long *size);


	/**
	******************************************************************************
	* AdcGetVersion - function get version from adc, library and bootloader
	*
	* @param    handle:in					adc handle
    * @param    versionStr:out				version string includes all version of 
    *                                       the components
    * @param    size:in/out				    size of versionStr
    *
	* @return   ADC_SUCCESS
    *			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
    BIZLARS_API short AdcGetVersion(const short handle, char *versionStr, unsigned long *size);


	/**
	******************************************************************************
	* AdcGetFirstDiagnosticData - function to get diagnostic data from the adc
	*
	* @param    handle:in		adc handle
	* @param    name:in			if name == EmptyString the function give all diagnostic values
	* @param    sensorHealthID:out	sensor health id for the function GetNextDiagnosticData
	* @param    sensorHealth:out	sensor health data
	* @param	state:out			0	no more diagnostic data
	*								1	succeed  
	*
	* @return   ADC_SUCCESS
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_SENSOR_HEALTH_QUEUE_FULL
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetFirstDiagnosticData(const short handle, const char *name, long *sensorHealthID, AdcSensorHealth *sensorHealth, short *state);


	/**
	******************************************************************************
	* AdcGetNextDiagnosticData - function to get diagnostic data from the adc
	*
	* @param    handle:in		adc handle
	* @param    sensorHealthID:in	sensor health id
	* @param    sensorHealth:out	sensor health data
	* @param	state:out			0	no more diagnostic data
	*								1	succeed
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetNextDiagnosticData(const short handle, const long sensorHealthID, AdcSensorHealth *sensorHealth, short *state);


	/**
	******************************************************************************
	* AdcConfigureDiagnosticData - function to configure adc diagnostic data (threshold,...)
	*
	* @param    handle:in		adc handle
	* @param    sensorHealth:in	sensor health data
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_INVALID_PARAMETER
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcConfigureDiagnosticData(const short handle, const AdcSensorHealth *sensorHealth);
	
	
	/**
	******************************************************************************
	* AdcCalibration - function for scale calibration
	*
	* @param    handle:in		adc handle
	* @param    cmd:in			calibration command
	* @param    adcState:out    state of the adc
	* @param    step:out		calibration step
	* @param    calibDigit:out	weight in digit in calibration mode
	*
	* @return	ADC_SUCCESS
	*			ADC_E_USB_ERROR
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcCalibration(const short handle, const AdcCalibCmd cmd, AdcState *adcState, long *step, long *calibDigit);

	
	/**
	******************************************************************************
	* AdcParameters - Send adc parameter mode to the Bizerba weighing system
	*
	* @param    handle:in		adc handle
	* @param	mode:in			0: load default sensor health configuration
	*							1: save sensor health data
	*							2: load factory settings
	*							3: reset sensor health data
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcParameters(const short handle, const AdcParamMode mode);


	/**
	******************************************************************************
	* AdcSetScaleModel - function to set the scale model
	*
	* @param    handle:in	adc handle
	* @param    model:in	scale model
	*
	* @return	ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcSetScaleModel(const short handle, const char *model);


	/**
	******************************************************************************
	* AdcGetScaleModel - function to get the scale model
	*
	* @param    handle:in	adc handle
	* @param    model:out	scale model
	* @param    size:in		size of variable model
	*
	* @return	ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetScaleModel(const short handle, char *model, unsigned long size);


    /**
	******************************************************************************
	* AdcSetTrace - function to activate the trace for the lib
	*
	* @param    fileName:in		path + filename for the trace file
	* @param    level:in		tracelevel	ADC_TRC_ERROR_WARNING
	*										ADC_TRC_ACTION
	*										ADC_TRC_INFO
	*
	* @return   
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API void AdcSetTrace(const char *fileName, const short level);


	/**
	******************************************************************************
	* AdcTiltAngleConfirmed - function to confirmed that the tilt compensation is correct under max angle
	*
	* @param    handle:in			adc handle
	* @param    cmd:in				ADC_CONFIRMED_TILT_ANGLE_RESET				reset the confirmed titl angle
	*								ADC_CONFIRM_CURRENT_MAX_TILT_ANGLE			only a greater max tilt angle will taken over
	*								ADC_CONFIRM_CURRENT_MAX_TILT_ANGLE_FORCE	the current max tilt angle will taken over
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	*			ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE
	*			ADC_E_EEPROM_ACCESS_VIOLATION
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcTiltAngleConfirmed(const short handle, const AdcTiltAngleConfirmCmd cmd);


	/**
	******************************************************************************
	* AdcGetInternalData - function to get the internal data of the adc, for
	*					   example n, l, M ...
	*
	* @param    handle:in			adc handle
	* @param    internalData:in/out	pointer of array internal data
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetInternalData(const short handle, AdcInternalData *internalData);


	/**
	******************************************************************************
	* AdcGetInternalDataEx - function to get the internal data of the adc, for
	*					     example n, l, M ...
	*
	* @param    handle:in				adc handle
	* @param    internalDataEx:in/out	pointer of array internal data
	* @param    sendRequestCmd:in		send the request command (supported only for special adw versions !!)
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	*			ADC_E_USB_ERROR
	*			ADC_E_ADC_TIMEOUT
	*			ADC_E_COMMAND_NOT_EXECUTED
	*			ADC_E_FUNCTION_NOT_IMPLEMENTED
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetInternalDataEx(const short handle, AdcInternalDataEx *internalDataEx, bool sendRequestCmd);


	/**
	******************************************************************************
	* AdcGetPortNr - function to get port number of the physical interface
	*
	* @param    handle:in				adc handle
	* @param    portNr:out				portNr
	* @param    size:in					size of the variable portNr
	*
	* @return   ADC_SUCCESS
	*			ADC_E_INVALID_HANDLE
	* @remarks
	******************************************************************************
	*/
	BIZLARS_API short AdcGetPortNr(const short handle, char *portNr, unsigned long *size);

#ifdef __cplusplus
}
#endif


