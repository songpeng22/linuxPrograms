/**
******************************************************************************
* File       : helpers.cpp
* Project    : BizLars
* Date       : 06.07.2016
* Author     : Thomas Buck, Sensor Technology
* Copyright  : Bizerba GmbH & Co. KG
*
* Content    : helpers class: include helpers functions
******************************************************************************
*/
#ifdef  _MSC_VER
#include <Windows.h>
#endif

#ifdef __GNUC__
#include <fstream>
#include <stdarg.h>
#endif
#include <time.h>
#include <iostream>
#include <locale>


#include "helpers.h"

Helpers::Helpers()
{
}


Helpers::~Helpers()
{
}

unsigned long Helpers::GetSystemTickCount()
{
#ifdef  _MSC_VER
	return GetTickCount();
#endif

#ifdef __GNUC__
	return 0;
#endif
}

void Helpers::DebugWrite(const char *fmt, ...)
{
	char		buffer[1024];

	// prepare buffer
	va_list args;
	va_start(args, fmt);
	vsprintf(buffer, fmt, args);
	va_end(args);

#ifdef  _MSC_VER
	OutputDebugStringA(buffer);
#endif

#ifdef __GNUC__
	return;
#endif
}


tm* Helpers::GetTime()
{
	time_t rawtime;

	// get local time
	time(&rawtime);
	return (localtime(&rawtime));
}


/**
******************************************************************************
* KeyExists - check if the map contain the key
*
* @param    keyValueMap:in	source map
* @param    key:in			key
* @param    value:in		value of the key
*
* @return   true	key exist
*			false	key not exist
* @remarks
******************************************************************************
*/

bool Helpers::KeyExists(const map<string, string> &keyValueMap, const string &key, string &value)
{
	map<string, string>::const_iterator it = keyValueMap.find(key);
	if (it != keyValueMap.end())
	{
		value = (*it).second;
		return true;
	}
	else
	{
		return false;
	}
}


/**
******************************************************************************
* KeyAddOrReplace - add key to map or replace existing key
*
* @param    keyValueMap:in	map in which key is add or replace
* @param    key:in			key
* @param    value:in		value of the key
*
* @return   void
* @remarks
******************************************************************************
*/
void Helpers::KeyAddOrReplace(map<string, string> &keyValueMap, const string &key, string &value)
{
	map<string, string>::iterator it = keyValueMap.find(key);
	if (it != keyValueMap.end())
	{
		(*it).second = value;
	}
	else
	{
		keyValueMap[key] = value;
	}
}


/**
******************************************************************************
* ReplaceAll - replace all characters from with character to in string
*
* @param    strValue:in/out	string
* @param    from:in			replacing string
* @param    to:in			new content
*
* @return   void
* @remarks
******************************************************************************
*/
void Helpers::ReplaceAll(string &strValue, string &from, string &to)
{
    size_t start_pos = 0;
	while ((start_pos < strValue.length()) && (start_pos = strValue.find(from, start_pos)) != string::npos)
    {
		strValue.replace(start_pos, from.length(), to);
        start_pos++;
    }
    return;
}


/**
******************************************************************************
* ReplaceDecimalPoint - replace decimal point with decimal point from current locale
*
* @param    strValue:in/out	float as string
*
* @return   void
* @remarks
******************************************************************************
*/
void Helpers::ReplaceDecimalPoint(string &strValue)
{
	string dotString = ".";
#ifdef  _MSC_VER
	// get decimal point from current locale
	string decimalPoint = string(1, std::use_facet< std::numpunct<char> >(std::cout.getloc()).decimal_point());
#endif
#ifdef __GNUC__
	// get decimal point from current locale
	struct lconv *currentLocale = localeconv();
	string decimalPoint = string(currentLocale->decimal_point);
#endif

	if (dotString != decimalPoint)
		ReplaceAll(strValue, dotString, decimalPoint); // replace all '.' with decimal point from current locale

	return;
}
