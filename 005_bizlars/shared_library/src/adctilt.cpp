/**
******************************************************************************
* File       : adctilt.cpp
* Project    : BizLars
* Date       : 16.11.2017
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adctcc class
******************************************************************************
*/
#define _USE_MATH_DEFINES 
#include "math.h"
#include "bizlars.h"
#include "adctilt.h"

#define DEFAULT_RESOLUTION	4096


AdcTilt::AdcTilt(AdcTiltComp* tilt)
{
	m_resolution = tilt->resolution;
	if (!m_resolution) m_resolution = DEFAULT_RESOLUTION;
}


AdcTilt::~AdcTilt()
{
}

/**
******************************************************************************
* GetAngleDeg - get angle in deg format
*
* @return   void
* @remarks
******************************************************************************
*/
float AdcTilt::GetAngleDeg(long digit)
{
	float deg;

	deg = (float)(asin(digit / m_resolution) * (180 / M_PI));

	return deg;
}


/**
******************************************************************************
* GetAngleDigit - get angle in digit format
*
* @return   void
* @remarks
******************************************************************************
*/
long AdcTilt::GetAngleDigit(float deg)
{
	long digit;

	digit = (long)(m_resolution * sin(deg * M_PI / 180) + 0.5);

	return digit;
}