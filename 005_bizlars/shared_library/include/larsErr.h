/**
******************************************************************************
* File       : larsErr.h
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : lars class
******************************************************************************
*/
#pragma once

class LarsErr
{
public:
    static const short  E_SUCCESS = 0;
    static const short  E_INVALID_HANDLE = 1;
    static const short  E_NO_DEVICE = 2;
    static const short  E_INVALID_PARAMETER = 3;
    static const short  E_NOT_ENOUGH_MEMORY = 4;
    static const short  E_ADC_ERROR = 5;
    static const short  E_COUNTRY_NOT_SUPPORTED = 6;
    static const short  E_FILE_CORRUPT = 7;
    static const short  E_FILE_NOT_FOUND = 8;
    static const short  E_PROTOCOL = 9;
	static const short  E_PROTOCOL_CRC = 10;
    static const short  E_KEY_NOT_FOUND = 11;
	static const short  E_ADC_TIMEOUT = 12;
	static const short	E_EEPROM_ACCESS_VIOLATION = 13;
	static const short  E_SENSOR_HEALTH_QUEUE_FULL = 14;
	static const short	E_LOAD_CAPACITY_NOT_SUPPORTED = 15;

	// error from adc
	static const short	E_COMMAND_NOT_EXECUTED = 101;
	static const short  E_FUNCTION_NOT_IMPLEMENTED = 102;
	static const short	E_AUTHENTICATION = 103;
	static const short	E_COMMAND_NOT_SUPPORTED_IN_THIS_OPERATING_MODE = 104;
	static const short	E_LOGBOOK_NO_FURTHER_ENTRY = 105;
	static const short	E_LOGBOOK_FULL = 106;
	static const short  E_TILT_COMPENSATION_SWITCH_ON = 107;
	static const short  E_CHKSUM_BLOCK_3_4 = 108;
	static const short	E_LINEAR_CALIBRATION = 109;
	static const short	E_COMMAND_ONLY_FOR_SEPARATE_LOADCELL = 110;
	static const short	E_UPDATE_NOT_ALLOWED = 111;
	static const short	E_TCC_SWITCH_ON = 112;
	static const short	E_INCOMPATIBLE_PROD_DATA = 113;
	static const short	E_TARE_OCCUPIED = 114;
	static const short	E_KNOWN_TARE_LESS_E = 115;				// known tare is less than the verification scale interval e
	static const short	E_BATCH_TARE_NOT_ALLOWED = 116;
	static const short	E_TARE_OUT_OF_RANGE = 117;				// tare could not be set because greater limit or gross weight < 0
	static const short	E_TARE_OUTSIDE_CLEARING_AREA = 118;		// tare could not be cleared because tare value is not identical to the negative weight value
	static const short	E_WS_NOT_SUPPORT_LOAD_CAPACITY = 119;	// weighing system does not support load capacity
	static const short  E_CHKSUM_FIRMWARE = 120;				// wrong firmware checksum

};

