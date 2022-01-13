#pragma once
#include <fstream>
#include <string>
#include <map>

using namespace std;

class Helpers
{
public:
	Helpers();
	~Helpers();

	static unsigned long GetSystemTickCount();
	static void DebugWrite(const char *fmt, ...);
	static tm* GetTime();

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
	static bool	KeyExists(const map<string, string> &keyValueMap, const string &key, string &value);

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
	static void KeyAddOrReplace(map<string, string> &keyValueMap, const string &key, string &value);


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
	static void ReplaceAll(string &strValue, string &from, string &to);


	/**
	******************************************************************************
	* ReplaceDecimalPoint - replace decimal point with decimal point from current locale
	*
	* @param    keyValueMap:in	map in which key is add or replace
	* @param    key:in			key
	* @param    value:in		value of the key
	*
	* @return   void
	* @remarks
	******************************************************************************
	*/
	static void ReplaceDecimalPoint(string &strValue);

};

