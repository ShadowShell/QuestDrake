/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2006.
-------------------------------------------------------------------------

Description: 
	Wraps a unit of Tweak metadata

-------------------------------------------------------------------------
History:
- 28:02:2006  : Created by Matthew Jack

*************************************************************************/

#include "StdAfx.h"
#include "TweakMetadataVariable.h"

//-------------------------------------------------------------------------

CTweakMetadataVariable::CTweakMetadataVariable(CTweakMenuController* pController, const string& command)
: CTweakMetadata(pController, command)
{
	m_type = eTT_CodeVariable;

	m_pIntVariable = NULL;
	m_pFloatVariable = NULL;
}

void CTweakMetadataVariable::Init(int* pVariable, float min, float max, float delta)
{
	m_fDelta = delta;
	m_fMax = max;
	m_fMin = min;

	m_pIntVariable = pVariable;
	m_pFloatVariable = NULL;
}

void CTweakMetadataVariable::Init(float* pVariable, float min, float max, float delta)
{
	m_fDelta = delta;
	m_fMax = max;
	m_fMin = min;

	m_pIntVariable = NULL;
	m_pFloatVariable = pVariable;
}

//-------------------------------------------------------------------------

string CTweakMetadataVariable::GetValue(void)
{
	string result = "Not found";

	if(m_pIntVariable)
	{
		string temp = GetDisplayString(*m_pIntVariable);

		if(temp.empty())
			result.Format("%d", *m_pIntVariable);
		else
			result = temp;
	}
	else if(m_pFloatVariable)
	{
		result.Format("%f", *m_pFloatVariable);
	}

	return result;
}
		
//-------------------------------------------------------------------------

bool CTweakMetadataVariable::ChangeValue(bool bIncrement)
{
	// Get delta
	double fDelta = m_fDelta;

	if (!bIncrement)
		fDelta *= -1.0f;

	if(m_pIntVariable)
	{
		*m_pIntVariable = (int)ClampToLimits(*m_pIntVariable + fDelta);
	}
	else if(m_pFloatVariable)
	{
		*m_pFloatVariable = (float)ClampToLimits(*m_pFloatVariable + fDelta);
	}

	return true;
}

