/**
******************************************************************************
* File       : bizlars.cpp
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : BizLars functions
******************************************************************************
*/

#include "stdafx.h"
#ifdef  _MSC_VER
#include "vld.h"		// Header for Visual Leak Detector for Visual Studio
#endif
#include <sstream>
#include <string>
#include <mutex>
#include <list>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <version.h>
#ifdef  __GNUC__
#include <string.h>
#endif
#include "larsErr.h"
#include "bizlars.h"
#include "adctrace.h"
#include "adctilt.h"
#include "lars.h"

/**
******************************************************************************
* typedefs
******************************************************************************
*/
typedef list <Lars *> LarsList;

static mutex g_larsMutex;
static LarsList g_larsList;

/**
******************************************************************************
* internal functions
******************************************************************************
*/
bool CheckIfAdcIsAlreadyOpen(list <Lars *> &larsList, Lars *lars);
short CreateLogicalDeviceHandle(list <Lars *> &larsList);
Lars *AdcCheckHandle(list <Lars *> &larsList, const short handle);
short ConvertLarsE2bizlarsE(short errorCode);


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
BIZLARS_API short AdcOpen(const char *adcName, const char *protocol, const char *port, short *handle, const unsigned char performSwReset)
{
    short   retCode = LarsErr::E_SUCCESS;
	bool	tmpPerformSwReset = true;
	Lars *lars;

	*handle = 0;
    if (adcName)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart AdcName: %s", __FUNCTION__, adcName);
        lars = new Lars(adcName, protocol, port);
    }
    else
    {
        g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart", __FUNCTION__);
        lars = new Lars();
    }

    // check if adc already open
	if (CheckIfAdcIsAlreadyOpen(g_larsList, lars))
	{
		g_adcTrace.Trace(AdcTrace::TRC_INFO, "%s\tend retCode: %d", __FUNCTION__, ADC_E_ADC_ALREADY_OPEN);
        delete lars;
		return ADC_E_ADC_ALREADY_OPEN;
	}

	// connect to adc
	if (!performSwReset) tmpPerformSwReset = false;
	retCode = lars->Open(tmpPerformSwReset);
	if (retCode != LarsErr::E_SUCCESS)
	{
        delete lars;
        retCode = ConvertLarsE2bizlarsE(retCode);
        g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
		return retCode;
	}

	// create the logical device handle
    lars->SetHandle(CreateLogicalDeviceHandle(g_larsList));
	*handle = lars->GetHandle();

    // queue lars object in list
    g_larsMutex.lock();
    g_larsList.push_back(lars);
    g_larsMutex.unlock();

    retCode = ConvertLarsE2bizlarsE(retCode);
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


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
short AdcClose(const short handle)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    lars->Close();

    // remove lars from list
    g_larsMutex.lock();
    g_larsList.remove(lars);
    g_larsMutex.unlock();

    // delete lars
    delete lars;

    retCode = ConvertLarsE2bizlarsE(retCode);
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


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
short AdcReset(const short handle, const AdcResetType type)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }
    
    retCode = ConvertLarsE2bizlarsE(lars->Reset(type));
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


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
short AdcSetScaleValues(const short handle, const AdcScaleValues *scaleValues)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tretCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    g_adcTrace.Trace(AdcTrace::TRC_INFO, "%s\tstruct type %d", __FUNCTION__, scaleValues->type);
    retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(scaleValues));

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}

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
short AdcSetBasePrice(const short handle, const AdcBasePrice *basePrice, short frozen)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_BASEPRICE;
	scaleValues.frozen = frozen;
	scaleValues.ScaleValue.basePrice = *basePrice;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcSetDisplayText - function to set display text
*
* @param    handle:in		adc handle
* @param    displayText:in	base price to set
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
short AdcSetDisplayText(const short handle, const AdcDisplayText *displayText)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_DISPLAY_TEXT;
	scaleValues.ScaleValue.displayText = *displayText;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcSetScaleMode(const short handle, const AdcScaleMode scaleMode)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_SCALE_MODE;
	scaleValues.ScaleValue.scaleMode = scaleMode;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcSetEeprom(const short handle, const AdcEeprom *eeprom)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_EEPROM;
	scaleValues.ScaleValue.eeprom = *eeprom;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcSetSspPath(const short handle, const char *path)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	lars->SetSspPath(path);

	retCode = ConvertLarsE2bizlarsE(retCode);
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcSetTiltCompensation(const short handle, const AdcTiltComp *tiltCompensation)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_TILT_COMP;
	scaleValues.ScaleValue.tiltCompensation = *tiltCompensation;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcSetScaleSpecificParam - function to set the scale specific parameters
*
* @param    handle:in				adc handle
* @param	sealed:in				false: not sealable parameters
*									true: sealable parameters
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
******************************************************************************
*/
short AdcSetScaleSpecificParam(const short handle, const bool presealed)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (presealed == true)
		scaleValues.type = AdcValueType::ADC_SCALE_SPECIFIC_PARAM_SEALED;
	else
		scaleValues.type = AdcValueType::ADC_SCALE_SPECIFIC_PARAM;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcSetFilter(const short handle, const AdcFilter *filter)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_FILTER;
	scaleValues.ScaleValue.filter = *filter;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcSetGFactor(const short handle, const short gFactor)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_G_FACTOR;
	scaleValues.ScaleValue.gFactor = gFactor;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcSetProdSettings(const short handle, const AdcProdSettings *prodSettings)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_PRODUCTION_SETTINGS;
	scaleValues.ScaleValue.prodSettings = *prodSettings;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcSetConversionWeightUnit(const short handle, const long factor)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_CONVERSION_WEIGHT_UNIT;
	scaleValues.ScaleValue.conversionWeightUnitFactor = factor;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcSetInterfaceMode(const short handle, const AdcInterfaceMode *interfaceMode)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_INTERFACE_MODE;
	scaleValues.ScaleValue.interfaceMode = *interfaceMode;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcSetZeroPointTracking(const short handle, const short mode)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_ZERO_POINT_TRACKING;
	scaleValues.ScaleValue.mode = mode;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcSetZeroSettingInterval - function to set the zero setting interval in seconds
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
BIZLARS_API short AdcSetZeroSettingInterval(const short handle, const long time)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_ZERO_SETTING_INTERVAL;
	scaleValues.ScaleValue.zeroSettingInterval = time;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcSetAutomaticZeroSettingTime - function to set the automatic zero setting time in seconds
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
BIZLARS_API short AdcSetAutomaticZeroSettingTime(const short handle, const long time)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_AUTOMATIC_ZERO_SETTING_TIME;
	scaleValues.ScaleValue.automaticZeroSettingTime = time;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcSetVerificationParameterProtected(const short handle)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_VERIF_PARAM_PROTECTED;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcSetUpdateMode(const short handle, const short mode)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_UPDATE_ALLOWED;
	scaleValues.ScaleValue.updateAllowed = mode;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcSetWarmUpTime(const short handle, const long time)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_WARM_UP_TIME;
	scaleValues.ScaleValue.warmUpTime = time;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcSetOperatingMode - function to set the operating mode
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
* @remarks	it's not possible to set the verification parameter to 0. To set it to 0
*			the adc must boot in calibration mode
******************************************************************************
*/
BIZLARS_API short AdcSetOperatingMode(const short handle, const AdcOperatingMode mode)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_OPERATING_MODE;
	scaleValues.ScaleValue.opMode = mode;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcCalSpiritLevel(const short handle, const AdcSpiritLevelCmd spiritLevelCmd)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_SPIRIT_LEVEL_CAL_MODE;
	scaleValues.ScaleValue.spiritLevelCmd = spiritLevelCmd;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcSetInitialZeroSetting - function to set the initial zero setting range
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
BIZLARS_API short AdcSetInitialZeroSetting(const short handle, AdcInitialZeroSettingParam* param)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_INITIAL_ZERO_SETTING;
	scaleValues.ScaleValue.initialZeroSettingParam = *param;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcSetStateAutomaticTiltSensor - function to enable/disable the automatic tilt sensor
*
* @param    handle:in	adc handle
* @param    state:in	false: disable
*						treu:  enable
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
BIZLARS_API short AdcSetStateAutomaticTiltSensor(const short handle, const bool state)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_AUTOMATIC_TILT_SENSOR;
	scaleValues.ScaleValue.state = state;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcGetScaleValues(const short handle, AdcScaleValues *scaleValues)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

	if (scaleValues)
	{
		g_adcTrace.Trace(AdcTrace::TRC_INFO, "%s\tstruct type %d", __FUNCTION__, scaleValues->type);
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(scaleValues));
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


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
short AdcGetBasePrice(const short handle, AdcBasePrice *basePrice)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (basePrice)
	{
		scaleValues.type = AdcValueType::ADC_BASEPRICE;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*basePrice = scaleValues.ScaleValue.basePrice;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcGetDisplayText(const short handle, AdcDisplayText *displayText)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (displayText)
	{
		scaleValues.type = AdcValueType::ADC_DISPLAY_TEXT;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*displayText = scaleValues.ScaleValue.displayText;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcGetMaxDisplayTextChar(const short handle, short *maxDisplayTextChar)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (maxDisplayTextChar)
	{
		scaleValues.type = AdcValueType::ADC_MAX_DISPL_TEXT_CHAR;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*maxDisplayTextChar = scaleValues.ScaleValue.maxDisplTextChar;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}

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
short AdcGetScaleMode(const short handle, AdcScaleMode *scaleMode)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (scaleMode)
	{
		scaleValues.type = AdcValueType::ADC_SCALE_MODE;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*scaleMode = scaleValues.ScaleValue.scaleMode;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcGetEeprom(const short handle, AdcEeprom *eeprom)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (eeprom)
	{
		scaleValues.type = AdcValueType::ADC_EEPROM;
		scaleValues.ScaleValue.eeprom = *eeprom;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*eeprom = scaleValues.ScaleValue.eeprom;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcGetEepromSize(const short handle, AdcEepromSize *eepromSize)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (eepromSize)
	{
		scaleValues.type = AdcValueType::ADC_EEPROM_SIZE;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*eepromSize = scaleValues.ScaleValue.eepromSize;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcGetTiltCompensation(const short handle, AdcTiltComp *tiltCompensation)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (tiltCompensation)
	{
		scaleValues.type = AdcValueType::ADC_TILT_COMP;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*tiltCompensation = scaleValues.ScaleValue.tiltCompensation;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcGetFilter(const short handle, AdcFilter *filter)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (filter)
	{
		scaleValues.type = AdcValueType::ADC_FILTER;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*filter = scaleValues.ScaleValue.filter;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetCalStrings(const short handle, AdcCalStrings *calStrings)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (calStrings)
	{
		scaleValues.type = AdcValueType::ADC_CAL_STRINGS;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*calStrings = scaleValues.ScaleValue.calStrings;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetCalStrings4LoadCapacity(const short handle, const char *loadCapacity, AdcCalStrings *calStrings)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (calStrings)
	{
		retCode = ConvertLarsE2bizlarsE(lars->GetCalStrings4LoadCapacity(loadCapacity, calStrings));
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;

}


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
BIZLARS_API short AdcGetGFactor(const short handle, short *gFactor)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (gFactor)
	{
		scaleValues.type = AdcValueType::ADC_G_FACTOR;
		scaleValues.ScaleValue.gFactor = 0;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*gFactor = scaleValues.ScaleValue.gFactor;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetProdSettings(const short handle, AdcProdSettings *prodSettings)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (prodSettings)
	{
		scaleValues.type = AdcValueType::ADC_PRODUCTION_SETTINGS;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*prodSettings = scaleValues.ScaleValue.prodSettings;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetInterfaceMode(const short handle, AdcInterfaceMode *interfaceMode)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (interfaceMode)
	{
		scaleValues.type = AdcValueType::ADC_INTERFACE_MODE;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*interfaceMode = scaleValues.ScaleValue.interfaceMode;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetLoadCapacityUnit(const short handle, AdcWeightUnit *unit)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (unit)
	{
		scaleValues.type = AdcValueType::ADC_LOADCAPACITY_UNIT;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*unit = scaleValues.ScaleValue.loadCapacityUnit;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcGetZeroPointTracking - function to get the zero point tracking mode
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
BIZLARS_API short AdcGetZeroPointTracking(const short handle, short *mode)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (mode)
	{
		scaleValues.type = AdcValueType::ADC_ZERO_POINT_TRACKING;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*mode = scaleValues.ScaleValue.mode;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetZeroSettingInterval(const short handle, long *time)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (time)
	{
		scaleValues.type = AdcValueType::ADC_ZERO_SETTING_INTERVAL;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*time = scaleValues.ScaleValue.zeroSettingInterval;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetAutomaticZeroSettingTime(const short handle, long *time)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (time)
	{
		scaleValues.type = AdcValueType::ADC_AUTOMATIC_ZERO_SETTING_TIME;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*time = scaleValues.ScaleValue.automaticZeroSettingTime;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcGetVerificationParameter - function to get the state of the verification paramter
*
* @param    handle:in	adc handle
* @param    mode:in		0 not protected
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
BIZLARS_API short AdcGetVerificationParameter(const short handle, short *mode)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (mode)
	{
		scaleValues.type = AdcValueType::ADC_VERIF_PARAM_PROTECTED;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*mode = scaleValues.ScaleValue.verifParamProtected;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcGetUpdateMode - function to get the state of the update mode
*
* @param    handle:in	adc handle
* @param    mode:in		0 not allowed
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
BIZLARS_API short AdcGetUpdateMode(const short handle, short *mode)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (mode)
	{
		scaleValues.type = AdcValueType::ADC_UPDATE_ALLOWED;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*mode = scaleValues.ScaleValue.updateAllowed;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcGetWarmUpTime - function to get the warm up time in seconds
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
BIZLARS_API short AdcGetWarmUpTime(const short handle, long *time)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (time)
	{
		scaleValues.type = AdcValueType::ADC_WARM_UP_TIME;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*time = scaleValues.ScaleValue.warmUpTime;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcGetRemainingWarmUpTime - function to get the reamining warm up time in seconds
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
BIZLARS_API short AdcGetRemainingWarmUpTime(const short handle, long *time)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (time)
	{
		scaleValues.type = AdcValueType::ADC_REMAINING_WARM_UP_TIME;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*time = scaleValues.ScaleValue.warmUpTime;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetOperatingMode(const short handle, AdcOperatingMode *mode)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (mode)
	{
		scaleValues.type = AdcValueType::ADC_OPERATING_MODE;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*mode = scaleValues.ScaleValue.opMode;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetMaxCapacity(const short handle, AdcWeight *maxCapacity)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (maxCapacity)
	{
		scaleValues.type = AdcValueType::ADC_MAX_CAPACITY;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*maxCapacity = scaleValues.ScaleValue.maxCapacity;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcGetLoadCapacityParams - function to get the load capacity parameters
*
* @param    handle:in		adc handle
* @param    lcparams:out		load capacity params
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
BIZLARS_API short AdcGetLoadCapacityParams(const short handle, AdcLoadCapacityParams *lcparams)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (lcparams)
	{
		scaleValues.type = AdcValueType::ADC_LOAD_CAPACITY_PARAMS;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*lcparams = scaleValues.ScaleValue.loadCapacityParams;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetInitialZeroSetting(const short handle, AdcInitialZeroSettingParam *param)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (param)
	{
		scaleValues.type = AdcValueType::ADC_INITIAL_ZERO_SETTING;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*param = scaleValues.ScaleValue.initialZeroSettingParam;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetStateAutomaticTiltSensor(const short handle, bool *state)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (state)
	{
		scaleValues.type = AdcValueType::ADC_AUTOMATIC_TILT_SENSOR;
		retCode = ConvertLarsE2bizlarsE(lars->GetScaleValues(&scaleValues));
		*state = scaleValues.ScaleValue.state;
	}
	else
	{
		retCode = ADC_E_INVALID_PARAMETER;
	}

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcZeroScale(const short handle, AdcState *adcState)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    retCode = ConvertLarsE2bizlarsE(lars->ZeroScale(adcState));
    if (adcState) g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tadcState: 0x%llx  retCode: %d", __FUNCTION__, adcState->state, retCode);
    else g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


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
short AdcSetTare(const short handle, AdcState *adcState, const AdcTare *tare)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    retCode = ConvertLarsE2bizlarsE(lars->SetTare(adcState, tare));
	if (adcState) g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tadcState: 0x%llx  retCode: %d", __FUNCTION__, adcState->state, retCode);
    else g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


/**
******************************************************************************
* AdcGetTare - function to get tare limit weiged and known
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
BIZLARS_API short AdcGetTare(const short handle, AdcState *adcState, AdcTare *tare)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	retCode = ConvertLarsE2bizlarsE(lars->GetTare(adcState, tare));
	if (adcState) g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tadcState: 0x%llx  retCode: %d", __FUNCTION__, adcState->state, retCode);
	else g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcSetTarePriority(const short handle, AdcState *adcState, const AdcTarePriority prio)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    retCode = ConvertLarsE2bizlarsE(lars->SetTarePriority(adcState, prio));
	if (adcState) g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tadcState: 0x%llx  retCode: %d", __FUNCTION__, adcState->state, retCode);
    else g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}

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
short AdcClearTare(const short handle, AdcState *adcState)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }
    
    retCode = ConvertLarsE2bizlarsE(lars->ClearTare(adcState));
	if (adcState) g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tadcState: 0x%llx  retCode: %d", __FUNCTION__, adcState->state, retCode);
    else g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


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
short AdcReadWeight(const short handle, const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcTare *tare, AdcBasePrice *basePrice, AdcPrice *sellPrice)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    retCode = ConvertLarsE2bizlarsE(lars->ReadWeight(registrationRequest, adcState, weight, tare, basePrice, sellPrice));
	if (adcState) g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tadcState: 0x%llx  retCode: %d", __FUNCTION__, adcState->state, retCode);
    else g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


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
short AdcGetCapability(const short handle, const AdcCapabilities cap, unsigned char *value)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    retCode = ConvertLarsE2bizlarsE(lars->GetCapability(cap, value));
    if (retCode != ADC_SUCCESS)
    {
        *value = 0;
    }

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}



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
short AdcGetLogger(const short handle, const short index, char *entry, unsigned long *size)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    retCode = ConvertLarsE2bizlarsE(lars->GetLogger(index, entry, size));
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


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
short AdcGetRandomAuthentication(const short handle, unsigned char *random, unsigned long *size)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    retCode = ConvertLarsE2bizlarsE(lars->GetRandomAuthentication(random, size));
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


/**
******************************************************************************
* AdcSetAuthentication - function do to the authentication
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
short AdcSetAuthentication(const short handle, const AdcAuthentication *authentication)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    retCode = ConvertLarsE2bizlarsE(lars->SetAuthentication(authentication));
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


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
short AdcSetCountryFilesPath(const short handle, const char *path)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	lars->SetCountryFilesPath(path);
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetSupportedCountries(const short handle, char *countries, unsigned long *size)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	retCode = ConvertLarsE2bizlarsE(lars->GetSupportedCountries(countries, size));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcSetCountry(const short handle, const char *name)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    retCode = ConvertLarsE2bizlarsE(lars->SetCountry(name));
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


/**
******************************************************************************
* AdcGetCountry - function to get the adc country
*
* @param    name:out		country name
* @param    size:in		    size of variable name
*
* @return   ADC_SUCCESS
*           ADC_E_INVALID_HANDLE
*           ADC_E_NOT_ENOUGH_MEMORY
* @remarks
******************************************************************************
*/
BIZLARS_API short AdcGetCountry(const short handle, char *name, unsigned long size)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    retCode = ConvertLarsE2bizlarsE(lars->GetCountry(name, size));
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


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
*           ADC_E_NOT_ENOUGH_MEMORY
* @remarks
******************************************************************************
*/
BIZLARS_API short AdcGetCompatibleLoadCapacities(const short handle, const char *country, char *loadCapacities, unsigned long *size)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    retCode = ConvertLarsE2bizlarsE(lars->GetCompatibleLoadCapacities(country, loadCapacities, size));
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


/**
******************************************************************************
* AdcSetLoadCapacity - function to set one load capacity
*
* @param    loadCapacity:in loadcapacity
*
* @return   ADC_SUCCESS
*           ADC_E_INVALID_HANDLE
*           ADC_E_INVALID_PARAMETER
*           ADC_E_FILE_CORRUPT
*           ADC_E_FILE_NOT_FOUND
*			ADC_E_ADC_TIMEOUT
*			ADC_E_COMMAND_NOT_EXECUTED
*			ADC_E_FUNCTION_NOT_IMPLEMENTED
*			ADC_E_FILE_NOT_FOUND
* @remarks
******************************************************************************
*/
BIZLARS_API short AdcSetLoadCapacity(const short handle, const char *loadCapacity)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    retCode = ConvertLarsE2bizlarsE(lars->SetLoadCapacity(loadCapacity));
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


/**
******************************************************************************
* AdcGetLoadCapacity - function to get the current load capacity
*
* @param    loadCapacity:in loadcapacity
* @param    size:in		    size of variable loadcapacity
*
* @return    ADC_SUCCESS
*           ADC_E_INVALID_HANDLE
*           ADC_E_NOT_ENOUGH_MEMORY
******************************************************************************
*/
BIZLARS_API short AdcGetLoadCapacity(const short handle, char *loadCapacity, unsigned long size)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	retCode = ConvertLarsE2bizlarsE(lars->GetLoadCapacity(loadCapacity, size));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcGetHighResolution(const short handle, const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcWeight *weightHighResolution, AdcTare *tare, long *digitValue)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

	retCode = ConvertLarsE2bizlarsE(lars->GetHighResolution(registrationRequest, adcState, weight, weightHighResolution, tare, digitValue));
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


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
short AdcGetGrossWeight(const short handle, AdcState *adcState, AdcWeight *grossWeight, AdcWeight *grossWeightHighResolution, long *digitValue)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	retCode = ConvertLarsE2bizlarsE(lars->GetGrossWeight(adcState, grossWeight, grossWeightHighResolution, digitValue));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
*           ADC_E_INVALID_PARAMETER
*			ADC_E_ADC_TIMEOUT
*			ADC_E_COMMAND_NOT_EXECUTED
*			ADC_E_FUNCTION_NOT_IMPLEMENTED* @remarks
* @remarks
******************************************************************************
*/
BIZLARS_API short AdcGetVersion(const short handle, char *versionStr, unsigned long *size)
{
    short           retCode = LarsErr::E_SUCCESS;
    Lars            *lars;
    stringstream    versionStream;
   
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

    if (!size)
    {
        return ADC_E_INVALID_PARAMETER;
    }

    retCode = ConvertLarsE2bizlarsE(lars->GetVersion(versionStr, size));
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


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
short AdcGetFirstDiagnosticData(const short handle, const char *name, long *sensorHealthID, AdcSensorHealth *sensorHealth, short *state)
{
    short   retCode = LarsErr::E_SUCCESS;
    Lars    *lars;

    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

    if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
    {
        g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
        return ADC_E_INVALID_HANDLE;
    }

	retCode = ConvertLarsE2bizlarsE(lars->GetFirstDiagnosticData(name, sensorHealthID, sensorHealth, state));
    g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
    return retCode;
}


/**
******************************************************************************
* AdcGetNextDiagnosticData - function to get diagnostic data from the adc
*
* @param    handle:in			adc handle
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
short AdcGetNextDiagnosticData(const short handle, const long sensorHealthID, AdcSensorHealth *sensorHealth, short *state)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	lars->GetNextDiagnosticData(sensorHealthID, sensorHealth, state);
	retCode = ConvertLarsE2bizlarsE(retCode);
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcConfigureDiagnosticData - function to configure adc diagnostic data (threshold,...)
*
* @param    handle:in		adc handle
* @param    sensorHealth:in	sensor health data
*
* @return   ADC_SUCCESS
*			ADC_E_INVALID_HANDLE
* @remarks
******************************************************************************
*/
BIZLARS_API short AdcConfigureDiagnosticData(const short handle, const AdcSensorHealth *sensorHealth)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	retCode = ConvertLarsE2bizlarsE(lars->ConfigureDiagnosticData(sensorHealth));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcSetFirmwarePath - function to set the path to the adc firmware files
*
* @param    handle:in	adc handle
* @param    path:in		path to the firmware files
*
* @return	ADC_SUCCESS
*			ADC_E_INVALID_HANDLE
* @remarks
******************************************************************************
*/
short AdcSetFirmwarePath(const short handle, const char *path)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	lars->SetFirmwarePath(path);
	retCode = ConvertLarsE2bizlarsE(retCode);
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}

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
*			ADC_E_ADC_TIMEOUT
*			ADC_E_COMMAND_NOT_EXECUTED
*			ADC_E_FUNCTION_NOT_IMPLEMENTED
* @remarks
******************************************************************************
*/
BIZLARS_API short AdcUpdate(const short handle, const short force)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	retCode = ConvertLarsE2bizlarsE(lars->Update(force));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
BIZLARS_API short AdcGetFirmwareFileVersion(const short handle, char *versionStr, unsigned long *size)
{
	short           retCode = LarsErr::E_SUCCESS;
	Lars            *lars;
	stringstream    versionStream;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	if (!size)
	{
		return ADC_E_INVALID_PARAMETER;
	}

	retCode = ConvertLarsE2bizlarsE(lars->GetFirmwareFileVersion(versionStr, size));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcCalibration - function for scale calibration
*
* @param    handle:in		adc handle
* @param    cmd:in			calibration command
* @param    adcState:out    state of the adc
* @param    step:out		calibration step
* @param    calibDigit:out	weight in digits in calibration mode
*
* @return	ADC_SUCCESS
*			ADC_E_USB_ERROR
*			ADC_E_INVALID_HANDLE
*			ADC_E_ADC_TIMEOUT
*			ADC_E_COMMAND_NOT_EXECUTED
*			ADC_E_FUNCTION_NOT_IMPLEMENTED* @remarks
* @remarks
******************************************************************************
*/
short AdcCalibration(const short handle, const AdcCalibCmd cmd, AdcState *adcState, long *step, long *calibDigit)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	retCode = ConvertLarsE2bizlarsE(lars->Calibration(cmd, adcState, step, calibDigit));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcParameters(const short handle, const AdcParamMode mode)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	retCode = ConvertLarsE2bizlarsE(lars->Parameters(mode));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
void AdcSetTrace(const char *fileName, const short level)
{
	g_adcTrace.SetConfig(fileName, level);
	return;
}


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
BIZLARS_API short AdcTiltAngleConfirmed(const short handle, const AdcTiltAngleConfirmCmd cmd)
{
	short   retCode = LarsErr::E_SUCCESS;
	AdcScaleValues scaleValues;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	scaleValues.type = AdcValueType::ADC_TILT_ANGLE_CONFIRM;
	scaleValues.ScaleValue.tiltAngleConfirmCmd = cmd;
	retCode = ConvertLarsE2bizlarsE(lars->SetScaleValues(&scaleValues));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcGetInternalDataEx - function to get the internal data of the adc, for
*					     example n, l, M ...
*
* @param    handle:in			adc handle
* @param    internalData:in/out	pointer of array internal data
* @param sendRequestCmd:in		send the request command (supported only for special adw versions !!)
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
short AdcGetInternalDataEx(const short handle, AdcInternalDataEx *internalDataEx, bool sendRequestCmd)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	retCode = ConvertLarsE2bizlarsE(lars->GetInternalDataEx(internalDataEx, sendRequestCmd));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcGetInternalData(const short handle, AdcInternalData *internalData)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;
	AdcInternalDataEx *internalDataEx;
	short	idx = 0;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	// get array number
	while (internalData[idx].ID != 0) idx++;

	// allocate structure
	internalDataEx = new AdcInternalDataEx[idx + 1];

	if (internalDataEx)
	{
		// copy ID from the internalData to internalDataEx
		idx = 0;
		while (internalData[idx].ID != 0)
		{
			internalDataEx[idx].ID = internalData[idx].ID;
			idx++;
		}
		internalDataEx[idx].ID = 0;		// end identifier

		retCode = ConvertLarsE2bizlarsE(lars->GetInternalDataEx(internalDataEx));

		// copy value of the internalDataEx to internalData
		idx = 0;
		while (internalData[idx].ID != 0)
		{
			internalData[idx].value = internalDataEx[idx].value;
			if (internalDataEx[idx].valid == false)
			{
				if (retCode == ADC_SUCCESS) retCode = ADC_E_PROTOCOL_ERROR;			// set protocol error because of compatibility
			}
			idx++;
		}

		delete[] internalDataEx;
	}
	else
	{
		retCode = ADC_E_NOT_ENOUGH_MEMORY;
	}
	
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}

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
short AdcSetScaleModel(const short handle, const char *model)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	retCode = ConvertLarsE2bizlarsE(lars->SetScaleModel(model));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


/**
******************************************************************************
* AdcGetScaleModel - function to get the scale model
*
* @param    handle:in	adc handle
* @param    model:in	scale model
* @param    size:in		size of variable model
*
* @return	ADC_SUCCESS
*			ADC_E_INVALID_HANDLE
* @remarks
******************************************************************************
*/
short AdcGetScaleModel(const short handle, char *model, unsigned long size)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	retCode = ConvertLarsE2bizlarsE(lars->GetScaleModel(model, size));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}


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
short AdcGetPortNr(const short handle, char *portNr, unsigned long *size)
{
	short   retCode = LarsErr::E_SUCCESS;
	Lars    *lars;

	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tstart hdl: 0x%x", __FUNCTION__, handle);

	if ((lars = AdcCheckHandle(g_larsList, handle)) == NULL)
	{
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\tend retCode: %d", __FUNCTION__, ADC_E_INVALID_HANDLE);
		return ADC_E_INVALID_HANDLE;
	}

	retCode = ConvertLarsE2bizlarsE(lars->GetPortNr(portNr, size));
	g_adcTrace.Trace(AdcTrace::TRC_ACTION, "%s\tend retCode: %d", __FUNCTION__, retCode);
	return retCode;
}

/**
******************************************************************************
* internal functions
******************************************************************************
*/


/**
******************************************************************************
* CheckIfAdcIsAlreadyOpen - function checks if adc is already open
*
* @param    larsList:in		list of all already open adc
* @param    lars:in			current adc
*
* @return	false		adc is not already open
*			true		adc is already open
* @remarks
******************************************************************************
*/
bool CheckIfAdcIsAlreadyOpen(list <Lars *> &larsList, Lars *lars)
{
	Lars *tmpLars;

	g_larsMutex.lock();

    LarsList::const_iterator    it;
	for (it = larsList.begin(); it != larsList.end(); ++it)
	{
		tmpLars = *it;
        if (*(tmpLars->GetAdcName()) == *(lars->GetAdcName()))
        {
            g_larsMutex.unlock();
            return true;
        }
	}

	g_larsMutex.unlock();

	return false;
}


/**
******************************************************************************
* CreateLogicalDeviceHandle - function to create the logical device handle
*
* @param    larsList:in		list of all already open adc
* @param    lars:in			current adc
*
* @return
* @remarks
******************************************************************************
*/
short CreateLogicalDeviceHandle(LarsList &larsList)
{
    Lars                            *tmpLars;
    LarsList::const_iterator        it;
    short                           random;
    bool                            alreadyCreated = true;

    /* initialize random seed: */
    srand((unsigned int)time(NULL));

    while (alreadyCreated)
    {
        alreadyCreated = false;

        // generate a random from 1 - 65000
        random = rand() % 65000 + 1;

        g_larsMutex.lock();
        for (it = larsList.begin(); it != larsList.end(); ++it)
        {
            tmpLars = *it;
            if (tmpLars->GetHandle() == random)
            {
                alreadyCreated = true;
                continue;
            }
        }
        g_larsMutex.unlock();
    }

    return random;
}


/**
******************************************************************************
* AdcCheckHandle - function check adv handle
*
* @param    handle:in		logical adc handle
*
* @return   lars object from list
* @remarks
******************************************************************************
*/
Lars *AdcCheckHandle(LarsList &larsList, const short handle)
{
    Lars                            *lars = NULL;
    LarsList::const_iterator        it;

    g_larsMutex.lock();
    for (it = larsList.begin(); it != larsList.end(); ++it)
    {
        lars = *it;
        if (lars->GetHandle() == handle)
        {
            g_larsMutex.unlock();
            return lars;
        }
    }
    g_larsMutex.unlock();
    return NULL;
}

/**
******************************************************************************
* ConvertLarsE2bizlarsE - convert error Code
*
* @param    errorCode:in    error code from class Lars
*
* @return   errorCode for bizlars
* @remarks
******************************************************************************
*/
short ConvertLarsE2bizlarsE(short errorCode)
{
    short retCode;

    switch (errorCode)
    {
    case LarsErr::E_SUCCESS:
        retCode = ADC_SUCCESS;
        break;

	case LarsErr::E_INVALID_HANDLE:
		retCode = ADC_E_INVALID_HANDLE;
		break;

    case LarsErr::E_NO_DEVICE:
        retCode = ADC_E_NO_DEVICE;
        break;

	case LarsErr::E_INVALID_PARAMETER:
		retCode = ADC_E_INVALID_PARAMETER;
		break;

    case LarsErr::E_NOT_ENOUGH_MEMORY:
        retCode = ADC_E_NOT_ENOUGH_MEMORY;
        break;

    case LarsErr::E_COUNTRY_NOT_SUPPORTED:
        retCode = ADC_E_COUNTRY_NOT_SUPPORTED;
        break;

    case LarsErr::E_FILE_CORRUPT:
        retCode = ADC_E_FILE_CORRUPT;
        break;

    case LarsErr::E_FILE_NOT_FOUND:
        retCode = ADC_E_FILE_NOT_FOUND;
        break;

	case LarsErr::E_PROTOCOL:
		retCode = ADC_E_PROTOCOL_ERROR;
		break;

	case LarsErr::E_PROTOCOL_CRC:
		retCode = ADC_E_PROTOCOL_CRC_ERROR;
		break;

	case LarsErr::E_KEY_NOT_FOUND:
		retCode = ADC_E_PROTOCOL_ERROR;
		break;

	case LarsErr::E_ADC_TIMEOUT:
		retCode = ADC_E_ADC_TIMEOUT;
		break;

	case LarsErr::E_EEPROM_ACCESS_VIOLATION:
		retCode = ADC_E_EEPROM_ACCESS_VIOLATION;
		break;

	case LarsErr::E_SENSOR_HEALTH_QUEUE_FULL:
		retCode = ADC_E_SENSOR_HEALTH_QUEUE_FULL;
		break;

	case LarsErr::E_LOAD_CAPACITY_NOT_SUPPORTED:
		retCode = ADC_E_LOAD_CAPACITY_NOT_SUPPORTED;
		break;

	case LarsErr::E_COMMAND_NOT_EXECUTED:
		retCode = ADC_E_COMMAND_NOT_EXECUTED;
		break;

	case LarsErr::E_FUNCTION_NOT_IMPLEMENTED:
		retCode = ADC_E_FUNCTION_NOT_IMPLEMENTED;
		break;

	case LarsErr::E_AUTHENTICATION:
		retCode = ADC_E_AUTHENTICATION;
		break;

	case LarsErr::E_COMMAND_NOT_SUPPORTED_IN_THIS_OPERATING_MODE:
		retCode = ADC_E_FUNCTION_NOT_SUPPORTED_IN_THIS_OPERATING_MODE;
		break;

	case LarsErr::E_LOGBOOK_NO_FURTHER_ENTRY:
		retCode = ADC_E_LOGBOOK_NO_FURTHER_ENTRY;
		break;

	case LarsErr::E_LOGBOOK_FULL:
		retCode = ADC_E_LOGBOOK_FULL;
		break;

	case LarsErr::E_TILT_COMPENSATION_SWITCH_ON:
		retCode = ADC_E_TILT_COMPENSATION_SWITCH_ON;
		break;

	case LarsErr::E_CHKSUM_BLOCK_3_4:
		retCode = ADC_E_CHKSUM_BLOCK_3_4;
		break;

	case LarsErr::E_LINEAR_CALIBRATION:
		retCode = ADC_E_CALIBRATION;
		break;

	case LarsErr::E_COMMAND_ONLY_FOR_SEPARATE_LOADCELL:
		retCode = ADC_E_COMMAND_ONLY_FOR_SEPARATE_LOADCELL;
		break;

	case LarsErr::E_UPDATE_NOT_ALLOWED:
		retCode = ADC_E_UPDATE_NOT_ALLOWED;
		break;

	case LarsErr::E_TCC_SWITCH_ON:
		retCode = ADC_E_TCC_SWITCH_ON;
		break;

	case LarsErr::E_INCOMPATIBLE_PROD_DATA:
		retCode = ADC_E_INCOMPATIBLE_PROD_DATA;
		break;

	case LarsErr::E_TARE_OCCUPIED:
		retCode = ADC_E_TARE_OCCUPIED;
		break;

	case LarsErr::E_KNOWN_TARE_LESS_E:
		retCode = ADC_E_KNOWN_TARE_LESS_E;
		break;

	case LarsErr::E_BATCH_TARE_NOT_ALLOWED:
		retCode = ADC_E_BATCH_TARE_NOT_ALLOWED;
		break;

	case LarsErr::E_TARE_OUT_OF_RANGE:
		retCode = ADC_E_TARE_OUT_OF_RANGE;
		break;

	case LarsErr::E_TARE_OUTSIDE_CLEARING_AREA:
		retCode = ADC_E_TARE_OUTSIDE_CLEARING_AREA;
		break;

	case LarsErr::E_WS_NOT_SUPPORT_LOAD_CAPACITY:
		retCode = ADC_E_WS_NOT_SUPPORT_LOAD_CAPACITY;
		break;

	case LarsErr::E_CHKSUM_FIRMWARE:
		retCode = ADC_E_CHKSUM_FIRMWARE;
		break;

    default:
        retCode = ADC_E_ADC_FAILURE;
    }

    return retCode;
}

