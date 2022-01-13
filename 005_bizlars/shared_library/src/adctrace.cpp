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
#include <stdarg.h>
#include <algorithm>
#include <sstream>
#define TRACE_CREATE
#include "adctrace.h"
#include "helpers.h"


AdcTrace::AdcTrace()
{
	m_fileName.clear();
	m_level = TRC_OFF;

    string controlSequence;
    controlSequence = 0x01;
    m_controlChar[controlSequence] = "<SOH>";
    controlSequence = 0x02;
    m_controlChar[controlSequence] = "<STX>";
    controlSequence = 0x03;
    m_controlChar[controlSequence] = "<ETX>";
    controlSequence = 0x1b;
    m_controlChar[controlSequence] = "<ESC>";
    controlSequence = 0x17;
    m_controlChar[controlSequence] = "<ETB>";
    controlSequence = 0x1c;
    m_controlChar[controlSequence] = "<FS>";
    controlSequence = 0x1d;
    m_controlChar[controlSequence] = "<GS>";
}


AdcTrace::~AdcTrace()
{
}

/**
******************************************************************************
* Trace - function to write into the trace file
*
* @param    level:in		loglevel
* @param    trcMessage:in	Tracedata
*
* @return   Bytes written
* @remarks
******************************************************************************
*/
char* AdcTrace::GetTime()
{
	struct tm * timeinfo;
	static char buffer[80];

	timeinfo = Helpers::GetTime();

	strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);

	return buffer;
}

/**
******************************************************************************
* Trace - function to write into the trace file
*
* @param    level:in		loglevel
* @param    trcMessage:in	Tracedata
*
* @return   Bytes written
* @remarks
******************************************************************************
*/
short AdcTrace::Trace(short level, const char *fmt, ...)
{
	short		ret = 0;
	ofstream	fHandle;
	string	    trcMessage;
	char		buffer[1024];
    size_t      pos;

	if (m_level && (level <= m_level))
	{
		m_mutex.lock();

		fHandle.open(m_fileName, ios::binary | ios::app);
		if (!fHandle.is_open())
		{
			m_mutex.unlock();
			return ret;
		}
		// check file size
		fHandle.seekp(0, ios::end);
		if (fHandle.tellp() > TRC_MAX_FILE_SIZE)
		{
			fHandle.close();

			MakeBackup();

			fHandle.open(m_fileName, ios::binary);
			if (!fHandle.is_open())
			{
				m_mutex.unlock();
				return ret;
			}
		}

		// prepare buffer
		va_list args;
		va_start(args, fmt);
		vsprintf(buffer, fmt, args);
		va_end(args);

		// write buffer
		trcMessage = GetTime() + string("   ") + to_string(level) + string("   ") + buffer + string("\r\n");
        for (map<string, string>::iterator it = m_controlChar.begin(); it != m_controlChar.end(); ++it)
        {
            // replace control sequence
            while ((pos = trcMessage.find((*it).first)) != std::string::npos)
                trcMessage.replace(pos, (*it).first.size(), (*it).second);
        }
        fHandle.write(trcMessage.c_str(), trcMessage.size());

		fHandle.close();

		ret = trcMessage.size();

		m_mutex.unlock();
	}

	return ret;
}

/**
******************************************************************************
* MakeBackup - function to backup the trace file
*
* @param 
*
* @return	
* @remarks
******************************************************************************
*/
bool AdcTrace::MakeBackup()
{
	ofstream	fOutHandle;
	ifstream	fInHandle;
	string		BackupFileName;
	size_t		pos;

	pos = m_fileName.find_last_of(".");

	if (pos != string::npos)
		BackupFileName = m_fileName.substr(0, pos);
	else
		BackupFileName = m_fileName;

	BackupFileName.append(".bak");

	fInHandle.open(m_fileName, ios::binary);
	if (!fInHandle.is_open())
		return false;

	fOutHandle.open(BackupFileName, ios::binary);
	if (!fOutHandle.is_open())
	{
		fInHandle.close();
		return false;
	}

	copy(istreambuf_iterator<char>(fInHandle), istreambuf_iterator<char>(), ostreambuf_iterator<char>(fOutHandle));

	fInHandle.close();
	fOutHandle.close();

	return true;
}

/**
******************************************************************************
* MakeBackup - function to make the configuration
*
* @param	fileName:in		Path + Filename
* @param	level:in		Tracelevel
*
* @return
* @remarks
******************************************************************************
*/
void AdcTrace::SetConfig(const char *fileName, const short level)
{
	m_mutex.lock();

	if ((level >= TRC_ERROR_WARNING) && (level <= TRC_INFO))
		m_level = level;
	else
		m_level = TRC_OFF;

	if (fileName)
	{
		m_fileName = string(fileName);
		if (m_fileName.empty()) m_level = TRC_OFF;
	}
	else
	{
		m_level = TRC_OFF;
	}

	m_mutex.unlock();

	return;
}
