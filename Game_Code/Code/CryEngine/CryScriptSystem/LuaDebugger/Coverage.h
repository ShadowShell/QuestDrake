// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef _COVERAGE_H_
#define _COVERAGE_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <VectorMap.h>

class CLUACodeCoverage
{
	typedef std::set<int> TLines;
	typedef VectorMap<string, TLines> TCoverage;
	typedef std::vector<TCoverage> TCoverageStack;
public:
	void VisitLine(const char* source, int line);
	void ResetFile(const char* source);
	bool GetCoverage(const char* source, int line)const;
	void DumpCoverage()const;
private:
	TCoverage m_coverage;
};

#endif //#ifndef _COVERAGE_H_
