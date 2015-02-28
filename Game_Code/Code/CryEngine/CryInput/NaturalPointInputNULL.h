// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.


#ifndef __NATURALPOINT_INPUT_H__
#define __NATURALPOINT_INPUT_H__
#pragma once

#include "StdAfx.h"

//////////////////////////////////////////////////////////////////////////
struct CNaturalPointInputNull : public INaturalPointInput
{
public:
	CNaturalPointInputNull(){};
	virtual ~CNaturalPointInputNull(){};

	virtual bool Init(){return true;}
	virtual void Update(){};
	virtual bool IsEnabled(){return true;}

	virtual void Recenter(){};

	// Summary;:
	//		Get raw skeleton data
	virtual bool GetNaturalPointData(NP_RawData &npRawData) const {return true;}

};

#endif //__NATURALPOINT_INPUT_H__
