/**
******************************************************************************
* File       : authentication.cpp
* Project    : BizLars
* Date       : 18.11.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : authentication class
******************************************************************************
*/
#include "authentication.h"


Authentication::Authentication(unsigned short crcStart, unsigned short crcPoly, unsigned short crcMask)
{
    m_crcStart = crcStart;
    m_crcPoly = crcPoly;
    m_crcMask = crcMask;
}


Authentication::~Authentication()
{
}


/**
******************************************************************************
* CalcCrc - function to calculate the CRC 
*
* @param    buffer:in				buffer over which the checksum is calculated
* @param    size:in 				size of buffer
*
* @return   crc
* @remarks
******************************************************************************
*/
unsigned short Authentication::CalcCrc(unsigned char *buffer, unsigned long size)
{
    unsigned short crc;

    // set start polynom
    crc = m_crcStart;

    // calculate the crc
    for (unsigned long i = 0; i < size; i++)
    {
        calcCrcOverOneByte(buffer[i], &crc);
    }

    crc ^= m_crcMask;
   
    return crc;
}


/**
******************************************************************************
* calcCrcOverOneByte - function to calculate the CRC over one byte
*
* @param    data:in				    
* @param    crc:out				    crc value
*
* @return
* @remarks
******************************************************************************
*/
void Authentication::calcCrcOverOneByte(unsigned char data, unsigned short *crc)
{
    *crc ^= (unsigned short)data << 8;

    for (int i = 0; i < 8; i++)
    {
        if (0x8000 & *crc)                       // check top bit
        {
            *crc <<= 1;
            *crc ^= m_crcPoly;
        }
        else
        {
            *crc <<= 1;
        }
    }

    return;
}



/*
crc_buffer[0]=0x82;
crc_buffer[1]=0x69;
crc_buffer[2]=0xAB;
crc_buffer[3]=0xCA;
crc_buffer[4]=0x4B;
crc_buffer[5]=0x84;
crc_buffer[6]=0x5A;
crc_buffer[7]=0x96;
*/
//           Ergebnis --> CRC von 8 Byte = DAB9

/*
crcwert=0xDAB9;
*/
//           Ergebnis --> CRC von 2 Byte = 7191




