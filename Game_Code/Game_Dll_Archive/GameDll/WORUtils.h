
#ifndef __WOR_UTILITY_H_INCLUDED__ // If not defined
#define __WOR_UTILITY_H_INCLUDED__ // Define it


//using namespace std;
//
#include<string>
#include<fstream>
#include<iostream>
#include<windows.h>

#include <string.h>
#include <wchar.h>
#include <limits.h>
#include <stdarg.h>
#include <ctype.h>



//#include "StdAfx.h"


//inline void DebuggerLogThis( std::string IN_FILE  , std::string IN_NAME )
//{
//	// IN_FILE = could be GameScreen.cpp etc.
//	// IN_NAME = could be Init(int a , int b) etc.
//
//	std::string OUT_FILE = "DebugLog.txt";
//
//	SYSTEMTIME time;
//	GetSystemTime(&time);
//	std::ofstream DebugFile;
//
//	DebugFile.open (OUT_FILE, std::ios_base::app);
//	DebugFile << 
//	"|" << std::to_string(time.wMilliseconds) << ":" <<
//	""  << std::to_string(time.wSecond      ) << ":" <<
//	""  << std::to_string(time.wMinute      ) << ":" <<
//	""  << std::to_string(time.wHour        ) << "|" <<
//	""  << std::to_string(time.wDay         ) << ":" <<
//	""  << std::to_string(time.wMonth       ) << ":" <<
//	""  << std::to_string(time.wYear        ) << "|" <<
//	" Running " << IN_FILE << IN_NAME  << "\n";
//	DebugFile.close();
//}

inline float GetExposed( int IN_LINE_NUMBER )
{
	// IN_LINE_NUMBER = the line number of the txtfile to pass to a number, it will pass the first 10 characters

	std::ifstream infile;
	std::string importFilePath;
	importFilePath = "Exposed.txt";
	infile.open(importFilePath); 

	std::string line;
	std::string newString;

	int l = 0;

	while ( getline(infile, line) ) // runs through the file line by line
	{
		// if the line select is equal to the line we want to get
		if (l == IN_LINE_NUMBER)
		{
			for (int i = 0; i < 10; i++)
			{
				newString = newString + line[i]; // construct string to be passed
			}
		}

		l++;
	}

	infile.close();
	float exposedVar = std::stof( newString );
	return exposedVar;
}

#endif//__WOR_UTILITY_H_INCLUDED__ // End define


