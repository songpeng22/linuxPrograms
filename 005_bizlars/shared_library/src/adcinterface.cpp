/**
******************************************************************************
* File       : adcinterface.cpp
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcinterface class (virtual)
******************************************************************************
*/
#include "adcinterface.h"


AdcInterface::AdcInterface()
{
    m_hDevice = 0;
    m_useCRC16 = false;
}

AdcInterface::~AdcInterface()
{
}

short AdcInterface::GetInterfaceType()
{
    return m_InterfaceType;
}

bool AdcInterface::UseCRC16()
{
	return m_useCRC16;
}

bool AdcInterface::ResetInterface()
{
	return true;
}

bool AdcInterface::DriverVersion(unsigned short *major, unsigned short *minor)
{
	return false;
}

void AdcInterface::GetPort(string &port)
{
	port = "";
}

