/**
******************************************************************************
* File       : lars.cpp
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : lars class
******************************************************************************
*/
#include <sstream>
#include <thread>         // std::this_thread::sleep_for
#include "version.h"
#ifdef  __GNUC__
#include <string.h>
#ifdef USE_LIBUSB
//#include <libusb-1.0/libusb.h>
#include <libusb/libusb.h>
#endif	// USE_LIBUSB
#endif	// __GNUC__
#include "larsErr.h"
#include "lars.h"
#include "adcusb.h"
#include "adcserial.h"
#include "adcrbs.h"
#include "authentication.h"
#include "helpers.h"
#include "adcfirmware.h"
#include "adcssp.h"


// defines for authentication
#define ADW_AUTH_CRC_START      0x4461      /* start value for the Polynomial */
#define ADW_AUTH_CRC_POLY       0x1021      /* Polynomial-coeffizient */
#define ADW_AUTH_CRC_MASK       0x0000      /* Mask for the Polynomial */

#define EEPROM_MAX_READ_WRITE_LENGTH	32	// max bytes for read/write access to eeprom


// define to set the lcs-State to authentication
#define LCS_AUTHENTICATION_MASK	0x00008340	// mask for bits needAuthentication, needLoogbook and calibMode

/**
******************************************************************************
* initialize non integral const data members
******************************************************************************
*/
const map<unsigned short, string> Lars::m_productID2adcType = { { 0x3200, "ADC505" },
                                                                { 0x3201, "ADC505" } };

/**
******************************************************************************
* Standardconstructor
*
* @param
*
* @return
* @remarks
******************************************************************************
*/
Lars::Lars()  
{
	m_adcName.clear();
	m_protocolType.clear();
	m_port.clear();

    Init();
}


/**
******************************************************************************
* Standardconstructor - overloaded with adcName
*
* @param
*
* @return   
* @remarks
******************************************************************************
*/
Lars::Lars(const char *adcName, const char *protocol, const char *port)
{
	if (adcName) m_adcName = adcName;
	else m_adcName.clear();

	if (protocol) m_protocolType = protocol;
	else m_protocolType.clear();
	
	if (port) m_port = port;
	else m_port.clear();

    Init();
}


/**
******************************************************************************
* Init - init all member variables
*
* @param
*
* @return   void
* @remarks
******************************************************************************
*/
void Lars::Init()
{
    m_adcType.clear();
	m_wsType.clear();
    m_adcVersion = "0.00";
    m_logicalHandle = 0;
    m_maxDisplayTextChars = 0;

	m_tilt = 0;
	
	if (m_port.empty() || m_port == "usb")
		m_interface = new AdcUsb();

	if (m_port.substr(0, 3) == "COM")
		m_interface = new AdcSerial(m_port);

    m_protocol = new AdcRbs(m_interface);

	InitCapabilities();

	m_applAuthenticationDone = false;

	m_firmwarePath.clear();
	m_sspPath.clear();
	m_scaleModel.clear();

   return;
}


/**
******************************************************************************
* Lars - copy constructor
*
* @param
*
* @return
* @remarks
******************************************************************************
*/
Lars::Lars(const Lars &obj)                             // copy constructor
{
	m_adcName = obj.m_adcName;
	m_adcType = obj.m_adcType;
	m_wsType = obj.m_wsType;
	m_adcVersion = obj.m_adcVersion;
	m_logicalHandle = obj.m_logicalHandle;
	m_maxDisplayTextChars = obj.m_maxDisplayTextChars;

	m_interface = new AdcUsb();
	m_protocol = new AdcRbs();

	*m_interface = *obj.m_interface;
	*m_protocol = *obj.m_protocol;
	m_protocol->SetInterface(m_interface);

	m_adcCap = obj.m_adcCap;

	m_firmwarePath = obj.m_firmwarePath;
	m_sspPath = obj.m_sspPath;
	m_scaleModel = obj.m_scaleModel;
	m_ssp = obj.m_ssp;
	m_tilt = obj.m_tilt;
}


/**
******************************************************************************
* ~Lars - destructor
*
* @param
*
* @return
* @remarks
******************************************************************************
*/
Lars::~Lars()
{
	m_interface->Close();

	if (m_tilt)
	{
		delete m_tilt;
		m_tilt = NULL;
	}
	if (m_protocol)
	{
		delete m_protocol;
		m_protocol = NULL;
	}
	if (m_interface)
	{
		delete m_interface;
		m_interface = NULL;
	}
}


/**
******************************************************************************
* GetAdcName - getter for adcName
*
* @param
*
* @return	pointer to adcName
* @remarks
******************************************************************************
*/
string* Lars::GetAdcName()
{
	return &m_adcName;
}


/**
******************************************************************************
* Open - Open the connection the Bizerba weighing system
*
* @param
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::Open(const bool performSwReset)
{
	short errorCode;

	m_mutex.lock();

	if ((m_interface == NULL) || (m_protocol == NULL))
	{
		m_mutex.unlock();
		return LarsErr::E_INVALID_PARAMETER;
	}

    if (!m_interface->Open(m_adcName))
    {
        m_mutex.unlock();
        return LarsErr::E_NO_DEVICE;
    }

	if (performSwReset)
	{
		// send software reset
		if ((errorCode = m_protocol->Reset(AdcResetType::ADC_SOFTWARE_RESET)) != LarsErr::E_SUCCESS)
		{
			m_mutex.unlock();
			// error -> cancel
			Close();
			return errorCode;
		}
	}

	// read all necessary information from adc
	if ((errorCode = SetAdcVariables()) != LarsErr::E_SUCCESS)
	{
		m_mutex.unlock();
		// error -> cancel
		Close();
		return errorCode;
	}

    m_mutex.unlock();

    return LarsErr::E_SUCCESS;
}

/**
******************************************************************************
* Close - close the connection the Bizerba weighing system
*
* @param
*
* @return
* @remarks
******************************************************************************
*/
bool Lars::Close()
{
	// connection to adc is closing, call store parameter to ensure that the adc parameters are storing persistent before shutdown
	Parameters(ADC_SAVE_SENSOR_HEALTH_DATA);

    return (m_interface->Close());
}


/**
******************************************************************************
* SetAdcVariables - set the library adc variables
*
* @param
*
* @return   handle
* @remarks
******************************************************************************
*/
short Lars::SetAdcVariables()
{
	short errorCode;

	// get protocol version to set adc type
	map<string, string> versionMap;
	if ((errorCode = m_protocol->GetVersion(versionMap)) != LarsErr::E_SUCCESS)
	{
		return errorCode;
	}
	SetAdcType(versionMap);
	SetWsType(versionMap);
	SetBootLoaderVersion(versionMap);

	// get operating mode
	if ((errorCode = m_protocol->GetOperatingMode(&m_opMode)) != LarsErr::E_SUCCESS)
	{
		return errorCode;
	}

	if (m_opMode == ADC_APPLICATION)
	{
		// read country specific settings
		map<string, string> countrySettings;
		if ((errorCode = m_protocol->GetCountrySettings(countrySettings)) != LarsErr::E_SUCCESS)
		{
			return errorCode;
		}
		else
		{
			m_cySetting.SetSettings(countrySettings);
		}

		// read load capacity settings
		if ((errorCode = ReadLcSettings()) != LarsErr::E_SUCCESS)
		{
			return errorCode;
		}

		// read capabilities from ADC
		InitCapabilities();
		if ((errorCode = m_protocol->GetCapabilities(m_adcCap)) != LarsErr::E_SUCCESS)
		{
			return errorCode;
		}

		// get eeprom size
		if ((errorCode = m_protocol->GetEepromSize(&m_eepromSize)) != LarsErr::E_SUCCESS)
		{
			return errorCode;
		}


		// check if authentication is necessary and do it
		if ((errorCode = CheckDoAuthentication(true)) != LarsErr::E_SUCCESS)
		{
			return errorCode;
		}

		// get tilt sensor resolution
		AdcScaleValues scaleValues;
		scaleValues.type = AdcValueType::ADC_TILT_COMP;
		if (m_protocol->GetTiltCompensation(&scaleValues.ScaleValue.tiltCompensation) == LarsErr::E_SUCCESS)
		{
			if (m_tilt) delete m_tilt;
			m_tilt = new AdcTilt(&scaleValues.ScaleValue.tiltCompensation);
			m_cySetting.SetTilt(m_tilt);
			m_ssp.SetTilt(m_tilt);
		}

		// read scale model
		// don't check error code, because older firmware version does not support the api
		m_protocol->GetScaleModel(m_scaleModel);

		// read ssp files
		LoadSspParam();
	}
	return errorCode;
}

/**
******************************************************************************
* ReadLcSettings - read the load capacity settings from adc
*
* @param
*
* @return   handle
* @remarks
******************************************************************************
*/
short Lars::ReadLcSettings()
{
	short errorCode;

	// read load capacity settings
	map<string, string> lcSettings;
	if ((errorCode = m_protocol->GetLoadCapacity(lcSettings)) == LarsErr::E_SUCCESS)
	{
		m_lc.SetSettings(lcSettings);
		m_lc.SetCalTemplateStrings(m_cySetting.MaxTemplateString(), m_cySetting.MinTemplateString(), m_cySetting.eTemplateString());
	}

	return errorCode;
}

/**
******************************************************************************
* GetHandle - return the logical handle for the application
*
* @param
*
* @return   handle
* @remarks
******************************************************************************
*/
short Lars::GetHandle()
{
	return m_logicalHandle;
}

/**
******************************************************************************
* SetHandle - set the logical handle for the application
*
* @param
*
* @return   void
* @remarks
******************************************************************************
*/
void Lars::SetHandle(short handle)
{
    m_logicalHandle = handle;
}

/**
******************************************************************************
* operator == - compare class
*
* @param
*
* @return
* @remarks
******************************************************************************
*/
bool Lars::operator == (const Lars &lars)
{
    return (lars.m_logicalHandle == m_logicalHandle);
}

/**
******************************************************************************
* GetCapability - get the capability from the adc
*
* @param    cap:in      capability
* @param    value:out   0: not supported
*                       1: supported
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::GetCapability(AdcCapabilities cap, unsigned char *value)
{
    short retCode; 

    m_mutex.lock();

    AdcCap::const_iterator it;
    it = m_adcCap.find(cap);
    if (it != m_adcCap.end())
    {
        *value = (unsigned char)((*it).second);
        retCode = LarsErr::E_SUCCESS;
    }
    else
    {
        *value = 0;
        retCode = LarsErr::E_INVALID_PARAMETER;
    }

    m_mutex.unlock();

    return retCode;
}



/**
******************************************************************************
* GetVersion - get the version of all the components
*
* @param    versionStr:out  version of the components as key value pairs
* @param    size:in/out     size fo versionStr
*
* @return	errorCode
* @remarks
******************************************************************************
*/
short Lars::GetVersion(char *versionStr, unsigned long *size)
{
    short               errorCode = LarsErr::E_SUCCESS;
    map<string, string> versionMap;
    stringstream        versionStream;
	unsigned short		driverMajor;
	unsigned short		driverMinor;

	m_mutex.lock();

    // library version
    versionStream << STX << APPLICATION_NAME << FS << APPLICATION_VERSION_STRING;

	// driver version
	if (m_interface->DriverVersion(&driverMajor, &driverMinor))
	{
		char driverVersion[255];
		sprintf(&driverVersion[0], "%d.%02d.0000", driverMajor, driverMinor);
		versionStream << GS << ADC_DRIVER_NAME << FS << driverVersion;
	}

	if (errorCode == LarsErr::E_SUCCESS)
	{
		errorCode = m_protocol->GetVersion(versionMap);
		if (versionMap.size())
		{
			for (map<string, string>::iterator it = versionMap.begin(); it != versionMap.end(); ++it)
			{
				versionStream << GS << (*it).first << FS << (*it).second;
			}
		}
	}

	versionStream << ETX;

    if ((errorCode == LarsErr::E_SUCCESS) ||
        (errorCode == LarsErr::E_NO_DEVICE))
    {
		if ((versionStr) && (size) && (*size != 0))
        {
            // don't forget the terminating \0 
            if (*size >= versionStream.str().size() + 1)
            {
                // don't forget the terminating \0 
                *size = versionStream.str().size() + 1;
                strcpy(versionStr, versionStream.str().c_str());

                errorCode = LarsErr::E_SUCCESS;
            }
            else
            {
                *size = 0;
                errorCode = LarsErr::E_NOT_ENOUGH_MEMORY;
            }
        }
        else if (size)
        {
            *size = versionStream.str().size() + 1;
            errorCode = LarsErr::E_SUCCESS;
        }
    }

	m_mutex.unlock();

    return errorCode;
}


/**
******************************************************************************
* SetScaleValues - set scale values
*
* @param    scaleValues:in  pointer to structure to set the specific scale value
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::SetScaleValues(const AdcScaleValues *scaleValues)
{
    short errorCode, errorCode1;
	long  eepromSize;

    m_mutex.lock();

    switch (scaleValues->type)
    {
		case AdcValueType::ADC_BASEPRICE:
        {
            errorCode = m_protocol->SetBaseprice(&scaleValues->ScaleValue.basePrice, scaleValues->frozen);
            break;
        }

		case AdcValueType::ADC_DISPLAY_TEXT:
        {
            errorCode = m_protocol->SetDisplayText(&scaleValues->ScaleValue.displayText);
            break;
        }

		case AdcValueType::ADC_SCALE_MODE:
        {
            errorCode = m_protocol->SetScaleMode(scaleValues->ScaleValue.scaleMode);
            break;
        }

		case AdcValueType::ADC_EEPROM:
        {
            AdcEeprom	eeprom;
			short		len;
			short		offset;

			eepromSize = 0;
			if (scaleValues->ScaleValue.eeprom.region == AdcEepromRegion::ADC_WELMEC_REGION)
				eepromSize = m_eepromSize.welmecRegion;
			else if (scaleValues->ScaleValue.eeprom.region == AdcEepromRegion::ADC_OPEN_REGION)
				eepromSize = m_eepromSize.openRegion;
			else if (scaleValues->ScaleValue.eeprom.region == AdcEepromRegion::ADC_PROD_REGION)
				eepromSize = m_eepromSize.prodRegion;
			else if (scaleValues->ScaleValue.eeprom.region == AdcEepromRegion::ADC_PROD_SENSORS_REGION)
				eepromSize = m_eepromSize.prodSensorsRegion;

			if ((scaleValues->ScaleValue.eeprom.startAdr + scaleValues->ScaleValue.eeprom.len) <= eepromSize)
			{
				eeprom.direction = 1;
				eeprom.region = scaleValues->ScaleValue.eeprom.region;

				offset = 0;
				len = scaleValues->ScaleValue.eeprom.len;
				
				do
				{
					eeprom.startAdr = scaleValues->ScaleValue.eeprom.startAdr + offset;

					// check for max write length
					if (len > EEPROM_MAX_READ_WRITE_LENGTH)
						eeprom.len = EEPROM_MAX_READ_WRITE_LENGTH;
					else
						eeprom.len = len;

					// copy bytes
					for (short idx = 0; idx < eeprom.len; idx++)
						eeprom.data[idx] = scaleValues->ScaleValue.eeprom.data[offset + idx];
					
					errorCode = m_protocol->SetEeprom(&eeprom);

					offset += eeprom.len;
					len -= eeprom.len;

				} while (len && (errorCode == LarsErr::E_SUCCESS));
			}
			else
			{
				errorCode = LarsErr::E_EEPROM_ACCESS_VIOLATION;
			}
            break;
        }

		case AdcValueType::ADC_TILT_COMP:
        {
            errorCode = m_protocol->SetTiltCompensation(&scaleValues->ScaleValue.tiltCompensation, NULL, NULL, NULL, NULL);
            break;
        }

		case AdcValueType::ADC_TILT_ANGLE_CONFIRM:
		{
			errorCode = m_protocol->SetTiltAngleConfirm(scaleValues->ScaleValue.tiltAngleConfirmCmd);
			break;
		}

		case AdcValueType::ADC_SCALE_SPECIFIC_PARAM_SEALED:
		{
			errorCode = m_protocol->SetTiltCompensation(NULL, m_ssp.GetTccSettings(), m_ssp.GetLinSettings(), m_ssp.GetWdtaSettings(m_lc.GetMultiInterval(), m_lc.GetNumberOfIntervalsE(), m_lc.GetMaxCapacityKG()), NULL);
			errorCode1 = m_protocol->SetScaleSpecificSettingsGeneral(NULL, m_ssp.GetGeneralSettingsSealed());
			if ((errorCode1 != LarsErr::E_SUCCESS) && (errorCode == LarsErr::E_SUCCESS))
				errorCode = errorCode1;
			break;
		}

		case AdcValueType::ADC_SPIRIT_LEVEL_CAL_MODE:
		{
			errorCode = m_protocol->SetTiltCompensation(NULL, NULL, NULL, NULL, &scaleValues->ScaleValue.spiritLevelCmd);
			break;
		}

		case AdcValueType::ADC_FILTER:
		{
			errorCode = m_protocol->SetFilter(&scaleValues->ScaleValue.filter);
			break;
		}

		case AdcValueType::ADC_G_FACTOR:
		{
			errorCode = m_protocol->SetGFactor(scaleValues->ScaleValue.gFactor);
			break;
		}

		case AdcValueType::ADC_PRODUCTION_SETTINGS:
		{
			errorCode = m_protocol->SetProdSettings(&scaleValues->ScaleValue.prodSettings);
			break;
		}

		case AdcValueType::ADC_CONVERSION_WEIGHT_UNIT:
		{
			errorCode = m_protocol->SetConversionWeightUnit(scaleValues->ScaleValue.conversionWeightUnitFactor);
			break;
		}

		case AdcValueType::ADC_INTERFACE_MODE:
		{
			errorCode = m_protocol->SetInterfaceMode(&scaleValues->ScaleValue.interfaceMode);
			break;
		}

		case AdcValueType::ADC_ZERO_POINT_TRACKING:
		{
			errorCode = m_protocol->SetZeroPointTracking(scaleValues->ScaleValue.mode);
			break;
		}

		case AdcValueType::ADC_ZERO_SETTING_INTERVAL:
		{
			errorCode = m_protocol->SetZeroSettingInterval(scaleValues->ScaleValue.zeroSettingInterval);
			break;
		}

		case AdcValueType::ADC_AUTOMATIC_ZERO_SETTING_TIME:
		{
			errorCode = m_protocol->SetAutomaticZeroSettingTime(scaleValues->ScaleValue.automaticZeroSettingTime);
			break;
		}

		case AdcValueType::ADC_VERIF_PARAM_PROTECTED:
		{
			errorCode = m_protocol->SetVerifParamProtected();
			break;
		}

		case AdcValueType::ADC_UPDATE_ALLOWED:
		{
			errorCode = m_protocol->SetUpdateAllowed(scaleValues->ScaleValue.updateAllowed);
			break;
		}

		case AdcValueType::ADC_OPERATING_MODE:
		{
			AdcOperatingMode currentMode;
			errorCode = LarsErr::E_SUCCESS;
			if (scaleValues->ScaleValue.opMode != m_opMode)
			{
				errorCode = m_protocol->SetOperatingMode(scaleValues->ScaleValue.opMode);
				if (errorCode == LarsErr::E_SUCCESS)
				{
					// wait until the adc is ready in the new operating mode
					short retry = 0;
					while (1)
					{
						errorCode = m_protocol->GetOperatingMode(&currentMode);
						if ((errorCode == LarsErr::E_SUCCESS) && (currentMode == scaleValues->ScaleValue.opMode))
						{
							// successfully switch to new operating mode 
							m_opMode = currentMode;
							break;
						}
						if (retry > 5)
						{
							if (errorCode == LarsErr::E_SUCCESS) errorCode = LarsErr::E_COMMAND_NOT_EXECUTED;
							break;
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
						retry++;
					}
				}
				if (errorCode == LarsErr::E_SUCCESS) 
				{
					// switching to application reinit adc variables
					errorCode = SetAdcVariables();
				}
			}
			break;
		}

		case AdcValueType::ADC_WARM_UP_TIME:
		{
			errorCode = m_protocol->SetWarmUpTime(scaleValues->ScaleValue.warmUpTime);
			break;
		}

		case AdcValueType::ADC_INITIAL_ZERO_SETTING:
		{
			if ((scaleValues->ScaleValue.initialZeroSettingParam.lowerLimit <= 0) && (scaleValues->ScaleValue.initialZeroSettingParam.upperLimit >= 0))
			{
				AdcInitialZeroSettingParam initZeroSetting = scaleValues->ScaleValue.initialZeroSettingParam;
				initZeroSetting.lowerLimit *= (-1);
				if ((errorCode = m_protocol->SetInitialZeroSetting(&initZeroSetting)) == LarsErr::E_SUCCESS)
				{
					// reload load capacity 
					ReadLcSettings();
				}
			}
			else
			{
				// lowerLimit must be <= 0%, upperLimit must be >= 0%, adc check the metrological norm
				errorCode = LarsErr::E_INVALID_PARAMETER;
			}
			break;
		}


		case AdcValueType::ADC_AUTOMATIC_TILT_SENSOR:
		{
			errorCode = m_protocol->SetStateAutomaticTiltSensor(scaleValues->ScaleValue.state);
			break;
		}

        default:
        {
            errorCode = LarsErr::E_INVALID_PARAMETER;
            break;
        }
    }

    m_mutex.unlock();

    return errorCode;
}


/**
******************************************************************************
* GetScaleValues - get the scale values
*
* @param    scaleValues:in  pointer to structure for the specific scale value
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::GetScaleValues(AdcScaleValues *scaleValues)
{
    short errorCode;
	long  eepromSize;

    m_mutex.lock();

    switch (scaleValues->type)
    {
	case AdcValueType::ADC_BASEPRICE:
	{
		errorCode = m_protocol->GetBaseprice(&scaleValues->ScaleValue.basePrice);
		break;
	}

	case AdcValueType::ADC_DISPLAY_TEXT:
	{
		errorCode = m_protocol->GetDisplayText(&scaleValues->ScaleValue.displayText);
		break;
	}

	case AdcValueType::ADC_SCALE_MODE:
	{
		errorCode = m_protocol->GetScaleMode(&scaleValues->ScaleValue.scaleMode);
		break;
	}

	case AdcValueType::ADC_EEPROM:
	{
		AdcEeprom	eeprom;
		short		len;
		short		offset;

		eepromSize = 0;
		if (scaleValues->ScaleValue.eeprom.region == AdcEepromRegion::ADC_WELMEC_REGION)
			eepromSize = m_eepromSize.welmecRegion;
		else if (scaleValues->ScaleValue.eeprom.region == AdcEepromRegion::ADC_OPEN_REGION)
			eepromSize = m_eepromSize.openRegion;
		else if (scaleValues->ScaleValue.eeprom.region == AdcEepromRegion::ADC_PROD_REGION)
			eepromSize = m_eepromSize.prodRegion;
		else if (scaleValues->ScaleValue.eeprom.region == AdcEepromRegion::ADC_PROD_SENSORS_REGION)
			eepromSize = m_eepromSize.prodSensorsRegion;

		if ((scaleValues->ScaleValue.eeprom.startAdr + scaleValues->ScaleValue.eeprom.len) <= eepromSize)
		{
			eeprom.direction = 0;
			eeprom.region = scaleValues->ScaleValue.eeprom.region;

			offset = 0;
			len = scaleValues->ScaleValue.eeprom.len;

			do
			{
				eeprom.startAdr = scaleValues->ScaleValue.eeprom.startAdr + offset;

				// check for max read length
				if (len > EEPROM_MAX_READ_WRITE_LENGTH)
					eeprom.len = EEPROM_MAX_READ_WRITE_LENGTH;
				else
					eeprom.len = len;

				if ((errorCode = m_protocol->GetEeprom(&eeprom)) == LarsErr::E_SUCCESS)
				{
					// copy bytes to input structure
					for (short idx = 0; idx < eeprom.len; idx++)
						scaleValues->ScaleValue.eeprom.data[offset + idx] = eeprom.data[idx];
				}

				offset += eeprom.len;
				len -= eeprom.len;

			} while (len && (errorCode == LarsErr::E_SUCCESS));
		}
		else
		{
			errorCode = LarsErr::E_EEPROM_ACCESS_VIOLATION;
		}
		break;
	}

	case AdcValueType::ADC_TILT_COMP:
	{
		errorCode = m_protocol->GetTiltCompensation(&scaleValues->ScaleValue.tiltCompensation);
		break;
	}

	case AdcValueType::ADC_MAX_DISPL_TEXT_CHAR:
	{
		errorCode = m_protocol->GetMaxDisplayTextChars(&scaleValues->ScaleValue.maxDisplTextChar);
		break;
	}

	case AdcValueType::ADC_FILTER:
	{
		errorCode = m_protocol->GetFilter(&scaleValues->ScaleValue.filter);
		break;
	}

	case AdcValueType::ADC_EEPROM_SIZE:
	{
		errorCode = m_protocol->GetEepromSize(&scaleValues->ScaleValue.eepromSize);
		break;
	}

	case AdcValueType::ADC_CAL_STRINGS:
	{
		strcpy(scaleValues->ScaleValue.calStrings.maxStr, m_lc.MaxString().c_str());
		strcpy(scaleValues->ScaleValue.calStrings.minStr, m_lc.MinString().c_str());
		strcpy(scaleValues->ScaleValue.calStrings.eStr, m_lc.eString().c_str());
		errorCode = LarsErr::E_SUCCESS;
		break;
	}

	case AdcValueType::ADC_G_FACTOR:
	{
		errorCode = m_protocol->GetGFactor(&scaleValues->ScaleValue.gFactor);
		break;
	}

	case AdcValueType::ADC_PRODUCTION_SETTINGS:
	{
		errorCode = m_protocol->GetProdSettings(&scaleValues->ScaleValue.prodSettings);
		break;
	}

	case AdcValueType::ADC_INTERFACE_MODE:
	{
		errorCode = m_protocol->GetInterfaceMode(&scaleValues->ScaleValue.interfaceMode);
		break;
	}

	case AdcValueType::ADC_LOADCAPACITY_UNIT:
	{
		scaleValues->ScaleValue.loadCapacityUnit = (AdcWeightUnit)m_lc.GetWeightUnit();
		errorCode = LarsErr::E_SUCCESS;
		break;
	}

	case AdcValueType::ADC_ZERO_POINT_TRACKING:
	{
		errorCode = m_protocol->GetZeroPointTracking(&scaleValues->ScaleValue.mode);
		break;
	}

	case AdcValueType::ADC_ZERO_SETTING_INTERVAL:
	{
		errorCode = m_protocol->GetZeroSettingInterval(&scaleValues->ScaleValue.zeroSettingInterval);
		break;
	}

	case AdcValueType::ADC_AUTOMATIC_ZERO_SETTING_TIME:
	{
		errorCode = m_protocol->GetAutomaticZeroSettingTime(&scaleValues->ScaleValue.automaticZeroSettingTime);
		break;
	}

	case AdcValueType::ADC_VERIF_PARAM_PROTECTED:
	{
		errorCode = m_protocol->GetVerifParamProtected(&scaleValues->ScaleValue.verifParamProtected);
		break;
	}

	case AdcValueType::ADC_UPDATE_ALLOWED:
	{
		errorCode = m_protocol->GetUpdateAllowed(&scaleValues->ScaleValue.updateAllowed);
		break;
	}

	case AdcValueType::ADC_WARM_UP_TIME:
	{
		errorCode = m_protocol->GetWarmUpTime(&scaleValues->ScaleValue.warmUpTime);
		break;
	}

	case AdcValueType::ADC_REMAINING_WARM_UP_TIME:
	{
		errorCode = m_protocol->GetRemainingWarmUpTime(&scaleValues->ScaleValue.warmUpTime);
		break;
	}

	case AdcValueType::ADC_OPERATING_MODE:
	{
		errorCode = m_protocol->GetOperatingMode(&scaleValues->ScaleValue.opMode);
		break;
	}

	case AdcValueType::ADC_MAX_CAPACITY:
	{
		scaleValues->ScaleValue.maxCapacity = m_lc.GetMaxCapacity();
		errorCode = LarsErr::E_SUCCESS;
		break;
	}

	case AdcValueType::ADC_LOAD_CAPACITY_PARAMS:
	{
		errorCode = m_lc.GetParams(&scaleValues->ScaleValue.loadCapacityParams);
		break;
	}

	case AdcValueType::ADC_INITIAL_ZERO_SETTING:
	{
		errorCode = m_lc.GetInitialZeroSetting(&scaleValues->ScaleValue.initialZeroSettingParam);
		scaleValues->ScaleValue.initialZeroSettingParam.lowerLimit *= (-1);
		break;
	}

	case AdcValueType::ADC_AUTOMATIC_TILT_SENSOR:
	{
		errorCode = m_protocol->GetStateAutomaticTiltSensor(&scaleValues->ScaleValue.state);
		break;
	}

    default:
        errorCode = LarsErr::E_INVALID_PARAMETER;
        break;
    }

    m_mutex.unlock();

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
short Lars::ZeroScale(AdcState *adcState)
{
    short errorCode;

    m_mutex.lock();

	if (adcState)
	{
		errorCode = m_protocol->ZeroScale(adcState);

		if ((errorCode == LarsErr::E_SUCCESS) &&
			(adcState->bit.calibMode == 0) &&
			!m_applAuthenticationDone)
		{
			adcState->state &= LCS_AUTHENTICATION_MASK;
			adcState->bit.needAuthentication = 1;
			adcState->bit.needLogbook = 1;
			errorCode = LarsErr::E_AUTHENTICATION;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

    m_mutex.unlock();

    return errorCode;
}


/**
******************************************************************************
* SetTare - tare scale
*
* @param    adcState:out  state of the adc
* @param    tare:in       tare structure
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::SetTare(AdcState *adcState, const AdcTare *tare)
{
    short   errorCode;

	m_mutex.lock();

    if (tare && adcState && tare->type <= ADC_TARE_LIMIT_KNOWN_CUSTOM)
    {
		errorCode = m_protocol->SetTare(tare, adcState);

		if ((errorCode == LarsErr::E_SUCCESS) &&
			(adcState->bit.calibMode == 0) &&
			!m_applAuthenticationDone)
		{
			adcState->state &= LCS_AUTHENTICATION_MASK;
			adcState->bit.needAuthentication = 1;
			adcState->bit.needLogbook = 1;
			errorCode = LarsErr::E_AUTHENTICATION;
		}
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    m_mutex.unlock();

    return errorCode;
}


/**
******************************************************************************
* GetTare - get tare limits
*
* @param    adcState:out  state of the adc
* @param    tare:in/out   tare structure
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::GetTare(AdcState *adcState, AdcTare *tare)
{
	short   errorCode;

	m_mutex.lock();

	if (tare && adcState && 
		((tare->type == ADC_TARE_LIMIT_DEVICE) || (tare->type == ADC_TARE_LIMIT_CW_CUSTOM) ||
		(tare->type == ADC_TARE_LIMIT_WEIGHED_CUSTOM) || (tare->type == ADC_TARE_LIMIT_KNOWN_CUSTOM) ))
	{
		errorCode = m_protocol->GetTare(tare, adcState);

		if ((errorCode == LarsErr::E_SUCCESS) &&
			(adcState->bit.calibMode == 0) &&
			!m_applAuthenticationDone)
		{
			adcState->state &= LCS_AUTHENTICATION_MASK;
			adcState->bit.needAuthentication = 1;
			adcState->bit.needLogbook = 1;
			errorCode = LarsErr::E_AUTHENTICATION;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* SetTarePriority - set tare priority
*
* @param    adcState:out  state of the adc
* @param    prio:in       prio
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::SetTarePriority(AdcState *adcState, const AdcTarePriority prio)
{
    short   errorCode;

    m_mutex.lock();

	if (adcState)
	{
		errorCode = m_protocol->SetTarePriority(adcState, prio);
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

    m_mutex.unlock();

    return errorCode;
}


/**
******************************************************************************
* ClearTare - clear tare scale
*
* @param    adcState:out  state of the adc
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::ClearTare(AdcState *adcState)
{
    short errorCode;

    m_mutex.lock();

	if (adcState)
	{
		errorCode = m_protocol->ClearTare(adcState);

		if ((errorCode == LarsErr::E_SUCCESS) &&
			(adcState->bit.calibMode == 0) &&
			!m_applAuthenticationDone)
		{
			adcState->state &= LCS_AUTHENTICATION_MASK;
			adcState->bit.needAuthentication = 1;
			adcState->bit.needLogbook = 1;
			errorCode = LarsErr::E_AUTHENTICATION;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

    m_mutex.unlock();

    return errorCode;
}


/**
******************************************************************************
* Reset - reset adc
*
* @param    
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::Reset(const AdcResetType type)
{
    short errorCode;

    m_mutex.lock();

	errorCode = m_protocol->Reset(type);

	m_mutex.unlock();

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
short Lars::ReadWeight(const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcTare *tare, AdcBasePrice *basePrice, AdcPrice *sellPrice)
{
    short errorCode;

    m_mutex.lock();

	if (adcState)
	{
		errorCode = m_protocol->ReadWeight(registrationRequest, adcState, weight, tare, basePrice, sellPrice);

		if ((errorCode == LarsErr::E_SUCCESS) &&
			(adcState->bit.calibMode == 0) &&
			!m_applAuthenticationDone)
		{
			adcState->state &= LCS_AUTHENTICATION_MASK;
			adcState->bit.needAuthentication = 1;
			adcState->bit.needLogbook = 1;

			if (weight)
			{
				weight->value = 0;
				weight->weightUnit = (AdcWeightUnit)m_lc.GetWeightUnit();
				weight->decimalPlaces = m_lc.GetDecimalPlaces();
			}

			if (tare)
			{
				tare->type = AdcTareType::ADC_TARE_WEIGHED;
				tare->frozen = 0;
				tare->value.value = 0;
				tare->value.weightUnit = (AdcWeightUnit)m_lc.GetWeightUnit();
				tare->value.decimalPlaces = m_lc.GetDecimalPlaces();
			}

			if (basePrice)
			{
				basePrice->price.value = 0;
				basePrice->price.decimalPlaces = 0;
				basePrice->price.currency[0] = '\0';
				basePrice->weightUnit = (AdcWeightUnit)m_lc.GetWeightUnit();
			}

			if (sellPrice)
			{
				sellPrice->value = 0;
				sellPrice->decimalPlaces = 0;
				sellPrice->currency[0] = '\0';
			}

			errorCode = LarsErr::E_AUTHENTICATION;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

    m_mutex.unlock();

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
* @return   errorCode
* @remarks  The latest log book entry has index = 0
******************************************************************************
*/
short Lars::GetLogger(const short index, char *entry, unsigned long *size)
{
    short errorCode;

    m_mutex.lock();

	errorCode = m_protocol->GetLogger(index, entry, size);

    m_mutex.unlock();

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
short Lars::GetRandomAuthentication(unsigned char *random, unsigned long *size)
{
    short errorCode;

    m_mutex.lock();

	errorCode = m_protocol->GetRandomAuthentication(random, size);

    m_mutex.unlock();

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
short Lars::SetAuthentication(const AdcAuthentication *authentication)
{
    short errorCode;
   
    m_mutex.lock();

    if (authentication)
    {
        errorCode = m_protocol->SetAuthentication(authentication);
		if (errorCode == LarsErr::E_SUCCESS)
		{
			m_applAuthenticationDone = true;
		}
    }
    else
    {
        errorCode = LarsErr::E_INVALID_PARAMETER;
    }

    m_mutex.unlock();

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
short Lars::GetHighResolution(const short registrationRequest, AdcState *adcState, AdcWeight *weight, AdcWeight *weightHighResolution, AdcTare *tare, long *digitValue)
{
    short errorCode;

    m_mutex.lock();

	if (adcState)
	{
		errorCode = m_protocol->GetHighResolution(registrationRequest, adcState, weight, weightHighResolution, tare, digitValue);

		if ((errorCode == LarsErr::E_SUCCESS) &&
			(adcState->bit.calibMode == 0) &&
			!m_applAuthenticationDone)
		{
			adcState->state &= LCS_AUTHENTICATION_MASK;
			adcState->bit.needAuthentication = 1;
			adcState->bit.needLogbook = 1;

			if (weight)
			{
				weight->value = 0;
				weight->weightUnit = (AdcWeightUnit)m_lc.GetWeightUnit();
				weight->decimalPlaces = m_lc.GetDecimalPlaces();
			}

			if (weightHighResolution)
			{
				weightHighResolution->value = 0;
				weightHighResolution->weightUnit = (AdcWeightUnit)m_lc.GetWeightUnit();
				weightHighResolution->decimalPlaces = 0;
			}

			if (digitValue)
			{
				*digitValue = 0;
			}

			errorCode = LarsErr::E_AUTHENTICATION;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

    m_mutex.unlock();

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
short Lars::GetGrossWeight(AdcState *adcState, AdcWeight *grossWeight, AdcWeight *grossWeightHighResolution, long *digitValue)
{
	short errorCode;

	m_mutex.lock();

	if (adcState)
	{
		errorCode = m_protocol->GetGrossWeight(adcState, grossWeight, grossWeightHighResolution, digitValue);

		if ((errorCode == LarsErr::E_SUCCESS) &&
			(adcState->bit.calibMode == 0) &&
			!m_applAuthenticationDone)
		{
			adcState->state &= LCS_AUTHENTICATION_MASK;
			adcState->bit.needAuthentication = 1;
			adcState->bit.needLogbook = 1;

			if (grossWeight)
			{
				grossWeight->value = 0;
				grossWeight->weightUnit = (AdcWeightUnit)m_lc.GetWeightUnit();
				grossWeight->decimalPlaces = m_lc.GetDecimalPlaces();
			}

			if (grossWeightHighResolution)
			{
				grossWeightHighResolution->value = 0;
				grossWeightHighResolution->weightUnit = (AdcWeightUnit)m_lc.GetWeightUnit();
				grossWeightHighResolution->decimalPlaces = 0;
			}

			if (digitValue)
			{
				*digitValue = 0;
			}

			errorCode = LarsErr::E_AUTHENTICATION;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* GetFirstDiagnosticData - function to get diagnostic data from the adc
*
* @param    name:in				if name == EmptyString the function give all diagnostic values
* @param    sensorHealthID:out	sensor health id for the function GetNextDiagnosticData
* @param    sensorHealth:out	diagnostic values
* @param	state:out			0	no more diagnostic data
*								1	succeed
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::GetFirstDiagnosticData(const char *name, long *sensorHealthID, AdcSensorHealth *sensorHealth, short *state)
{
    short errorCode;
	map<string, AdcSensorHealth> *mapSensorHealth;

    m_mutex.lock();

	if (state) *state = 0;

	mapSensorHealth = new map<string, AdcSensorHealth>;

	errorCode = m_protocol->GetDiagnosticData(name, *mapSensorHealth);

	if ((errorCode == LarsErr::E_SUCCESS) && (mapSensorHealth->size() != 0))
	{
		// create new sensorHealthID
		long id = CreateNewSensorHealthID();

		if (id != INVALID_SENSOR_ID)
		{
			// get first element from map
			map<string, AdcSensorHealth>::iterator it = mapSensorHealth->begin();

			if (sensorHealth) *sensorHealth = (*it).second;
			if (state) *state = 1;
			if (sensorHealthID) *sensorHealthID = id;
	
			// delete first element from map
			mapSensorHealth->erase((*it).first);

			// queuing element
			m_sensorHealth[id] = *mapSensorHealth;
		}
		else
		{
			errorCode = LarsErr::E_SENSOR_HEALTH_QUEUE_FULL;
		}
	}

	// free map
	delete mapSensorHealth;

    m_mutex.unlock();

    return errorCode;
}


/**
******************************************************************************
* GetNextDiagnosticData - function to get diagnostic data from the adc
*
* @param    sensorHealth:out	diagnostic values
* @param    sensorHealthID:in	sensor health id
* @param	state:out			0	no more diagnostic data
*								1	succeed
*
* @remarks
******************************************************************************
*/
void Lars::GetNextDiagnosticData(const long sensorHealthID, AdcSensorHealth *sensorHealth, short *state)
{
	map<string, AdcSensorHealth> *mapSensorHealth;

	m_mutex.lock();

	if (state) *state = 0;

	// get element
	map<long, map<string, AdcSensorHealth>>::iterator it = m_sensorHealth.find(sensorHealthID);
	if (it != m_sensorHealth.end())
	{
		mapSensorHealth = &(*it).second;
		if (mapSensorHealth->size() != 0)
		{
			// get first element from map
			map<string, AdcSensorHealth>::iterator it = mapSensorHealth->begin();

			if (sensorHealth) *sensorHealth = (*it).second;

			// delete first element from map
			mapSensorHealth->erase((*it).first);

			// overwrite 
			m_sensorHealth[sensorHealthID] = *mapSensorHealth;

			if (state) *state = 1;
		}
		else
		{
			// no more elements in map, so delete element in queue
			m_sensorHealth.erase(sensorHealthID);
		}
	}

	m_mutex.unlock();

	return;
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
short Lars::ConfigureDiagnosticData(const AdcSensorHealth *sensorHealth)
{
	short errorCode;

	m_mutex.lock();

	if (sensorHealth)
	{
		errorCode = m_protocol->ConfigureDiagnosticData(sensorHealth);
		if (errorCode == LarsErr::E_SUCCESS)
		{
			m_applAuthenticationDone = true;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* CreateNewSensorHealthID - function to create a new sensor health id
*
* @return   errorCode
* @remarks
******************************************************************************
*/
long Lars::CreateNewSensorHealthID()
{
	long sensorID;

	if (m_sensorHealth.size() == 0)
	{
		sensorID = 1;
	}
	else
	{
		// get id from last entry
		map<long, map<string, AdcSensorHealth>>::iterator it = m_sensorHealth.end();

		sensorID = (*it).first;

		if (sensorID < 0x100)
			sensorID++;
		else
			sensorID = 1;

		// check if ID is free
		if (m_sensorHealth.find(sensorID) == m_sensorHealth.end())
			sensorID = INVALID_SENSOR_ID;
	}

	return sensorID;
}



/**
******************************************************************************
* Calibration - function for scale calibration
*
* @param    cmd:in			calibration command
* @param    adcState:out    state of the adc
* @param    step:out		calibration step
* @param    calibDigit:out	weight in digit in calibration mode
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::Calibration(const AdcCalibCmd cmd, AdcState *adcState, long *step, long *calibDigit)
{
	short errorCode;

	m_mutex.lock();

	errorCode = m_protocol->Calibration(cmd, adcState, step, calibDigit);

	if ((cmd == AdcCalibCmd::ADC_CALIB_GET_STATE && step && *step == 1000) ||
		(cmd == AdcCalibCmd::ADC_CALIB_CANCEL))
	{
		// refresh capabilities from ADC, because of wdta settings
		// ignore any error
		m_adcCap.clear();
		m_protocol->GetCapabilities(m_adcCap);
	}

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* SetCountryFilesPath - function to set the path to the adc country files
*
* @param    path:in		
*
* @return   
* @remarks
******************************************************************************
*/
void Lars::SetCountryFilesPath(const char *path)
{
	m_cySetting.SetPath(string(path));
}


/**
******************************************************************************
* GetSupportedCountries - function to get the supported countries
*
* @param    countries:out	list of all supported countries
* @param    size:in		    size of variable countries
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::GetSupportedCountries(char *countries, unsigned long *size)
{
	short                       errorCode = LarsErr::E_SUCCESS;
	CountrySettings::supportedCountries   countriesVector;
	stringstream                outStream;
	bool						bFirstRun;

	if (!size)
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
		return errorCode;
	}

	m_mutex.lock();

	m_cySetting.GetSupportedCountries(countriesVector);

	// convert to user format
	if (!countriesVector.empty())
	{
		bFirstRun = true;
		outStream << STX;
		for (CountrySettings::supportedCountries::iterator it = countriesVector.begin(); it != countriesVector.end(); ++it)
		{
			if (bFirstRun == false) outStream << GS;

			outStream << *it;

			bFirstRun = false;
		}
		outStream << ETX;

		if (*size >= outStream.str().size() + 1)
		{
			// don't forget the terminating \0 on allocating memory
			*size = outStream.str().size() + 1;
			if (countries) strcpy(countries, outStream.str().c_str());
		}
		else
		{
			if (countries == NULL)
				// don't forget the terminating \0
				*size = outStream.str().size() + 1;
			else
				*size = 0;
		}
	}
	else
	{
		*size = 0;
	}

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* SetCountry - function to set the adc country
*
* @param    name:in		ISO country name
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::SetCountry(const char *name)
{
    short errorCode = LarsErr::E_SUCCESS;
    string countryISO = name;
    string saveCountryISO;

    m_mutex.lock();

    // save old country
    saveCountryISO = m_cySetting.GetCountry();

    // set new country
    errorCode = m_cySetting.SetCountry(countryISO);
    if (errorCode != LarsErr::E_SUCCESS)
    {
        // the new country is not supported, so restore the old one
        m_cySetting.SetCountry(saveCountryISO);
    }

	if (errorCode == LarsErr::E_SUCCESS)
	{
		// send country specific parameters to adc
		errorCode = m_protocol->SetCountrySettings(m_cySetting.GetSettings());
	}

	if (errorCode == LarsErr::E_SUCCESS)
	{
		m_lc.SetRegistrationParameters(m_cySetting.GetRegistrationParameters());
		m_lc.SetCalTemplateStrings(m_cySetting.MaxTemplateString(), m_cySetting.MinTemplateString(), m_cySetting.eTemplateString());
	}

    m_mutex.unlock();

    return errorCode;
}

/**
******************************************************************************
* GetCountry - function to get the adc country
*
* @param    name:out		country name
* @param    size:in		    size of variable name
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::GetCountry(char *name, unsigned long size)
{
    short   errorCode = LarsErr::E_SUCCESS;
    string  countryISO;

    m_mutex.lock();

    countryISO = m_cySetting.GetCountry();
    if (size > countryISO.size())
        strcpy(name, countryISO.c_str());
    else
        errorCode = LarsErr::E_NOT_ENOUGH_MEMORY;

    m_mutex.unlock();

    return errorCode;
}


/**
******************************************************************************
* GetCompatibleLoadCapacities - function to get the compatbile load capacities
*
* @param    loadCapacities:out		supported load capacities of the country
* @param    size:in/out				size of variable name
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::GetCompatibleLoadCapacities(char *loadCapacities, unsigned long *size)
{
	return GetCompatibleLoadCapacities(NULL, loadCapacities, size);
}


/**
******************************************************************************
* GetCompatibleLoadCapacities - function to get the compatbile load capacities
*
* @param    country:in				country name
* @param    loadCapacities:out		supported load capacities of the country
* @param    size:in					size of variable name
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::GetCompatibleLoadCapacities(const char *country, char *loadCapacities, unsigned long *size)
{
	short                       errorCode = LarsErr::E_SUCCESS;
	CountrySettings				cySettings;
	CountrySettings::loadCapacity   loadCapacityMap;
	stringstream                outStream;
	bool						bFirstRun;

	if (!size)
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
		return errorCode;
	}

	m_mutex.lock();

	if (country == NULL)
	{
		m_cySetting.GetCompatibleLoadCapacities(loadCapacityMap);
	}
	else if (strlen(country) == 0)
	{
		m_cySetting.GetCompatibleLoadCapacities(loadCapacityMap);
	}
	else
	{
		if ((errorCode = cySettings.SetCountry(country, m_cySetting.GetPath())) != LarsErr::E_SUCCESS)
		{
			m_mutex.unlock();
			return errorCode;
		}

		cySettings.GetCompatibleLoadCapacities(loadCapacityMap);
	}

	// convert to user format
	if (!loadCapacityMap.empty())
	{
		bFirstRun = true;
		outStream << STX;
		for (CountrySettings::loadCapacity::iterator it = loadCapacityMap.begin(); it != loadCapacityMap.end(); ++it)
		{
			if (bFirstRun == false) outStream << GS;

			outStream << (*it).first;

			bFirstRun = false;
		}
		outStream << ETX;

		if (*size >= outStream.str().size() + 1)
		{
			// don't forget the terminating \0 on allocating memory
			*size = outStream.str().size() + 1;
			if (loadCapacities) strcpy(loadCapacities, outStream.str().c_str());
		}
		else
		{
			if (loadCapacities == NULL)
				// don't forget the terminating \0
				*size = outStream.str().size() + 1;
			else
				*size = 0;
		}
	}
	else
	{
		*size = 0;
	}

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* SetLoadCapacity - function to set the load capacity
*
* @param    loadCapacity:int	load capacity
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::SetLoadCapacity(const char *loadCapacity)
{
    short                           errorCode = LarsErr::E_SUCCESS;
    CountrySettings::loadCapacity   loadCapacityMap;
    CountrySettings::loadCapacity::iterator it;
	string							loadCapacityString(loadCapacity);
	string							fileName;
	string							path;
	long							digitsResolution;

    m_mutex.lock();

    // check if load capacity is supported
    m_cySetting.GetCompatibleLoadCapacities(loadCapacityMap);
    it = loadCapacityMap.find(loadCapacity);
    if (it != loadCapacityMap.end())
    {
		loadCapacityString = (*it).first;
        fileName = (*it).second;
		errorCode = m_lc.Set(loadCapacityString, fileName, m_cySetting.GetPath(), m_cySetting.GetRegistrationParameters());
    }
    else
    {
		errorCode = m_lc.Set(loadCapacityString, fileName, path, m_cySetting.GetRegistrationParameters());
    }

	if (errorCode == LarsErr::E_SUCCESS)
	{
		// get digits resolution
		digitsResolution = m_lc.GetDigitsResolution();

		// check if load capacity is supported
		if ((digitsResolution != m_lc.DEFAULT_DIGITS_RESOLUTION) && (!m_adcCap[AdcCapabilities::ADC_CAP_DIGITS_RESOLUTION_CONFIGURABLE]))
			errorCode = LarsErr::E_LOAD_CAPACITY_NOT_SUPPORTED;
	}

	if (errorCode == LarsErr::E_SUCCESS)
	{
		// send load capacity parameter to adc
		errorCode = m_protocol->SetLoadCapacity(m_lc.GetSettings());
	}

    m_mutex.unlock();

    return errorCode;
}


/**
******************************************************************************
* GetLoadCapacity - function to get the current adc load capacity
*
* @param    loadCapacity:out	load capacity
* @param    size:in				size of variable name
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::GetLoadCapacity(char *loadCapacity, unsigned long size)
{
	short   errorCode = LarsErr::E_SUCCESS;
	string  loadCapacityStr;

	m_mutex.lock();

	if (loadCapacity)
	{
		loadCapacityStr = m_lc.GetLoadCapacity();
		if (size > loadCapacityStr.size())
			strcpy(loadCapacity, loadCapacityStr.c_str());
		else
			errorCode = LarsErr::E_NOT_ENOUGH_MEMORY;
	}
	else
		errorCode = LarsErr::E_INVALID_PARAMETER;

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* SetFirmwarePath - function to set the path to the adc country files
*
* @param    path:in
*
* @return
* @remarks
******************************************************************************
*/
void Lars::SetFirmwarePath(const char *path)
{
	m_firmwarePath = path;
}

/**
******************************************************************************
* Update - function to update the adc
*
* @param    directory:in	directory of the firmware files
* @param    force:in		0: check parameter if update is possible
*							1: no check, always execute update
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::Update(const short force)
{
	short		errorCode = LarsErr::E_SUCCESS;
	AdcFirmware firmware;
	AdcFrmData	frmData;
	unsigned long pos;
	bool		sendData;
	
	m_mutex.lock();

	errorCode = firmware.LoadFile(m_firmwarePath, m_adcType);
	if ((errorCode == LarsErr::E_FILE_NOT_FOUND) && ((m_adcType == "ADC505") || (m_adcType == "ADW505")))
	{
		// file not found, try again with the compatible adctype
		string tmpAdcType;
		if (m_adcType == "ADC505") tmpAdcType = "ADW505";
		else tmpAdcType = "ADC505";

		errorCode = firmware.LoadFile(m_firmwarePath, tmpAdcType);
	}

	if (errorCode == LarsErr::E_SUCCESS)
	{
		// send start download command
		string usbStackVersion = firmware.GetUsbStackVersion();
		string welmecStructureVersion = firmware.GetWelmecStructureVersion();
		errorCode = m_protocol->FirmwareUpdate(ADC_FRMUPDATE_START, NULL, &usbStackVersion, &welmecStructureVersion, force);

		if (errorCode == LarsErr::E_SUCCESS)
		{
			// write new firmware
			pos = 0;
			while (firmware.GetData(pos, &frmData))
			{
				// send only data package != 0xFF
				sendData = false;
				for (short idx = 0; idx < frmData.len; idx++)
				{
					if (frmData.data[idx] != 0xFF)
					{
						sendData = true;
						break;
					}
				}
				if (sendData == true)
				{
					if ((errorCode = m_protocol->FirmwareUpdate(ADC_FRMUPDATE_WRITE_DATA, &frmData)) != LarsErr::E_SUCCESS)
					{
						if ((m_bootLoaderVersion < 1.05) && ((frmData.startAdr + frmData.len - 1) >= 0xFFFF9F00) && ((frmData.startAdr + frmData.len - 1) <= 0xFFFF9FFF))
						{
							// bugfix for bootloader < 1.05
							// only check errorCode for data not written to flash block [0xFFFF9F00 - 0xFFFF9FFF], ignore errorCode for data written to flash block [0xFFFF9F00 - 0xFFFF9FFF] 
							errorCode = LarsErr::E_SUCCESS;
						}
						else
						{
							break;
						}
					}
				}
				pos += frmData.len;
			}
		}

		if (errorCode == LarsErr::E_SUCCESS)
		{
			// send end download command
			errorCode = m_protocol->FirmwareUpdate(ADC_FRMUPDATE_END, NULL);
		}
		else
		{
			// error occured, send cancel download command
			m_protocol->FirmwareUpdate(ADC_FRMUPDATE_CANCEL, NULL);
		}
	}

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* GetFirmwareFileVersion - get the version of the firmware file
*
* @param    versionStr:out  version of the components as key value pairs
* @param    size:in/out     size fo versionStr
*
* @return	errorCode
* @remarks
******************************************************************************
*/
short Lars::GetFirmwareFileVersion(char *versionStr, unsigned long *size)
{
	short               errorCode;
	AdcFirmware			firmware;
	string				firmwareVersion;
	
	m_mutex.lock();

	errorCode = firmware.GetVersion(m_firmwarePath, m_adcType, firmwareVersion);
	if ((errorCode == LarsErr::E_FILE_NOT_FOUND) && ((m_adcType == "ADC505") || (m_adcType == "ADW505")))
	{
		// file not found, try again with the compatible adctype
		string tmpAdcType;
		if (m_adcType == "ADC505") tmpAdcType = "ADW505";
		else tmpAdcType = "ADC505";

		errorCode = firmware.GetVersion(m_firmwarePath, tmpAdcType, firmwareVersion);
	}

	if (errorCode == LarsErr::E_SUCCESS)
	{
		if ((versionStr) && (size) && (*size != 0))
		{
			// don't forget the terminating \0 
			if (*size >= firmwareVersion.length() + 1)
			{
				// don't forget the terminating \0 
				*size = firmwareVersion.length() + 1;
				strcpy(versionStr, firmwareVersion.c_str());

				errorCode = LarsErr::E_SUCCESS;
			}
			else
			{
				*size = 0;
				errorCode = LarsErr::E_NOT_ENOUGH_MEMORY;
			}
		}
		else if (size)
		{
			*size = firmwareVersion.length() + 1;
			errorCode = LarsErr::E_SUCCESS;
		}
	}

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* MakeAuthentication - make the authentication 
*
* @param
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::MakeAuthentication(bool withIdentityString)
{
    short               errorCode;
    unsigned char       random[8];
    unsigned long       size = sizeof(random);
    AdcAuthentication   structauth;
    Authentication      auth(ADW_AUTH_CRC_START, ADW_AUTH_CRC_POLY, ADW_AUTH_CRC_MASK);
	struct tm * timeinfo;
	char timeString[80];

    // get random from adc
    errorCode = m_protocol->GetRandomAuthentication(random, &size);
    if (errorCode == LarsErr::E_SUCCESS)
    {
        structauth.chksum = auth.CalcCrc(random, size);
        structauth.swIdentity = NULL;

		if (withIdentityString)
		{
			// get local time
			timeinfo = Helpers::GetTime();
			strftime(timeString, sizeof(timeString), "%y%m%d%H%M", timeinfo);

			stringstream    logBookEntry;
			logBookEntry << STX << "vid" << FS << APPLICATION_VENDOR_ID << GS << \
				"cid" << FS << APPLICATION_COMPONENT_ID << GS << \
				"swvl" << FS << APPLICATION_VERSION_WELMEC << GS << \
				"swid" << FS << APPLICATION_ID_WELMEC << GS << \
				"swv" << FS << APPLICATION_VERSION_STRING << GS << \
				"date" << FS << timeString << ETX;

			// don't forget the terminating 0
			structauth.swIdentity = (char *)malloc(logBookEntry.str().size() + 1);
			if (structauth.swIdentity)
				strcpy(structauth.swIdentity, logBookEntry.str().c_str());
		}

	    errorCode = m_protocol->SetAuthentication(&structauth);

        // free memory
        if (structauth.swIdentity)
        {
            free(structauth.swIdentity);
        }
    }
    return errorCode;
}


/**
******************************************************************************
* CheckDoAuthentication - check and it necessary do the authentication
*
* @param	setApplIdentication:in	true set variable m_applAuthenticationDone
*									false not set variable m_applAuthenticationDone
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::CheckDoAuthentication(bool setApplIdentication)
{
	short		errorCode = LarsErr::E_SUCCESS;
	//AdcState	adcState;
	//bool		swIdentity = false;

	//adcState.state = 0;

	// get adc state
	//errorCode = m_protocol->ReadWeight(0, &adcState, NULL, NULL, NULL, NULL);
	//if ((errorCode != LarsErr::E_SUCCESS) &&
	//	(errorCode != LarsErr::E_AUTHENTICATION))
	//{
	//	return errorCode;
	//}

	//if ((adcState.bit.needAuthentication == 1) ||
	//	(adcState.bit.needLogbook == 1))
	//{
	//	if (setApplIdentication) m_applAuthenticationDone = false;

	//	if (adcState.bit.needLogbook) swIdentity = true;

	//	// make authentication only in welmec mode
	//	errorCode = MakeAuthentication(swIdentity);
	//}
	//else
	//{
	//	if (setApplIdentication) m_applAuthenticationDone = true;
	//}

	if (setApplIdentication) m_applAuthenticationDone = true;
	
	return errorCode;
}



/**
******************************************************************************
* Parameters - send adc parameters mode
*
* @param	mode:in			0: load default sensor health configuration
*							1: save sensor health data
*							2: load factory settings
*							3: reset sensor health data
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::Parameters(const AdcParamMode mode)
{
	short errorCode;

	m_mutex.lock();

	errorCode = m_protocol->Parameters(mode);

	m_mutex.unlock();

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
short Lars::GetInternalDataEx(AdcInternalDataEx *internalData, bool sendRequestCmd)
{
	short errorCode;

	m_mutex.lock();

	errorCode = m_protocol->GetInternalDataEx(internalData, sendRequestCmd);

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* GetEepromSize - get eeprom size
*
* @param eepromSize:out	pointer of struct eeprom size
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::GetEepromSize(AdcEepromSize *eepromSize)
{
	short errorCode;

	m_mutex.lock();

	errorCode = m_protocol->GetEepromSize(eepromSize);

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* SetScaleModel - function to set the scale model
*
* @param    model:in	scale model
*
* @return
* @remarks
******************************************************************************
*/
short Lars::SetScaleModel(const char *model)
{
	short errorCode;

	m_mutex.lock();

	errorCode = SetScaleModelIntern(model);

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* SetScaleModelIntern - function to set the scale model
*
* @param    model:in	scale model
*
* @return
* @remarks
******************************************************************************
*/
short Lars::SetScaleModelIntern(const char *model)
{
	short   errorCode = LarsErr::E_SUCCESS;

	if (model != NULL)
	{
		m_scaleModel = model;

		// write scale model to adc
		errorCode = m_protocol->SetScaleModel(m_scaleModel);
	}

	if (m_opMode == ADC_APPLICATION)
	{
		LoadSspParam();
	}

	return errorCode;
}


/**
******************************************************************************
* GetScaleModel - function to get the scale model
*
* @param    model:out	scale model
* @param    size:in		size of variable model

*
* @return	errorCode
* @remarks
******************************************************************************
*/
short Lars::GetScaleModel(char *model, unsigned long size)
{
	short   errorCode = LarsErr::E_SUCCESS;

	m_mutex.lock();

	if (model)
	{
		if (size > m_scaleModel.size())
			strcpy(model, m_scaleModel.c_str());
		else
			errorCode = LarsErr::E_NOT_ENOUGH_MEMORY;
	}
	else
		errorCode = LarsErr::E_INVALID_PARAMETER;

	m_mutex.unlock();

	return errorCode;
}


/**
******************************************************************************
* SetAdcType - extract the adc type from the version map
*
* @param versionMap:in	pointer to version map
*
* @return
* @remarks
******************************************************************************
*/
void Lars::SetAdcType(map<string, string> &versionMap)
{
	// get adc type from version map
	for (map<string, string>::iterator it = versionMap.begin(); it != versionMap.end(); it++)
	{
		if ((*it).first == "adct")
		{
			m_adcType = (*it).second;
			break;
		}
	}
}


/**
******************************************************************************
* SetWsType - extract the ws type from the version map
*
* @param versionMap:in	pointer to version map
*
* @return
* @remarks
******************************************************************************
*/
void Lars::SetWsType(map<string, string> &versionMap)
{
	// get adc type from version map
	for (map<string, string>::iterator it = versionMap.begin(); it != versionMap.end(); it++)
	{
		if ((*it).first == "wsc")
		{
			m_wsType = (*it).second;
			size_t pos;
			if ((pos = m_wsType.find(' ', 0)) != string::npos)
			{
				m_wsType = m_wsType.substr(0, pos);
			}
			break;
		}
	}
}


/**
******************************************************************************
* SetBootLoaderVersion - extract the bootloader version from the version map
*
* @param versionMap:in	pointer to version map
*
* @return
* @remarks
******************************************************************************
*/
void Lars::SetBootLoaderVersion(map<string, string> &versionMap)
{
	m_bootLoaderVersion = 0;

	// get adc type from version map
	for (map<string, string>::iterator it = versionMap.begin(); it != versionMap.end(); it++)
	{
		if ((*it).first == "boot")
		{
			m_bootLoaderVersion = atof((*it).second.c_str());
			break;
		}
	}
}


/**
******************************************************************************
* SetSspPath - function to set the path to the ssp files
*
* @param    path:in		path to the ssp files
*
* @return
* @remarks
******************************************************************************
*/
void Lars::SetSspPath(const char *path)
{
	if (path != NULL) m_sspPath = path;

	if (m_opMode == ADC_APPLICATION)
	{
		LoadSspParam();
	}
}

/**
******************************************************************************
* LoadSspParam - function to load the ssp parameters
*
* @param    
*
* @return
* @remarks
******************************************************************************
*/
void Lars::LoadSspParam()
{
	m_ssp.Clear();

	// try to load the scale specific parameter
	m_ssp.LoadFile(&m_sspPath, NULL, NULL, NULL);

	// try to load the scale specific parameter for the adcType
	m_ssp.LoadFile(&m_sspPath, &m_adcType, NULL, NULL);

	// try to load the scale specific parameter for the adcType and the wsType
	m_ssp.LoadFile(&m_sspPath, NULL, &m_wsType, NULL);

	// try to load the scale specific parameter for the adcType and the wsType
	m_ssp.LoadFile(&m_sspPath, &m_adcType, &m_wsType, NULL);

	if (m_scaleModel.length())
	{
		// try to load the scale specific parameter for the scale model
		m_ssp.LoadFile(&m_sspPath, NULL, NULL, &m_scaleModel);

		// try to load the scale specific parameter for the wsType and the scale model
		m_ssp.LoadFile(&m_sspPath, NULL, &m_wsType, &m_scaleModel);

		// try to load the scale specific parameter for the adcType, the wsType and the scale model
		m_ssp.LoadFile(&m_sspPath, &m_adcType, &m_wsType, &m_scaleModel);
	}
}


/**
******************************************************************************
* GetCalStrings4LoadCapacity - function to get the calibration strings min/max/e
*							   for a specific load capacity
*
* @param    handle:in		adc handle
* @param	loadCapacity:in	load capacity
* @param    calStrings:out	structure for calibration strings
*
* @return	errorCode
* @remarks
******************************************************************************
*/
short Lars::GetCalStrings4LoadCapacity(const char *loadCapacity, AdcCalStrings *calStrings)
{
	short			errorCode = LarsErr::E_SUCCESS;
	LoadCapacity    lc;
	string			loadCapacityString(loadCapacity);
	string			loadCapacityFile;

	loadCapacityFile = loadCapacityString + "/loadcapacity_" + loadCapacityString + ".txt";
	errorCode = lc.Set(loadCapacityString, loadCapacityFile, m_cySetting.GetPath());

	if (errorCode == LarsErr::E_SUCCESS)
	{
		lc.SetCalTemplateStrings(m_cySetting.MaxTemplateString(), m_cySetting.MinTemplateString(), m_cySetting.eTemplateString());
		strcpy(calStrings->maxStr, lc.MaxString().c_str());
		strcpy(calStrings->minStr, lc.MinString().c_str());
		strcpy(calStrings->eStr, lc.eString().c_str());
		errorCode = LarsErr::E_SUCCESS;
	}
	
	return errorCode;
}


/**
******************************************************************************
* InitCapabilities - function to initialize the adc capabilities
*
* @param    void
*
* @return	void
* @remarks
******************************************************************************
*/
void Lars::InitCapabilities()
{
	m_adcCap[AdcCapabilities::ADC_CAP_DEVICE_AUTHENTICATION] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_DISPLAY_WEIGHT] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_DISPLAY_TARE] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_DISPLAY_BASEPRICE] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_DISPLAY_PRICE] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_DISPLAY_TEXT] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_PRICE_CALCULATION] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_TARE] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_ZERO_SCALE] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_TILT_COMPENSATION] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_TARE_PRIORITY] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_TWO_CONVERTER] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_SEPARATE_LOADCELL] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_FIRMWARE_UPDATE] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_HIGHRESOLUTION_WITH_TARE] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_DIGITS_RESOLUTION_CONFIGURABLE] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_TILT_COMPENSATION_CORNER_CORRECTION] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_WEIGHT_DEPENDENT_TILT_ANGLE] = 0;
	m_adcCap[AdcCapabilities::ADC_CAP_AUTOMATIC_TILT_SENSOR] = 0;
}

/**
******************************************************************************
* GetPort - get port
*
* @return   errorCode
* @remarks
******************************************************************************
*/
short Lars::GetPortNr(char *portNr, unsigned long *size)
{
	short	errorCode = LarsErr::E_SUCCESS;
	string	strPortNr;

	if ((portNr) && (size))
	{
		m_interface->GetPort(strPortNr);

		// don't forget the terminating \0 
		if (*size >= strPortNr.size() + 1)
		{
			// don't forget the terminating \0 
			*size = strPortNr.size() + 1;
			strcpy(portNr, strPortNr.c_str());

			errorCode = LarsErr::E_SUCCESS;
		}
		else
		{
			*size = 0;
			errorCode = LarsErr::E_NOT_ENOUGH_MEMORY;
		}
	}
	else
	{
		errorCode = LarsErr::E_INVALID_PARAMETER;
	}

	return errorCode;
}
