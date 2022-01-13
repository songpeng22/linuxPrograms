/**
******************************************************************************
* File       : AdcTrace.h
* Project    : BizLars
* Date       : 25.08.2015
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : trace functions
******************************************************************************
*/
#pragma once
#include <fstream>
#include <ostream>
#include <time.h>
#include <mutex>
#include <map>
using namespace std;

class AdcTrace
{
public:
	AdcTrace();
	~AdcTrace();

	short	Trace(short level, const char *fmt, ...);
	void	SetConfig(const char *fileName, const short level);

	static const short TRC_OFF = 0;
	static const short TRC_ERROR_WARNING = 1;
	static const short TRC_ACTION = 2;
	static const short TRC_INFO = 3;

	static const long TRC_MAX_FILE_SIZE = 0x100000;				// max file size 1 MByte

private:
	bool		MakeBackup();
	char		*GetTime();

	string		m_fileName;
	short		m_level;
	mutex		m_mutex;
    map <string, string> m_controlChar;
};


#ifdef TRACE_CREATE
#define EXTERN 
#else
#define EXTERN extern
#endif

EXTERN AdcTrace g_adcTrace;

