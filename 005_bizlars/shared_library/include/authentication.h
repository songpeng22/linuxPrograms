/**
******************************************************************************
* File       : adcauthentication.h
* Project    : BizLars
* Date       : 18.11.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : adcauthentication class 
******************************************************************************
*/
#pragma once

class Authentication
{
public:
   
    Authentication(unsigned short crcStart, unsigned short crcPoly, unsigned short crcMask);
    ~Authentication();

    unsigned short CalcCrc(unsigned char *buffer, unsigned long size);
    
private:
    void calcCrcOverOneByte(unsigned char data, unsigned short *crc);

    unsigned short m_crcStart;
    unsigned short m_crcPoly;
    unsigned short m_crcMask;
};

