/**
******************************************************************************
* File       : adctilt.h
* Project    : BizLars
* Date       : 16.11.2018
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcssp class
******************************************************************************
*/
#pragma once
#include <string>
#include <map>
#include "bizlars.h"
using namespace std;

class AdcTilt
{
public:
	AdcTilt(AdcTiltComp* tilt);
	~AdcTilt();

	float GetAngleDeg(long digit);
	long GetAngleDigit(float deg);

private:
	long	m_resolution;
};
