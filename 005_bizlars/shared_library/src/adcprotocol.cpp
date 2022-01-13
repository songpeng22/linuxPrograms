/**
******************************************************************************
* File       : adcprotocol.cpp
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcprotocol class (virtual)
******************************************************************************
*/
#include "adcprotocol.h"
#include "adctrace.h"

AdcProtocol::AdcProtocol()
{
	Init(NULL);
}

AdcProtocol::AdcProtocol(AdcInterface *pInterface) 
{
	Init(pInterface);
}

AdcProtocol::~AdcProtocol()
{
    if (m_receiveBuffer) delete[] m_receiveBuffer;
}

void AdcProtocol::Init(AdcInterface *Interface)
{
	try
	{
		m_receiveBuffer = new char[RECEIVE_BUFFER_SIZE];
	}
	catch (std::bad_alloc &e)
	{
		m_receiveBuffer = 0;
		g_adcTrace.Trace(AdcTrace::TRC_ERROR_WARNING, "%s\texception: %s", __FUNCTION__, e.what());
	}
	m_interface = Interface;

	m_major = 0;
	m_minor = 0;
}

void AdcProtocol::SetInterface(AdcInterface *pInterface)
{
    m_interface = pInterface;
}
